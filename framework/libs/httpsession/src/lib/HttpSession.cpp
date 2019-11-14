#include "HttpSession.h"

#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <vector>

#define __LOG__(_tag_, ...) printf("[" #_tag_ "] " __VA_ARGS__)
#define RKLog(...) __LOG__(INFO, __VA_ARGS__)
#define RKWarn(...) __LOG__(WARN, __VA_ARGS__)
#define RKError(...) __LOG__(ERROR, __VA_ARGS__)

#define DEBUG_LOCKS 0

#if DEBUG_LOCKS
#define http_session_lock(_m_)                   \
    do                                           \
    {                                            \
        RKLog("%s %p lock\n", __func__, this);   \
        (_m_).lock();                            \
        RKLog("%s %p locked\n", __func__, this); \
    } while (0)

#define http_session_unlock(_m_)                 \
    do                                           \
    {                                            \
        (_m_).unlock();                          \
        RKLog("%s %p unlock\n", __func__, this); \
    } while (0)

#define http_session_try_lock(_m_)                         \
    ({                                                     \
        int ret = (_m_).try_lock();                        \
        RKLog("%s %p try_lock %d\n", __func__, this, ret); \
        ret;                                               \
    })
#else
#define http_session_lock(_m_) (_m_).lock()
#define http_session_unlock(_m_) (_m_).unlock()
#define http_session_try_lock(_m_) (_m_).try_lock()
#endif

#if DEBUG_LOCKS

struct HttpSessionLockGuard
{
    string file;
    int line;
    string func;
    void *userdata;
    mutex &m;

    HttpSessionLockGuard(string file, int line, string func, void *userdata, mutex &m)
        : file(file)
        , line(line)
        , func(func)
        , userdata(userdata)
        , m(m)
    {
        RKLog("[%s %d] %s %p lock\n", file.c_str(), line, func.c_str(), userdata);
        m.lock();
    }

    ~HttpSessionLockGuard()
    {
        RKLog("[%s %d] %s %p unlock\n", file.c_str(), line, func.c_str(), userdata);
        m.unlock();
    }

    HttpSessionLockGuard(HttpSessionLockGuard const &) = delete;
    HttpSessionLockGuard &operator=(HttpSessionLockGuard const &) = delete;
};

#define http_session_lock_guard(_n_, _m_) HttpSessionLockGuard _n_(__FILE__, __LINE__, __func__, this, (_m_))
#else
#define http_session_lock_guard(_n_, _m_) lock_guard<mutex> _n_(_m_)
#endif

#define CURL_EXEC(_F_, ...)              \
    ({                                   \
        CURLcode ret = _F_(__VA_ARGS__); \
        if (ret != CURLE_OK)             \
        {                                \
            false;                       \
        }                                \
        true;                            \
    })

#define CURLM_EXEC(_F_, ...)              \
    ({                                    \
        CURLMcode ret = _F_(__VA_ARGS__); \
        if (ret != CURLM_OK)              \
        {                                 \
            false;                        \
        }                                 \
        true;                             \
    })

static double get_timestamp(void)
{
    struct timespec spec;
    if (clock_gettime(CLOCK_MONOTONIC, &spec))
    {
        return 0;
    }
    return (double)spec.tv_sec + ((double)spec.tv_nsec) / 1e9;
}

static bool inited = false;

static void global_init()
{
    if (inited)
    {
        return;
    }

    inited = true;
    curl_global_init(CURL_GLOBAL_ALL);
}

static void global_deinit()
{
    if (!inited)
    {
        return;
    }

    curl_global_cleanup();
}

static const char *nindex(const char *buf, size_t size, char c)
{
    for (int i = 0; i < size; ++i)
    {
        if (*(buf + i) == c)
        {
            return buf + i;
        }
    }
    return NULL;
}

static const char *parse_number(const char *buf, int *v)
{
    if (isdigit(*buf))
    {
        *v = *v * 10 + (*buf - '0');
        return parse_number(buf + 1, v);
    }
    else if (*v != 0)
    {
        return buf;
    }
    else
    {
        return NULL;
    }
}

static const char *parse_space(const char *buf)
{
    if (*buf != ' ')
    {
        return NULL;
    }
    while (*++buf == ' ')
    {
    }
    return buf;
}

#define PARSE_ARG(_F_, _V_, ...)                               \
    if ((buf = parse_##_F_(buf, __VA_ARGS__)) == NULL)         \
    {                                                          \
        printf("%s parse_" #_F_ " %s error\n", __func__, buf); \
        return (_V_);                                          \
    }
#define PARSE(_F_, _V_)                                        \
    if ((buf = parse_##_F_(buf)) == NULL)                      \
    {                                                          \
        printf("%s parse_" #_F_ " %s error\n", __func__, buf); \
        return (_V_);                                          \
    }

static const char *parse_http_version(const char *buf, int *major, int *minor)
{
    if (strncmp(buf, "HTTP/", 5) != 0)
    {
        return NULL;
    }
    buf += 5;
    *major = 0;
    PARSE_ARG(number, NULL, major);

    if (*buf++ != '.')
    {
        return NULL;
    }

    *minor = 0;
    PARSE_ARG(number, NULL, minor);

    return buf;
}

static int parse_status_line(const char *buf,
                             // align with first parameter
                             int *major, int *minor, int *code, const char **reason)
{
    PARSE_ARG(http_version, -1, major, minor);
    PARSE(space, -1);
    *code = 0;
    PARSE_ARG(number, -1, code);
    PARSE(space, -1);
    *reason = buf;
    return 0;
}

static int parse_header_line(const char *buf, const char **key, size_t *nkey, const char **value)
{
    const char *temp = index(buf, ':');
    if (temp == NULL)
    {
        return -1;
    }
    *key = buf;
    *nkey = temp - *key;

    buf = temp + 1;
    PARSE(space, -1);

    *value = buf;

    return 0;
}

size_t HttpSession::Ticket::parseResponse(char *buf, size_t size)
{
    http_session_lock_guard(lock, mMutex);
    mBuffer.append(buf, size);
    return size;
}

size_t HttpSession::Ticket::parseHeader(char *buf, size_t size)
{
    http_session_lock_guard(lock, mMutex);

    const char *temp = nindex(buf, size, '\r');
    if (temp && *++temp == '\n')
    {
        mBuffer.append(buf, temp++ - buf - 1);
        if (mBuffer.empty())
        {
            mParseHeaderFinished = true;
            goto parsed;
        }
        if (response.code == 0 || response.code == 100 || mParseHeaderFinished)
        {
            if (mVerbose)
            {
                RKLog("clear headers, response code was %d\n", response.code);
            }
            mParseHeaderFinished = false;
            response.headers.clear();
            const char *r = NULL;
            if (parse_status_line(mBuffer.c_str(), &response.majorVersion, &response.minorVersion, &response.code, &r))
            {
                RKWarn("parse_status_line failed %s\n", mBuffer.c_str());
                return 0;
            }
            response.reason.assign(r);
        }
        else
        {
            const char *key = NULL, *value = NULL;
            size_t nkey = 0;
            if (parse_header_line(mBuffer.c_str(), &key, &nkey, &value))
            {
                RKWarn("parse_header_line failed %s\n", mBuffer.c_str());
                return 0;
            }
            string skey(key, nkey);
            response.headers[skey] = value;
        }
    parsed:
        mBuffer.assign(temp, size - (temp - buf));
    }
    else
    {
        mBuffer.append(buf, size);
    }

    return size;
}

void HttpSession::Ticket::willBeClean()
{
    auto info = &mInfo;
#define CURL_GET_TIME(_key_, _value_) CURL_EXEC(curl_easy_getinfo, mEasyHandle, (_key_), &info->_value_)
    CURL_GET_TIME(CURLINFO_TOTAL_TIME, totleTime);
    CURL_GET_TIME(CURLINFO_NAMELOOKUP_TIME, nameLookupTime);
    CURL_GET_TIME(CURLINFO_CONNECT_TIME, connectTime);
    CURL_GET_TIME(CURLINFO_APPCONNECT_TIME, handshakeTime);
    CURL_GET_TIME(CURLINFO_PRETRANSFER_TIME, pretransferTime);
    CURL_GET_TIME(CURLINFO_STARTTRANSFER_TIME, startTransferTime);
    CURL_GET_TIME(CURLINFO_REDIRECT_TIME, redirectTime);
#undef CURL_GET_TIME
}

void HttpSession::Ticket::markAsFinished(CURLcode code, const char *msg)
{
    if (mVerbose)
    {
        RKLog("%s %p error %d msg %s\n", __func__, this, code, msg);
    }
    http_session_lock(mMutex);

    mEndTime = get_timestamp();
    if (mVerbose)
    {
#define CURL_TIME_KEY_AND_VALUE(_value_) #_value_ ":", mInfo._value_
        RKLog(
            "%s %p duration %lf\n"
            "%20s %.3lf\n%20s %.3lf\n%20s %.3lf\n%20s %.3lf\n"
            "%20s %.3lf\n%20s %.3lf\n%20s %.3lf\n",
            __func__, this, duration(), CURL_TIME_KEY_AND_VALUE(totleTime), CURL_TIME_KEY_AND_VALUE(nameLookupTime),
            CURL_TIME_KEY_AND_VALUE(connectTime), CURL_TIME_KEY_AND_VALUE(handshakeTime),
            CURL_TIME_KEY_AND_VALUE(pretransferTime), CURL_TIME_KEY_AND_VALUE(startTransferTime),
            CURL_TIME_KEY_AND_VALUE(redirectTime));
#undef CURL_TIME_KEY_AND_VALUE
    }

    if (code == CURLE_OK)
    {
        mStatus = FINISHED;
        mErrorCode = 0;
        mError.erase();
    }
    else
    {
        mStatus = FAILED;
        mErrorCode = code;
        mError.assign(msg);
    }

    if (mBuffer.empty())
    {
        response.body = NULL;
        response.contentLength = 0;
    }
    else
    {
        response.body = mBuffer.c_str();
        response.contentLength = mBuffer.size();
    }

    http_session_unlock(mMutex);

    if (mListener)
    {
        mListener->onRequestFinished(mSession, this, &response);
    }
}

HttpSession::Ticket::~Ticket()
{
    if (mVerbose)
    {
        RKLog("%s %p\n", __func__, this);
    }
    http_session_lock_guard(lock, mMutex);

    if (mStatus == PULLING)
    {
        RKWarn("%s ticket is pulling %p\n", __func__, this);
    }

    if (mDeleter)
    {
        mDeleter(this);
    }
    if (request.releaseBody && request.body)
    {
        delete request.body;
    }
}

HttpSession::HttpSession(Options opt)
    : mOpt(opt)
    , mMsgQueue(new MessageQueue)
{
    global_init();
    tryStart();
}

bool HttpSession::tryStart()
{
    if (mOpt.verbose)
    {
        RKLog("%s %d %p\n", __func__, mIsRunning, mSelectThread);
    }
    if (mSelectThread)
    {
        return true;
    }

    mSelectThread = new thread([=] { selectRoutine(); });
    if (!mSelectThread)
    {
        RKError("create select thread failed\n");
        return false;
    }
    if (mOpt.verbose)
    {
        RKLog("return isRunning %d %p\n", mIsRunning, mSelectThread);
    }
    return true;
}

HttpSession::~HttpSession()
{
    if (mOpt.verbose)
    {
        RKLog("%s %p is running: %d, has %d easy handles,\n", __func__, this, mIsRunning, mEasyHandles.size());
    }

    http_session_lock(mMutex);
    mIsRunning = false;
    if (mSelectThread)
    {
        http_session_unlock(mMutex);
        mSelectThread->join();
        delete mSelectThread;
    }
    else
    {
        http_session_unlock(mMutex);
    }

    if (mMsgQueue)
    {
        delete mMsgQueue;
    }
}

shared_ptr<HttpSession::Ticket> HttpSession::request(Request &req, HttpSessionRequestListenerInterface *listener,
                                                     Ticket::Deleter deleter)
{
    http_session_lock_guard(lock, mMutex);
    if (!mIsRunning)
    {
        RKWarn("httpsession %p is not running\n", this);
        return NULL;
    }
    if (!tryStart())
    {
        RKWarn("tryStart failed\n");
        return NULL;
    }

    auto *tic = new Ticket(req, deleter);
    if (tic == NULL)
    {
        RKWarn("create ticket failed\n");
        return NULL;
    }
    tic->mListener = listener;
    tic->mSession = this;
    tic->mVerbose = mOpt.verbose;

    auto ref = shared_ptr<Ticket>(tic);

    mMsgQueue->push(new Message(MESSAGE_TYPE_REQUEST, ref));

    if (mOpt.verbose)
    {
        RKLog("return ticket %p %s\n", tic, req.url.c_str());
    }
    return ref;
}

int HttpSession::cancel(shared_ptr<Ticket> tic)
{
    if (mOpt.verbose)
    {
        RKLog("%s %p %s\n", __func__, tic, tic->request.url.c_str());
    }
    if (!tic)
    {
        return -1;
    }

    http_session_lock_guard(lock, mMutex);
    if (!mIsRunning)
    {
        RKWarn("httpsession %p is not running\n", this);
        return -1;
    }

    mMsgQueue->push(new Message(MESSAGE_TYPE_CANCEL, tic));
    return 0;
}

int HttpSession::stop(shared_ptr<Ticket> tic)
{
    if (mOpt.verbose)
    {
        RKLog("%s %p %s\n", __func__, tic, tic->request.url.c_str());
    }
    if (!tic)
    {
        return -1;
    }

    http_session_lock_guard(lock, mMutex);
    if (!mIsRunning)
    {
        RKWarn("httpsession %p is not running\n", this);
        return -1;
    }
    http_session_lock_guard(tlock, tic->mMutex);
    tic->mListener = NULL;

    mMsgQueue->push(new Message(MESSAGE_TYPE_STOP, tic));
    return 0;
}

void HttpSession::selectRoutine()
{
    if (mOpt.verbose)
    {
        RKLog("%s\n", __func__);
    }
#define SHORT_SLEEP_TIME 50
#define SHORT_SLEEP usleep(SHORT_SLEEP_TIME * 1e3)
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int rc, maxfd = -1;
    long curl_timeo;
    CURLMcode mc;
    struct timeval timeout;
    Message *msg = NULL;

    while (true)
    {
        {
            http_session_lock_guard(lock, mMutex);
            if (!mIsRunning)
            {
                break;
            }
            if (!mMultiHandle)
            {
                if ((mMultiHandle = curl_multi_init()) == NULL)
                {
                    RKError("curl_multi_init error\n");
                    continue;
                }
                if (mOpt.verbose)
                {
                    RKLog("%p mMultiHandle %p\n", this, mMultiHandle);
                }
            }

            msg = mMsgQueue->pop();
        }

        if (msg)
        {
            handleMessage(msg);
            delete msg;
        }

        CURLM_EXEC(curl_multi_timeout, mMultiHandle, &curl_timeo);
        if (curl_timeo < 0 || curl_timeo > SHORT_SLEEP_TIME)
        {
            curl_timeo = SHORT_SLEEP_TIME;
        }

        timeout.tv_sec = curl_timeo / 1000;
        timeout.tv_usec = (curl_timeo % 1000) * 1000;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* get file descriptors from the transfers */
        if (!CURLM_EXEC(curl_multi_fdset, mMultiHandle, &fdread, &fdwrite, &fdexcep, &maxfd))
        {
            continue;
        }

        if (maxfd == -1)
        {
            SHORT_SLEEP;
            rc = 0;
        }
        else
        {
            rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        if (rc == -1)
        {
            RKError("select error: %s\n", strerror(errno));
        }
        else
        {
            performActions();
        }
    }

    http_session_lock_guard(lock, mMutex);
    if (mOpt.verbose)
    {
        RKLog("%s removed all handles\n", __func__);
    }

    msg = mMsgQueue->pop();
    while (msg)
    {
        delete msg;
        msg = mMsgQueue->pop();
    }

    if (mMultiHandle)
    {
        vector<shared_ptr<Ticket>> allTics;
        auto it = mEasyHandles.begin();
        while (it != mEasyHandles.end())
        {
            allTics.push_back(it->second);
            ++it;
        }
        for (auto tic : allTics)
        {
            handleCancelMessage(tic);
        }
        curl_multi_cleanup(mMultiHandle);
        mMultiHandle = NULL;
    }
    else if (mEasyHandles.size() > 0)
    {
        RKWarn("mMultiHandle is NULL, there should no tickets\n");
    }

    if (mOpt.verbose)
    {
        RKLog("%s %p finished\n", __func__, this);
    }
}

int HttpSession::handleMessage(Message *msg)
{
    if (msg == NULL || msg->tic == NULL || msg->type == MESSAGE_TYPE_NONE)
    {
        return -1;
    }
    switch (msg->type)
    {
        case MESSAGE_TYPE_REQUEST:
            return handleRequestMessage(msg->tic);
        case MESSAGE_TYPE_STOP:
            return handleStopMessage(msg->tic);
        case MESSAGE_TYPE_CANCEL:
            return handleCancelMessage(msg->tic);
        default:
            return -1;
    }
}

int HttpSession::handleRequestMessage(shared_ptr<Ticket> tic)
{
    if (mOpt.verbose)
    {
        RKLog("%s %p use_count %ld\n", __func__, tic.get(), tic.use_count());
    }
    if (tic == NULL)
    {
        RKWarn("ticket has been released\n");
        return -1;
    }

    http_session_lock_guard(lock, mMutex);
    if (mMultiHandle == NULL)
    {
        return -1;
    }
    http_session_lock_guard(tlock, tic->mMutex);

    CURL *handle = buildEasyHandle(tic->request, tic.get());
    if (handle == NULL)
    {
        RKWarn("build easy handle failed\n");
        return -1;
    }
    if (!CURLM_EXEC(curl_multi_add_handle, mMultiHandle, handle))
    {
        return -1;
    }

    tic->mStartTime = get_timestamp();
    tic->mEasyHandle = handle;
    tic->mStatus = PULLING;

    mEasyHandles[handle] = tic;

    return 0;
}

int HttpSession::handleStopMessage(shared_ptr<Ticket> tic)
{
    if (mOpt.verbose)
    {
        RKLog("%s %p use_count %ld\n", __func__, tic.get(), tic.use_count());
    }
    if (tic == NULL)
    {
        RKWarn("ticket has been released\n");
        return -1;
    }

    clean(tic.get());

    http_session_lock_guard(tlock, tic->mMutex);
    if (tic->mStatus == PULLING)
    {
        tic->mStatus = INIT;
    }
    return 0;
}

int HttpSession::handleCancelMessage(shared_ptr<Ticket> tic)
{
    if (mOpt.verbose)
    {
        RKLog("%s %p use_count %ld\n", __func__, tic.get(), tic.use_count());
    }
    if (tic == NULL)
    {
        RKWarn("ticket has been released\n");
        return -1;
    }

    if (clean(tic.get()))
    {
        return -1;
    }

    http_session_lock(tic->mMutex);
    if (tic->mStatus == PULLING)
    {
        tic->mStatus = CANCELED;
        http_session_unlock(tic->mMutex);
        if (tic->mListener)
        {
            tic->mListener->onRequestCanceled(this, tic.get());
        }
        return 0;
    }
    http_session_unlock(tic->mMutex);
    return 0;
}

int HttpSession::clean(Ticket *tic, bool onlyHandle)
{
    http_session_lock(tic->mMutex);
    CURL *handle = tic->mEasyHandle;
    if (mOpt.verbose)
    {
        RKLog("%s tic %p(%s) handle %p\n", __func__, tic, tic->request.url.c_str(), handle);
    }
    tic->willBeClean();
    http_session_unlock(tic->mMutex);

    if (!handle)
    {
        return -1;
    }

    bool locked = false;
    if (http_session_try_lock(mMutex))
    {
        locked = true;
    }
    CURLM_EXEC(curl_multi_remove_handle, mMultiHandle, handle);
    mEasyHandles.erase(handle);
    if (locked)
    {
        http_session_unlock(mMutex);
    }

    curl_easy_cleanup(handle);

    http_session_lock(tic->mMutex);
    tic->mEasyHandle = NULL;
    if (tic->mRequestHeaders)
    {
        curl_slist_free_all(tic->mRequestHeaders);
        tic->mRequestHeaders = NULL;
    }
    if (!onlyHandle)
    {
        tic->mSession = NULL;
        tic->mBuffer.clear();
        tic->mError.clear();
        tic->mErrorCode = 0;
    }
    http_session_unlock(tic->mMutex);

    return 0;
}

void HttpSession::performActions()
{
    int running, left;
    CURLcode code;
    CURLMsg *msg;

    CURLM_EXEC(curl_multi_perform, mMultiHandle, &running);

    while (true)
    {
        msg = curl_multi_info_read(mMultiHandle, &left);
        if (msg == NULL)
        {
            break;
        }
        if (msg->msg != CURLMSG_DONE)
        {
            continue;
        }

        code = msg->data.result;
        http_session_lock(mMutex);
        auto tic = mEasyHandles[msg->easy_handle];
        http_session_unlock(mMutex);
        if (mOpt.verbose)
        {
            RKLog("%s tic %p is done\n", __func__, tic.get());
        }
        if (tic != NULL)
        {
            clean(tic.get(), true);
            tic->markAsFinished(code, curl_easy_strerror(code));
        }
    }
}

CURL *HttpSession::buildEasyHandle(const Request &req, Ticket *tic)
{
    if (mOpt.verbose)
    {
        RKLog("%s %s %s\n", __func__, req.url.c_str(), req.method.c_str());
    }
    CURL *handle = curl_easy_init();
    if (handle == NULL)
    {
        RKError("curl_easy_init error\n");
        return NULL;
    }

#define CURL_SETOPT(_OPT_, _V_) CURL_EXEC(curl_easy_setopt, handle, (_OPT_), (_V_))
#define CURL_SET_VERIFYOPT(_OPT_)                   \
    if (!(req.verifyPolicy & VERIFY_##_OPT_))       \
    {                                               \
        CURL_SETOPT(CURLOPT_SSL_VERIFY##_OPT_, 0L); \
    }
#define CURL_SET_STROPT(_OPT_, _STR_)          \
    if (!req._STR_.empty())                    \
    {                                          \
        CURL_SETOPT(_OPT_, req._STR_.c_str()); \
    }

    CURL_SETOPT(CURLOPT_URL, (mOpt.baseUrl + req.url).c_str());
    if (mOpt.verbose)
    {
        CURL_SETOPT(CURLOPT_VERBOSE, 1L);
    }

    CURL_SET_VERIFYOPT(PEER);
    CURL_SET_VERIFYOPT(HOST);
    CURL_SET_VERIFYOPT(STATUS);

    CURL_SET_STROPT(CURLOPT_SSLCERT, sslCert);
    CURL_SET_STROPT(CURLOPT_SSLCERTTYPE, sslCertType);
    CURL_SET_STROPT(CURLOPT_SSLKEY, sslKey);
    CURL_SET_STROPT(CURLOPT_KEYPASSWD, sslPassword);

    // https://stackoverflow.com/a/10755612
    CURL_SETOPT(CURLOPT_NOSIGNAL, 1L);
    CURL_SETOPT(CURLOPT_DNS_USE_GLOBAL_CACHE, false);
    CURL_SETOPT(CURLOPT_DNS_CACHE_TIMEOUT, 2);

    if (req.followLocation != 0)
    {
        if (mOpt.verbose)
        {
            RKLog("%s follow location\n", __func__);
        }
        CURL_SETOPT(CURLOPT_FOLLOWLOCATION, 1L);
        CURL_SETOPT(CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
        if (req.followLocation > 0)
        {
            CURL_SETOPT(CURLOPT_MAXREDIRS, req.followLocation);
        }
    }

    if (req.method == "POST")
    {
        CURL_SETOPT(CURLOPT_POST, 1L);
        if (mOpt.verbose)
        {
            RKLog("%s POST body %s\n", __func__, (char *)req.body);
        }
        if (req.body)
        {
            CURL_SETOPT(CURLOPT_POSTFIELDS, req.body);
            size_t len = req.length >= 0 ? req.length : strlen((const char *)req.body);
            CURL_SETOPT(CURLOPT_POSTFIELDSIZE, len);
        }
        else
        {
            CURL_SETOPT(CURLOPT_POSTFIELDSIZE, 0L);
        }
    }
    else if (req.method == "PUT")
    {
        CURL_SETOPT(CURLOPT_PUT, 1L);
    }
    else if (req.method != "GET")
    {
        CURL_SETOPT(CURLOPT_CUSTOMREQUEST, req.method.c_str());
    }

    int timeout = req.timeout ?: mOpt.timeout;
    if (timeout != 0)
    {
        CURL_SETOPT(CURLOPT_TIMEOUT, timeout);
    }

    struct curl_slist *headers = NULL;
    if (!req.headers.empty())
    {
        auto it = req.headers.begin();
        while (it != req.headers.end())
        {
            headers = curl_slist_append(headers, (it->first + ": " + it->second).c_str());
            it++;
        }
        CURL_SETOPT(CURLOPT_HTTPHEADER, headers);
    }

    CURL_SETOPT(CURLOPT_WRITEFUNCTION, handleWriteCallback);
    CURL_SETOPT(CURLOPT_WRITEDATA, tic);
    CURL_SETOPT(CURLOPT_HEADERFUNCTION, handleHeaderCallback);
    CURL_SETOPT(CURLOPT_HEADERDATA, tic);

    tic->mRequestHeaders = headers;

    if (mOpt.verbose)
    {
        RKLog("return handle %p headers %p\n", handle, headers);
    }
    return handle;
}

size_t HttpSession::handleWriteCallback(char *buf, size_t size, size_t nmemb, void *userdata)
{
    if (userdata == NULL)
    {
        return 0;
    }
    Ticket *tic = (Ticket *)userdata;
    return tic->parseResponse(buf, nmemb);
}

size_t HttpSession::handleHeaderCallback(char *buf, size_t size, size_t nitems, void *userdata)
{
    if (userdata == NULL)
    {
        return 0;
    }
    Ticket *tic = (Ticket *)userdata;
    return tic->parseHeader(buf, nitems);
}

int HttpSession::handleProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    return 0;
}

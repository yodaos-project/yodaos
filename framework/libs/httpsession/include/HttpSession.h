#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include <curl/multi.h>
#include <curl/easy.h>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>

using namespace std;

class HttpSessionRequestListenerInterface;

class HttpSession
{
public:
    struct Options
    {
        string baseUrl;
        int timeout;
        bool verbose;
    };

    enum SSLVerifyPolicy
    {
        VERIFY_NONE = 0,
        VERIFY_PEER = 1 << 0,
        VERIFY_HOST = 1 << 1,
        VERIFY_STATUS = 1 << 2,
        VERIFY_ALL = 0xff,
    };

    struct Request
    {
        string url;
        string method = "GET";
        map<string, string> headers;
        int timeout = 0;
        const char *body = NULL;
        size_t length = -1;
        int followLocation = -1;
        bool releaseBody = false;

        void *userdata = NULL;

        SSLVerifyPolicy verifyPolicy = VERIFY_ALL;
        string sslCert, sslCertType, sslKey, sslPassword;
    };

    struct Response
    {
        int majorVersion = 0;
        int minorVersion = 0;
        int code = 0;
        string reason;
        map<string, string> headers;

        size_t contentLength = -1;
        const char *body = NULL;
    };

    enum Status
    {
        INIT,
        PULLING,
        FINISHED,
        CANCELED,
        FAILED,
    };

    struct Info
    {
        double totleTime, nameLookupTime, connectTime, handshakeTime, pretransferTime, startTransferTime, redirectTime;

        Info()
            : totleTime(0)
            , nameLookupTime(0)
            , connectTime(0)
            , handshakeTime(0)
            , pretransferTime(0)
            , startTransferTime(0)
            , redirectTime(0)
        {
        }
    };

    class Ticket
    {
    public:
        friend class HttpSession;
        typedef void (*Deleter)(Ticket *tic);

        const Request request;
        Response response;

        ~Ticket();

        Status status()
        {
            return mStatus;
        }

        int errorCode()
        {
            return mErrorCode;
        }

        const char *errorMessage()
        {
            if (mStatus == FAILED && !mError.empty())
            {
                return mError.c_str();
            }
            else
            {
                return NULL;
            }
        }

        void *userdata()
        {
            return mUserdata;
        }

        void setUserdata(void *userdata)
        {
            mUserdata = userdata;
        }

        double startTime()
        {
            return mStartTime;
        }

        double endTime()
        {
            return mEndTime;
        }

        double duration()
        {
            if (mEndTime >= 0 && mStartTime >= 0)
            {
                return mEndTime - mStartTime;
            }
            else
            {
                return -1;
            }
        }

        const Info &info()
        {
            return mInfo;
        }

    private:
        explicit Ticket(const Request req, Deleter deleter)
            : request(req)
            , mDeleter(deleter)
        {
        }

        size_t parseResponse(char *buf, size_t size);
        size_t parseHeader(char *buf, size_t size);
        void willBeClean();
        void markAsFinished(CURLcode code, const char *msg);

        HttpSession *mSession = NULL;
        Deleter mDeleter = NULL;
        void *mUserdata = NULL;
        HttpSessionRequestListenerInterface *mListener;
        CURL *mEasyHandle = NULL;
        string mBuffer, mError;
        struct curl_slist *mRequestHeaders = NULL;
        Status mStatus = INIT;
        int mErrorCode = 0;
        mutex mMutex;
        double mStartTime = -1, mEndTime = -1;
        Info mInfo;
        bool mVerbose = false, mParseHeaderFinished = false;
    };

    explicit HttpSession(Options opt = { "", 60, false });
    ~HttpSession();
    HttpSession(const HttpSession &) = delete;
    HttpSession &operator=(const HttpSession &) = delete;

    const Options &getOptions()
    {
        return mOpt;
    }

    bool isRunning()
    {
        return mIsRunning;
    }

    shared_ptr<Ticket> request(Request &req, HttpSessionRequestListenerInterface *listener,
                               Ticket::Deleter deleter = NULL);
    int cancel(shared_ptr<Ticket> tic);
    int stop(shared_ptr<Ticket> tic);

private:
    enum MessageType
    {
        MESSAGE_TYPE_NONE,
        MESSAGE_TYPE_REQUEST,
        MESSAGE_TYPE_CANCEL,
        MESSAGE_TYPE_STOP,
        MESSAGE_TYPE_ANY = 999,
    };

    struct Message
    {
        MessageType type = MESSAGE_TYPE_NONE;
        shared_ptr<Ticket> tic;

        Message()
            : type(MESSAGE_TYPE_NONE)
            , tic(shared_ptr<Ticket>())
        {
        }

        Message(MessageType type, shared_ptr<Ticket> tic)
            : type(type)
            , tic(tic)
        {
        }

        Message *next = NULL, *prev = NULL;
    };

    class MessageQueue
    {
    public:
        MessageQueue()
            : mLast(&mRoot)
        {
        }

        void push(Message *msg)
        {
            mLast->next = msg;
            msg->prev = mLast;
            msg->next = NULL;
            mLast = msg;
        }

        Message *pop()
        {
            if (mLast == &mRoot)
            {
                return NULL;
            }
            Message *msg = mRoot.next, *next = msg->next;
            mRoot.next = next;
            if (next)
            {
                next->prev = &mRoot;
            }
            msg->next = NULL;
            msg->prev = NULL;
            if (msg == mLast)
            {
                mLast = &mRoot;
            }
            return msg;
        }

        Message *find(MessageType type, Ticket *tic)
        {
            if (mLast == &mRoot)
            {
                return NULL;
            }
            Message *msg = mRoot.next;
            while (msg)
            {
                if ((type == MESSAGE_TYPE_ANY || msg->type == type) && msg->tic.get() == tic)
                {
                    return msg;
                }
                msg = msg->next;
            }
            return NULL;
        }

        bool contains(Message *msg)
        {
            if (msg == NULL)
            {
                return false;
            }
            Message *m = mRoot.next;
            while (m)
            {
                if (msg == m)
                {
                    return true;
                }
                m = m->next;
            }
            return false;
        }

        int drop(Message *msg)
        {
            if (msg == NULL)
            {
                return -1;
            }

            if (!contains(msg))
            {
                return -1;
            }
            msg->prev->next = msg->next;
            if (msg->next)
            {
                msg->next->prev = msg->prev;
            }
            else
            {
                mLast = msg->prev;
            }
            return 0;
        }

    private:
        Message mRoot, *mLast;
    };

    int clean(Ticket *tic, bool onlyHandle = false);
    bool tryStart();
    void selectRoutine();
    void performActions();

    CURL *buildEasyHandle(const Request &req, Ticket *tic);

    MessageQueue *mMsgQueue;
    Options mOpt;
    thread *mSelectThread = NULL;
    mutex mMutex;
    CURLM *mMultiHandle = NULL;
    map<CURL *, shared_ptr<Ticket>> mEasyHandles;
    bool mIsRunning = true;

    int handleMessage(Message *msg);
    int handleRequestMessage(shared_ptr<Ticket> tic);
    int handleCancelMessage(shared_ptr<Ticket> tic);
    int handleStopMessage(shared_ptr<Ticket> tic);

    static size_t handleWriteCallback(char *buf, size_t size, size_t nmemb, void *userdata);
    static size_t handleHeaderCallback(char *buf, size_t size, size_t nitems, void *userdata);
    static int handleProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
};

class HttpSessionRequestListenerInterface
{
public:
    virtual void onRequestFinished(HttpSession *session, HttpSession::Ticket *tic, HttpSession::Response *resp) = 0;

    virtual void onRequestCanceled(HttpSession *session, HttpSession::Ticket *tic) = 0;
};

#endif // HTTPSESSION_H

#include "RKLog.hpp"

#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mutex>
#include <sys/time.h>
#include <ctime>
#include <sys/shm.h>
#include <flora-cli.h>
#include <syslog.h>

#define MAX_FILE_NAME_LEN 20

#define MIN(_A_, _B_) ((_A_) < (_B_) ? (_A_) : (_B_))
enum SocPort
{
    ACTIVATION = 15003,
    MQTT = 15005,
    LIGHT = 15006,
    TUREN = 30100
};
static const char *gTags[] = {NULL, "", "❗️", "☘", "⚠️", "❌"};
static unsigned int *gShare = NULL;

#define SHARED_MEM_RKLOG 8975

static int logPort()
{
    char *port_str = getenv("ROKID_LUA_PORT");
    if (!port_str)
    {
        return -1;
    }
    int port = atoi(port_str);
    if (port > 0)
    {
        return port;
    }
    else
    {
        return -1;
    }
}

int RKDLog::mOutputFd = 1;
#ifdef ROKID_LOG_LEVEL
int RKDLog::mLevel = ROKID_LOG_LEVEL;
#else
int RKDLog::mLevel = RKDLog::DEFAULT;
#endif

std::mutex RKDLog::mMutex;
char RKDLog::mBuffer[LOG_SIZE];
int RKDLog::mLogPort = 0;
RKLogServer *RKDLog::mLogServer = NULL;

unsigned int getLogLevel()
{
    if (gShare != NULL)
    {
        return *gShare;
    }
    else
    {
        int shmId;
        shmId = shmget(SHARED_MEM_RKLOG, 0, 0);
        if (shmId == -1)
        {
            return RKDLog::mLevel; //could not get shared buffer, use mLevel
        }
        gShare = (unsigned int *)shmat(shmId, 0, 0);
        return *gShare;
    }
}

static const char *stripFile(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }
    size_t pathLen = strlen(path);
    if (pathLen <= 0)
    {
        return NULL;
    }

    const char *fileName = strrchr(path, '/');
    if (fileName)
    {
        size_t nameLen = strlen(fileName);
        if (nameLen < MAX_FILE_NAME_LEN && pathLen > MAX_FILE_NAME_LEN)
        {
            fileName = path + pathLen - MAX_FILE_NAME_LEN;
            while (*fileName != '/')
            {
                fileName++;
            }
            if (strlen(fileName) > 1)
            {
                fileName++;
            }
        }
    }
    else
    {
        fileName = path;
    }

    return fileName;
}

static size_t dateString(char *buf, size_t len, struct timespec &time)
{
    struct tm *timeinfo = localtime(&time.tv_sec);
    return strftime(buf, len, "%m-%d %H:%M:%S", timeinfo);
}

size_t RKDLog::log(int level, const char *file, int line, const char *logtag, std::vector<char *> &args)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (level < getLogLevel())
    {
        return 0;
    }
    if (mOutputFd < 0)
    {
        return -1;
    }

    size_t len = formatHeader(level, file, line, logtag);
    if (len < 0)
    {
        return -1;
    }

    const char *arg;
    size_t l;
    for (int i = 0; i < args.size(); ++i)
    {
        if (i != 0)
        {
            mBuffer[len] = ' ';
            len++;
        }
        arg = args[i];
        l = strlen(arg);
        l = MIN(sizeof(mBuffer) - len, l);
        memcpy(mBuffer + len, arg, l);
        len += l;
        if (len >= sizeof(mBuffer))
        {
            break;
        }
    }
    return outPut(level, len, logtag);
}
RKDLog::~RKDLog()
{
    if (gShare != NULL)
    {
        shmdt(gShare);
        gShare = NULL;
    }
}
size_t RKDLog::log(int level, const char *file, int line, const char *logtag, const char *format, va_list args)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (level < getLogLevel())
    {
        return 0;
    }
    if (mOutputFd < 0)
    {
        return -1;
    }

    size_t p1, p2, len, ret;

    p1 = formatHeader(level, file, line, logtag);
    if (p1 < 0)
    {
        return -1;
    }

    p2 = vsnprintf(mBuffer + p1, sizeof(mBuffer) - p1, format, args);
    if (p2 < 0)
    {
        return -1;
    }
    len = p1 + p2;

    return outPut(level, len, logtag);
}

void RKDLog::logToServer(const char *buf, size_t len)
{
    if (buf == NULL || len <= 0)
    {
        return;
    }
    if (mLogPort < 0)
    {
        return;
    }
    if (mLogPort == 0)
    {
        mLogPort = logPort();
        if (mLogPort < 0)
        {
            return;
        }
        if (!mLogServer)
        {
            mLogServer = new RKLogServer(mLogPort);
        }
    }

    if (mLogServer && mLogServer->showLog())
    {
        mLogServer->log(buf, len);
    }
}

size_t RKDLog::formatHeader(int level, const char *file, int line, const char *logtag)
{
#define FORMAT_PART(_FORMAT_, ...) ({                                                        \
    if ((len = snprintf(mBuffer + pos, sizeof(mBuffer) - pos, (_FORMAT_), __VA_ARGS__)) < 0) \
    {                                                                                        \
        return -1;                                                                           \
    }                                                                                        \
    pos += len;                                                                              \
    len;                                                                                     \
})
    file = stripFile(file);
    size_t pos = 10, len = 0;
    switch (level)
    {
    case VERBOSE:
        memcpy(mBuffer, "[VERBOSE] ", 10);
        break;
    case DEBUG:
        memcpy(mBuffer, "[DEBUG]   ", 10);
        break;
    case INFO:
        memcpy(mBuffer, "[INFO]    ", 10);
        break;
    case WARN:
        memcpy(mBuffer, "[WARN]    ", 10);
        break;
    case ERROR:
        memcpy(mBuffer, "[ERROR]   ", 10);
        break;
    default:
        memcpy(mBuffer, "[VERBOSE] ", 10);
    }

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    pos += dateString(mBuffer + pos, sizeof(mBuffer) - pos, t);
    FORMAT_PART(".%06d", (int)(t.tv_nsec / 1e3));

    mBuffer[pos++] = ' ';

    const char *tag = (level < sizeof(gTags) / sizeof(const char *)) ? gTags[level] : "";
    if (file)
    {
        if (logtag)
        {
            FORMAT_PART("[%-20s %3d][%s]%s ", file, line, logtag, tag);
        }
        else
        {
            FORMAT_PART("[%-20s %3d]%s ", file, line, tag);
        }
    }
    else
    {
        if (logtag)
        {
            FORMAT_PART("[%s]%s ", logtag, tag);
        }
        else
        {
            FORMAT_PART("%s ", tag);
        }
    }
#undef FORMAT_PART
    return pos;
}

size_t RKDLog::outPut(int level, size_t len, const char *logtag)
{
    if (len < LOG_SIZE)
    {
        mBuffer[len] = '\n';
        len += 1;
    }
    //if log size >4k, the pipe(openwrt openwrt/logd) will be blocked, then block the process/thread, so we will drop log > LOG_SIZE for safe.
    if (len >= LOG_SIZE)
    {
        mBuffer[LOG_SIZE - 1] = '\n';
        len = LOG_SIZE;
    }
    logToServer(mBuffer, len);

    if (isatty(fileno(stdout)))
    {
        write(mOutputFd, mBuffer, len);
    }
    else
    {
        //forward log directly to logd, does not need procd.
        mBuffer[len - 1] = '\0';
        openlog(NULL, LOG_PID, 0);
        //LOG_ERR      3      RKDLog::Error    5
        //LOG_WARNING  4      RKDLog::WARN     4
        //LOG_NOTICE   5      RKDlog::INFO     3
        //LOG_INFO     6      RKDlog::DEBUG    2
        //LOG_DEBUG    7      RKDlog::VERBOSE  1
        //so syslog level = 8-rklog level
        //LOG_USER=8
        syslog((8 - level)|LOG_USER, "%s\n", mBuffer);
        closelog();
    }
    return 0;
}

//below is for c API
size_t loger(int level, const char *file, int line, const char *logtag, const char *format, va_list args)
{
    RKDLog::log(level, file, line, logtag, format, args);
    return 0;
}

extern void jslog(int level, const char *file, int line, const char *logtag, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    RKDLog::log(level, file, line, logtag, format, args);
    va_end(args);
}
extern void set_cloudlog_on(unsigned int on)
{
    return;
}

static flora_cli_t cli;
int log_flora_init()
{

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", NULL, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            syslog(LOG_ERR|LOG_USER, "log2cloud--flora connect failed, will retry later\n");
            sleep(1);
        } else {
            flora_cli_subscribe(cli, "log2cloud.on");
            break;
        }
    }

    return 0;
}

extern void set_cloudlog_on_with_auth(unsigned int on, const char *auth)
{
    log_flora_init();
    syslog(LOG_NOTICE|LOG_USER, "log2cloud--flora init done\n");
    caps_t msg = caps_create();
    caps_write_integer(msg,on);
    if(0 != on && NULL != auth)
    {
        caps_write_string(msg, auth);
    }
    flora_cli_post(cli, "log2cloud.on", msg, FLORA_MSGTYPE_INSTANT);
    syslog(LOG_NOTICE|LOG_USER, "log2cloud--flora post done\n");
    caps_destroy(msg);
    return;
}

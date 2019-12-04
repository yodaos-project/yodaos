#ifndef RKD_LOG_H
#define RKD_LOG_H
#include "RKLogServer.hpp"
#include "RKLog.h"
#include <stdarg.h>
#include <vector>


#define LOG_SIZE 3968 //1024*4-128
class RKLogServer;

class RKDLog
{

public:
    static const int VERBOSE = 1;
    static const int DEBUG = 2;
    static const int INFO = 3;
    static const int WARN = 4;
    static const int ERROR = 5;

    static const int DEFAULT = RKDLog::VERBOSE;

public:
    static int mLevel;
    static int mOutputFd;

    static size_t log(int level, const char *file, int line, const char *logtag, std::vector<char *> &args);

    static size_t log(int level, const char *file, int line, const char *logtag, const char *format, va_list args);

    static size_t log(int level, const char *file, int line, const char *logtag, const char *format, ...)
    {
        return LOG_VAR_ARGS_TAGS(level);
    }
    ~RKDLog();

private:
    static std::mutex mMutex;
    static char mBuffer[LOG_SIZE];
    static int mLogPort;
    static RKLogServer *mLogServer;
    static struct LogHeaderSegs
    {
        size_t tsStart;
        size_t tsEnd;
        size_t fileStart;
        size_t fileEnd;
        size_t lineStart;
        size_t lineEnd;
        size_t totalLen;
    } mHeaderSegs;

    static size_t formatHeader(int level, const char *file, int line, const char *logtag);
    static size_t outPut(int level, size_t len, const char *logtag);
    static void logToServer(const char *buf, size_t len);
};

#endif // RKD_LOG_H

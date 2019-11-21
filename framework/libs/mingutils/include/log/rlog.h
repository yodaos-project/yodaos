#ifndef ROKID_LOG_H
#define ROKID_LOG_H

#include <stdint.h>
#include <stdarg.h>

typedef enum {
  ROKID_LOGLEVEL_VERBOSE = 0,
  ROKID_LOGLEVEL_DEBUG,
  ROKID_LOGLEVEL_INFO,
  ROKID_LOGLEVEL_WARNING,
  ROKID_LOGLEVEL_ERROR,

  ROKID_LOGLEVEL_NUMBER,
} RokidLogLevel;

typedef enum {
  ROKID_LOGWRITER_FD = 0,
  ROKID_LOGWRITER_SOCKET
} RokidBuiltinLogWriter;

// name of endpoint is duplicated
#define RLOG_EDUP -1
// name of endpoint not found
#define RLOG_ENOTFOUND -2
// endpoint already enabled
#define RLOG_EALREADY -3
// writer init failed
#define RLOG_EFAULT -4
// invalid arguments
#define RLOG_EINVAL -5

#ifdef __cplusplus
class RLogWriter {
public:
  virtual ~RLogWriter() = default;

  virtual bool init(void* arg) = 0;

  virtual void destroy() = 0;

  virtual bool write(const char *data, uint32_t size) = 0;

  /// \return  0  should call 'write'
  //           1  don't call 'write'
  //           -1 error, will not call 'write'
  virtual int32_t raw_write(const char* file, int line, RokidLogLevel lv,
      const char* tag, const char* fmt, va_list ap) {
    return 0;
  }
};

class RLog {
public:
  static void print(const char *file, int line, RokidLogLevel lv,
                    const char* tag, const char* fmt, ...);

  static int32_t add_endpoint(const char* name, RLogWriter* writer);

  static int32_t add_endpoint(const char *name, RokidBuiltinLogWriter type);

  static void remove_endpoint(const char* name);

  static int32_t enable_endpoint(const char* name, void* init_arg, bool enable);
};

extern "C" {
#endif

// built-in writer
// "std"

typedef int32_t (*RokidLogInit)(void *arg, void *init_arg);
typedef void (*RokidLogDestroy)(void *);
typedef int32_t (*RokidLogWrite)(const char *, uint32_t, void *);
typedef struct {
  RokidLogInit init;
  RokidLogDestroy destroy;
  RokidLogWrite write;
} RokidLogWriter;

#ifdef __ANDROID__
void android_log_print(const char *file, int line, RokidLogLevel lv,
                     const char* tag, const char* fmt, ...);
#else
void rokid_log_print(const char *file, int line, RokidLogLevel lv,
                     const char* tag, const char* fmt, ...);
#endif

int32_t rokid_log_add_endpoint(const char *name, RokidLogWriter *writer, void *arg);

int32_t rokid_log_add_builtin_endpoint(const char *name, RokidBuiltinLogWriter type);

void rokid_log_remove_endpoint(const char *name);

int32_t rokid_log_enable_endpoint(const char *name, void *init_arg, bool enable);

#ifdef __cplusplus
} // extern "C"
#endif

#ifndef ROKID_LOG_ENABLED
#define ROKID_LOG_ENABLED 2
#endif

#ifdef __ANDROID__
#define RLOG_PRINT(lv, tag, fmt, ...)  android_log_print(__FILE__, __LINE__, lv, tag, fmt, ##__VA_ARGS__)
#else
#ifdef __cplusplus
#define RLOG_PRINT(lv, tag, fmt, ...)  RLog::print(__FILE__, __LINE__, lv, tag, fmt, ##__VA_ARGS__)
#else
#define RLOG_PRINT(lv, tag, fmt, ...)  rokid_log_print(__FILE__, __LINE__, lv, tag, fmt, ##__VA_ARGS__)
#endif // __cplusplus
#endif // __ANDROID__

#if ROKID_LOG_ENABLED <= 0
#define KLOGV(tag, fmt, ...) RLOG_PRINT(ROKID_LOGLEVEL_VERBOSE, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGV(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 1
#define KLOGD(tag, fmt, ...) RLOG_PRINT(ROKID_LOGLEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGD(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 2
#define KLOGI(tag, fmt, ...) RLOG_PRINT(ROKID_LOGLEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGI(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 3
#define KLOGW(tag, fmt, ...) RLOG_PRINT(ROKID_LOGLEVEL_WARNING, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGW(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 4
#define KLOGE(tag, fmt, ...) RLOG_PRINT(ROKID_LOGLEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGE(tag, fmt, ...)
#endif

#endif // ROKID_LOG_H

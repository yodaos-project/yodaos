#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <string>

#define ROKID_MAX_ERROR_MSG_SIZE 1024

namespace rokid {

class GlobalError {
public:
  static void set(const char* tag, int32_t code, const char* file, int line,
      const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    set(tag, code, file, line, fmt, ap);
    va_end(ap);
  }

  static void set(const char* tag, int32_t code, const char* file, int line,
      const char* fmt, va_list ap);

  static inline int32_t code() { return errCode; }

  static inline const std::string& tag() { return errTag; }

  static inline const char* msg() { return errMsg; }

public:
  static thread_local int32_t errCode;
  static thread_local std::string errTag;
  static thread_local char errMsg[ROKID_MAX_ERROR_MSG_SIZE];
};

} // namespace rokid

#define ROKID_GERROR(tag, code, fmt, ...) rokid::GlobalError::set(tag, code, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ROKID_GERROR_STRING rokid::GlobalError::msg()

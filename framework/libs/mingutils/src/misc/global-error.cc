#include <stdio.h>
#include <libgen.h>
#include "global-error.h"

namespace rokid {

thread_local int32_t GlobalError::errCode{0};
thread_local std::string GlobalError::errTag;
thread_local char GlobalError::errMsg[ROKID_MAX_ERROR_MSG_SIZE]{0};

void GlobalError::set(const char* tag, int32_t code, const char* file,
    int line, const char* fmt, va_list ap) {
  if (tag)
    errTag = tag;
  errCode = code;
  auto bn = basename((char*)file);
  auto c = snprintf(errMsg, sizeof(errMsg), "<%s>[%d](%s:%d) ", errTag.c_str(), code, bn, line);
  vsnprintf(errMsg + c, sizeof(errMsg) - c, fmt, ap);
}

} // namespace rokid

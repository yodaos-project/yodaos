#pragma once

#include <unistd.h>
#include "rlog.h"

class FileDescWriter : public RLogWriter {
public:
  virtual bool init(void* arg) {
    fd = reinterpret_cast<intptr_t>(arg);
    return true;
  }

  virtual void destroy() {
    fd = -1;
  }

  virtual bool write(const char *data, uint32_t size) {
    int32_t r = ::write(fd, data, size);
    if (r > 0)
      fsync(fd);
    return r > 0;
  }

private:
  int fd = -1;
};

#pragma once

#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

class StringPool {
public:
  StringPool(uint32_t size) : capacity(size) {
    buffer = (char*)mmap(nullptr, size, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  }

  ~StringPool() {
    munmap(buffer, capacity);
  }

  uint32_t push(const char* p, uint32_t size) {
    char* d = buffer + buf_offset;
    uint32_t r = buf_offset;
    if (size)
      memcpy(d, p, size);
    d[size] = '\0';
    buf_offset += size + 1;
    return r;
  }

  const char* get(uint32_t idx) const {
    if (idx == 0 || idx >= buf_offset)
      return nullptr;
    return buffer + idx;
  }

private:
  char* buffer;
  uint32_t buf_offset{1};
  uint32_t capacity;
};

#pragma once

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <string>

#define MIN_MEM_SIZE sizeof(ControlData)

namespace rokid {

class CircleStream {
private:
  class ControlData {
  public:
    uint32_t front;
    uint32_t back;
    uint32_t space_size;
    uint32_t unused;
  };

public:
  bool create(void* mem, uint32_t size) {
    // already initialized
    if (_control)
      return false;
    // invalid args
    if (mem == nullptr || size < MIN_MEM_SIZE)
      return false;
    _control = reinterpret_cast<ControlData*>(mem);
    _remain_size = size - sizeof(ControlData);
    _begin = reinterpret_cast<uint8_t*>(mem) + sizeof(ControlData);
    _end = _begin + _remain_size;
    _control->space_size = _remain_size;
    _control->front = 0;
    _control->back = 0;
    return true;
  }

  void close() {
    _remain_size = 0;
    _control = nullptr;
  }

  inline uint32_t free_space() const {
    return _remain_size;
  }

  inline uint32_t capacity() const {
    return _control ? _control->space_size : 0;
  }

  uint32_t size() const {
    return _control ? _control->space_size - _remain_size : 0;
  }

  // write data to stream
  int32_t write(const void* data, uint32_t length) {
    if (_control == nullptr)
      return -1;
    if (length == 0 || _remain_size == 0)
      return 0;
    // circle stream, if write reach memory pointer end,
    // jump to memory pointer begin, write remain data.
    uint8_t *bp = _begin + _control->back;
    uint32_t csz = _end - bp;
    uint32_t wlen;
    if (csz >= length) {
      if (_remain_size >= length)
        wlen = length;
      else
        wlen = _remain_size;
      memcpy(bp, data, wlen);
      _control->back += wlen;
    } else {
      uint32_t sz1 = csz;
      uint32_t sz2 = length - sz1;
      uint32_t sz3;
      memcpy(bp, data, sz1);
      if (_control->front >= sz2)
        sz3 = sz2;
      else
        sz3 = _control->front;
      memcpy(_begin, reinterpret_cast<const uint8_t*>(data) + sz1, sz3);
      _control->back = sz3;
      wlen = sz1 + sz3;
    }
    _remain_size -= wlen;
    return wlen;
  }

  // return actual length of the data
  // or zero if no data available
  uint32_t read(void *buf, uint32_t bufsize) {
    if (empty())
      return 0;
    uint32_t total_size = _control->space_size - _remain_size;
    uint32_t outsz = total_size <= bufsize ? total_size : bufsize;
    char *bp = reinterpret_cast<char *>(_begin + _control->front);
    uint32_t csz = _end - reinterpret_cast<uint8_t*>(bp);
    if (buf) {
      if (outsz <= csz) {
        memcpy(buf, bp, outsz);
      } else {
        // 不是连续内存块，把两部分数据拼接起来
        uint32_t sz1 = csz;
        uint32_t sz2 = outsz - csz;
        memcpy(buf, bp, sz1);
        memcpy(reinterpret_cast<char *>(buf) + sz1, _begin, sz2);
      }
    }
    _remain_size += outsz;
    if (_remain_size == _control->space_size) {
      _control->front = 0;
      _control->back = 0;
    } else {
      if (outsz < csz)
        _control->front += outsz;
      else
        _control->front = outsz - csz;
    }
    return outsz;
  }

  void* peek(uint32_t& sz) {
    uint32_t count;
    if (empty() || (count = size()) == 0) {
      sz = 0;
      return nullptr;
    }
    uint8_t *bp = _begin + _control->front;
    uint32_t csz = _end - bp;
    if (csz == 0) {
      bp = _begin;
      csz = _control->space_size;
    }
    if (count <= csz)
      sz = count;
    else
      sz = csz;
    return bp;
  }

  // return erased data length
  uint32_t erase(uint32_t size) {
    return read(nullptr, size);
  }

  void clear() {
    if (_control) {
      _control->front = 0;
      _control->back = 0;
      _remain_size = _control->space_size;
    }
  }

  bool empty() const {
    return _control == nullptr || _control->space_size == _remain_size;
  }

  inline uint32_t header_size() const { return sizeof(ControlData); }

private:
  // memory adress of begin of queue data space
  uint8_t *_begin = nullptr;
  uint8_t *_end = nullptr;
  ControlData *_control = nullptr;
  uint32_t _remain_size = 0;
};

class MmapCircleStream : public CircleStream {
public:
  ~MmapCircleStream() { close(); }

  bool create(uint32_t size) {
    void* p = mmap(nullptr, size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == nullptr)
      return false;
    auto r = CircleStream::create(p, size);
    if (!r) {
      munmap(p, size);
      return false;
    }
    map_mem = p;
    map_size = size;
    return true;
  }

  void close() {
    if (map_mem) {
      munmap(map_mem, map_size);
      map_mem = nullptr;
      map_size = 0;
    }
  }

private:
  void* map_mem{nullptr};
  uint32_t map_size{0};
};

} // namespace rokid

#pragma once

#include <stdint.h>
#include <string>

namespace rokid {
namespace queue {

typedef struct {
  uint32_t front;
  uint32_t back;
  uint32_t remain_size;
  uint32_t continuous:1;
  uint32_t write_counter:31;
} ControlData;

class VariableQueue {
public:
  VariableQueue();

  // continuous = true
  //   写入的数据块必须存储于连续内存中
  // continuous = false
  //   写入的数据块不必存储于连续内存
  //   可能一部分在内存尾端，一部分在内存首端
  //   此种模式下peek函数不可用, 将返回nullptr
  void create(void* mem, uint32_t size, bool continuous = false);

  bool reuse(void* mem, uint32_t size);

  void close();

  inline uint32_t free_space() const {
    return _control->remain_size;
  }

  uint32_t capacity() const;

  // write a data record to queue
  bool write(const void* data, uint32_t length);

  // read a data record from queue
  // return actual length of the data record
  //        or zero if no data record available
  uint32_t read(std::string& result);

  void* peek(uint32_t* size) const;

  // return erased data length
  uint32_t erase();

  void clear();

  bool is_continuous() const;

#ifdef ROKID_DEBUG
  inline void set_name(const std::string& name) { _name = name; }

  void print_state();
#endif

private:
  // memory adress of begin of queue data space
  uint8_t* _begin;
  uint8_t* _end;
  ControlData* _control;
  uint32_t _space_size;
#ifdef ROKID_DEBUG
  std::string _name;
#endif
};


} // namespace queue
} // namespace rokid

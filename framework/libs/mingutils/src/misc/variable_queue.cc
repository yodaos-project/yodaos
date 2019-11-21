#include <assert.h>
#include <string.h>
#include "variable_queue.h"
#ifdef ROKID_DEBUG
#include "rlog.h"
#define LOG_TAG "rkutils.VariableQueue"
#endif

using std::string;

namespace rokid {
namespace queue {

typedef struct {
  uint32_t pad:1;
  uint32_t size:31;
} DataHeader;

static const uint32_t HEADER_SIZE = sizeof(DataHeader);
static const uint32_t MIN_MEM_SIZE = sizeof(ControlData);

static inline uint32_t pack_size(uint32_t sz) {
  return (sz + 3) & ~3;
}

VariableQueue::VariableQueue()
    : _begin(nullptr)
    , _end(nullptr)
    , _control(nullptr)
    , _space_size(0) {
}

void VariableQueue::create(void* mem, uint32_t size, bool continuous) {
  if (!reuse(mem, size))
    return;
  _control->front = 0;
  _control->back = 0;
  _control->remain_size = _space_size;
  _control->continuous = continuous ? 1 : 0;
  _control->write_counter = 0;
#ifdef ROKID_DEBUG
  KLOGD(LOG_TAG, "[%s]: varq.create clear all data", _name.c_str());
#endif
}

bool VariableQueue::reuse(void* mem, uint32_t size) {
  // already initialized
  if (_space_size > 0)
    return false;
  // invalid args
  if (mem == nullptr || size < MIN_MEM_SIZE)
    return false;
  _control = reinterpret_cast<ControlData*>(mem);
  _space_size = (size - sizeof(ControlData));
  _begin = reinterpret_cast<uint8_t*>(mem) + sizeof(ControlData);
  _end = _begin + _space_size;
  return true;
}

void VariableQueue::close() {
  _space_size = 0;
  _control = nullptr;
}

uint32_t VariableQueue::capacity() const {
  return _space_size - HEADER_SIZE;
}

bool VariableQueue::write(const void* data, uint32_t length) {
  if (_control == nullptr || _control->remain_size < length + HEADER_SIZE)
    return false;
  // write HEADER (data length) + data
  // circle queue, if write reach memory pointer end,
  // jump to memory pointer begin, write remain data.
  uint32_t rsz = pack_size(length + HEADER_SIZE);
  uint8_t* bp = _begin + _control->back;
  uint32_t csz = _end - bp;
  DataHeader* header;
  assert(csz > 0 && (csz & 3) == 0);
  if (csz >= rsz) {
    header = reinterpret_cast<DataHeader*>(bp);
    header->pad = 0;
    header->size = length;
    memcpy(bp + HEADER_SIZE, data, length);
    if (csz == rsz)
      _control->back = 0;
    else
      _control->back += rsz;
  } else {
    // 如果插入的数据必须要连续内存中保存
    // 且尾端已经没有足够大的连续内存保存当前插入的数据块
    // 需要在尾端插入一个占位空数据块
    // 再在首端插入需要保存的数据块
    if (_control->continuous) {
      assert(_control->back >= _control->front);
      uint32_t hcsz = _control->front;
      // 首端没有足够的空间保存插入的数据块
      if (hcsz < rsz) {
        return false;
      }
      // 插入占位数据块，占据尾端剩余空间
      header = reinterpret_cast<DataHeader*>(bp);
      header->pad = 1;
      header->size = csz - HEADER_SIZE;
      _control->remain_size -= csz;
      // 插入数据至内存块首端
      header = reinterpret_cast<DataHeader*>(_begin);
      header->pad = 0;
      header->size = length;
      memcpy(_begin + HEADER_SIZE, data, length);
      _control->back = rsz;
    } else {
      header = reinterpret_cast<DataHeader*>(bp);
      header->pad = 0;
      header->size = length;
      uint32_t sz1 = csz - HEADER_SIZE;
      uint32_t sz2 = length - sz1;
      if (sz1 > 0)
        memcpy(bp + HEADER_SIZE, data, sz1);
      assert(sz2 > 0);
      memcpy(_begin, reinterpret_cast<const uint8_t*>(data) + sz1, sz2);
      _control->back = pack_size(sz2);
    }
  }
#ifdef ROKID_DEBUG
  KLOGD(LOG_TAG, "[%s]: varq.write %u bytes success, front=%u, back=%u",
      _name.c_str(), rsz, _control->front, _control->back);
#endif
  _control->remain_size -= rsz;
  ++_control->write_counter;
  return true;
}

uint32_t VariableQueue::read(string& result) {
  if (_control == nullptr || _space_size == _control->remain_size)
    return 0;
  DataHeader* header = reinterpret_cast<DataHeader*>(_begin + _control->front);
  uint32_t total_size = pack_size(header->size + HEADER_SIZE);
#ifdef ROKID_DEBUG
  KLOGD(LOG_TAG, "[%s]: varq.read data size %u, total size %u, front=%u, back=%u",
      _name.c_str(), header->size, total_size, _control->front, _control->back);
#endif
  assert(total_size <= _space_size - _control->remain_size);
  uint32_t outsz = header->size;
  char* bp = reinterpret_cast<char*>(header) + HEADER_SIZE;
  if (_control->continuous) {
    // 读到占位块，必定在尾端。
    // 忽略此块数据，从首端开始读取
    if (header->pad) {
      assert(_control->front + total_size == _space_size);
      _control->front = 0;
      _control->remain_size += total_size;
      return read(result);
    }
    // 必然是连续内存块，不用做检查
    result.assign(bp, outsz);
    _control->front += total_size;
    if (_control->front == _space_size)
      _control->front = 0;
  } else {
    uint32_t csz = _end - reinterpret_cast<uint8_t*>(bp);
    if (outsz <= csz) {
      result.assign(bp, outsz);
    } else {
      // 不是连续内存块，把两部分数据拼接起来放入'result'
      uint32_t sz1 = csz;
      uint32_t sz2 = outsz - csz;
      result.assign(bp, sz1);
      result.append(reinterpret_cast<char*>(_begin), sz2);
    }
    csz = _space_size - _control->front;
    if (total_size < csz)
      _control->front += total_size;
    else
      _control->front = total_size - csz;
  }
  _control->remain_size += total_size;
#ifdef ROKID_DEBUG
  KLOGD(LOG_TAG, "[%s]: varq.read %u bytes success, front=%u, back=%u",
      _name.c_str(), total_size, _control->front, _control->back);
#endif
  return outsz;
}

void* VariableQueue::peek(uint32_t* size) const {
  if (_control == nullptr || _control->continuous == 0 || _space_size == _control->remain_size) {
    if (size)
      *size = 0;
    return nullptr;
  }
  DataHeader* header = reinterpret_cast<DataHeader*>(_begin + _control->front);
  assert(header->size + HEADER_SIZE <= _space_size - _control->remain_size);
  if (header->pad) {
    assert(_control->front + header->size + HEADER_SIZE == _space_size);
    header = reinterpret_cast<DataHeader*>(_begin);
  }
  if (size)
    *size = header->size;
  return header + 1;
}

uint32_t VariableQueue::erase() {
  if (_control == nullptr || _space_size == _control->remain_size)
    return 0;
  DataHeader* header = reinterpret_cast<DataHeader*>(_begin + _control->front);
  uint32_t total_size = pack_size(header->size + HEADER_SIZE);
#ifdef ROKID_DEBUG
  KLOGD(LOG_TAG, "[%s]: varq.erase data size %u, total size %u, front=%u, back=%u",
      _name.c_str(), header->size, total_size, _control->front, _control->back);
#endif
  assert(total_size <= _space_size - _control->remain_size);
  uint32_t outsz = header->size;
  if (_control->continuous) {
    // 略过占位数据块
    if (header->pad) {
      assert(_control->front + total_size == _space_size);
      _control->front = 0;
      _control->remain_size += total_size;
      return erase();
    }
    _control->front += total_size;
    if (_control->front == _space_size)
      _control->front = 0;
  } else {
    uint32_t csz = _space_size - _control->front;
    if (total_size < csz)
      _control->front += total_size;
    else
      _control->front = total_size - csz;
  }
  _control->remain_size += total_size;
#ifdef ROKID_DEBUG
  KLOGD(LOG_TAG, "[%s]: varq.erase %u bytes success, front=%u, back=%u",
      _name.c_str(), total_size, _control->front, _control->back);
#endif
  return outsz;
}

void VariableQueue::clear() {
  if (_control) {
    _control->front = 0;
    _control->back = 0;
    _control->remain_size = _space_size;
    _control->continuous = 0;
    _control->write_counter = 0;
  }
}

bool VariableQueue::is_continuous() const {
  if (_control == nullptr)
    return false;
  return _control->continuous;
}

#ifdef ROKID_DEBUG
void VariableQueue::print_state() {
  if (_control == nullptr) {
    KLOGD(LOG_TAG, "VariableQueue.print_state: queue not initialized");
    return;
  }
  KLOGD(LOG_TAG, "[%s]: front = %u, back = %u, remain = %u, write counter = %u",
      _name.c_str(), _control->front, _control->back,
      _control->remain_size, _control->write_counter);
}
#endif

} // namespace queue
} // namespace rokid

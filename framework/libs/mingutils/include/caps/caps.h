#ifndef ROKID_MUTILS_CAPS_H
#define ROKID_MUTILS_CAPS_H

#include <stdint.h>

#define CAPS_VERSION 4

#define CAPS_SUCCESS 0
#define CAPS_ERR_INVAL -1  // 参数非法
#define CAPS_ERR_CORRUPTED -2  // parse数据格式不正确
#define CAPS_ERR_VERSION_UNSUPP -3  // 不支持parse数据的版本(数据版本高于反序列化工具版本)
#define CAPS_ERR_WRONLY -4  // read的caps对象不可读(通过caps_create创建的caps对象只写)
#define CAPS_ERR_RDONLY -5  // write的caps对象不可写(通过caps_parse创建的caps对象只读)
#define CAPS_ERR_INCORRECT_TYPE -6  // read类型不匹配
#define CAPS_ERR_EOO -7  // 没有更多的成员变量了，read结束(End of Object)

#define CAPS_TYPE_WRITER 0
#define CAPS_TYPE_READER 1

#define CAPS_MEMBER_TYPE_INTEGER 'i'
#define CAPS_MEMBER_TYPE_FLOAT 'f'
#define CAPS_MEMBER_TYPE_LONG 'l'
#define CAPS_MEMBER_TYPE_DOUBLE 'd'
#define CAPS_MEMBER_TYPE_STRING 'S'
#define CAPS_MEMBER_TYPE_BINARY 'B'
#define CAPS_MEMBER_TYPE_OBJECT 'O'
#define CAPS_MEMBER_TYPE_VOID 'V'

#define CAPS_FLAG_NET_BYTEORDER 0x80

typedef intptr_t caps_t;

#ifdef __cplusplus
#include <memory>
#include <string>
#include <vector>

class Caps {
public:
  virtual ~Caps() = default;

  virtual int32_t write(int32_t v) = 0;
  virtual int32_t write(uint32_t v) = 0;
  virtual int32_t write(float v) = 0;
  virtual int32_t write(int64_t v) = 0;
  virtual int32_t write(uint64_t v) = 0;
  virtual int32_t write(double v) = 0;
  virtual int32_t write(const char* v) = 0;
  virtual int32_t write(const std::string& v) = 0;
  virtual int32_t write(const void* v, uint32_t len) = 0;
  // write binary data
  virtual int32_t write(const std::vector<uint8_t>& v) = 0;
  virtual int32_t write(std::shared_ptr<Caps>& v) = 0;
  virtual int32_t serialize(void* buf, uint32_t size,
      uint32_t flags = CAPS_FLAG_NET_BYTEORDER) const = 0;

  virtual int32_t read(int32_t& v) = 0;
  virtual int32_t read(uint32_t& v) = 0;
  virtual int32_t read(float& v) = 0;
  virtual int32_t read(int64_t& v) = 0;
  virtual int32_t read(uint64_t& v) = 0;
  virtual int32_t read(double& v) = 0;
  virtual int32_t read(const char*& r) = 0;
  virtual int32_t read(std::string& r) = 0;
  // read binary data
  virtual int32_t read(const void*& r, uint32_t& size) = 0;
  virtual int32_t read(std::vector<uint8_t>& r) = 0;
  // deprecated, replaced by read(std::string&)
  virtual int32_t read_string(std::string& r) = 0;
  // deprecated, replaced by read(std::vector<uint8_t>&)
  virtual int32_t read_binary(std::string& r) = 0;
  virtual int32_t read(std::shared_ptr<Caps>& v) = 0;
  virtual int32_t next_type() const = 0;

  virtual int32_t type() const = 0;
  virtual uint32_t binary_size() const = 0;
  virtual uint32_t size() const = 0;

  // read/write void type
  virtual int32_t write() = 0;
  virtual int32_t read() = 0;

  // create WRONLY Caps
  static std::shared_ptr<Caps> new_instance();

  // create RDONLY Caps
  static int32_t parse(const void* data, uint32_t length,
      std::shared_ptr<Caps>& caps, bool duplicate = true);

  // 同c api: caps_binary_info
  static int32_t binary_info(const void* data, uint32_t* version, uint32_t* length);

  static std::shared_ptr<Caps> convert(caps_t caps);

  static caps_t convert(std::shared_ptr<Caps>& caps);
};

extern "C" {
#endif

// create创建的caps_t对象可写，不可读
caps_t caps_create();

// parse创建的caps_t对象可读，不可写
int32_t caps_parse(const void* data, uint32_t length, caps_t* result);

// 如果serialize生成的数据长度大于'bufsize'，将返回所需的buf size，
// 但'buf'不会写入任何数据，需外部重新分配更大的buf，再次调用serialize
int32_t caps_serialize(caps_t caps, void* buf, uint32_t bufsize);

int32_t caps_write_integer(caps_t caps, int32_t v);

int32_t caps_write_long(caps_t caps, int64_t v);

int32_t caps_write_float(caps_t caps, float v);

int32_t caps_write_double(caps_t caps, double v);

int32_t caps_write_string(caps_t caps, const char* v);

int32_t caps_write_binary(caps_t caps, const void* data, uint32_t length);

int32_t caps_write_object(caps_t caps, caps_t v);

int32_t caps_write_void(caps_t caps);

int32_t caps_read_integer(caps_t caps, int32_t* r);

int32_t caps_read_long(caps_t caps, int64_t* r);

int32_t caps_read_float(caps_t caps, float* r);

int32_t caps_read_double(caps_t caps, double* r);

int32_t caps_read_string(caps_t caps, const char** r);

int32_t caps_read_binary(caps_t caps, const void** r, uint32_t* length);

int32_t caps_read_object(caps_t caps, caps_t* r);

int32_t caps_read_void(caps_t caps);

void caps_destroy(caps_t caps);

// 'data' 必须不少于8字节
// 根据8字节信息，得到整个二进制数据长度及caps版本
int32_t caps_binary_info(const void* data, uint32_t* version, uint32_t* length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ROKID_MUTILS_CAPS_H

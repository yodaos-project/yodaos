#ifndef ROKID_COMMAND_LINE_ARGS_H
#define ROKID_COMMAND_LINE_ARGS_H

#include <stdint.h>

#ifdef __cplusplus
#include <memory>

class CLPair {
public:
  const char* key;

  const char* value;

  bool match(const char* key) const;

  bool to_integer(int32_t& res) const;
};

class CLArgs {
public:
  static std::shared_ptr<CLArgs> parse(int32_t argc, char** argv);

  virtual uint32_t size() const = 0;

  virtual const CLPair& at(uint32_t idx, CLPair &res) const = 0;

  virtual bool find(const char* key, uint32_t *begin, uint32_t *end) const = 0;
};

extern "C" {
#endif

typedef intptr_t clargs_h;

// 解析命令行参数
// 成功返回句柄，失败返回0
clargs_h clargs_parse(int32_t argc, char** argv);

// 销毁句柄
void clargs_destroy(clargs_h handle);

// 获取命令行参数对数量
// clargs_get下标参数idx必须小于本函数返回的值
uint32_t clargs_size(clargs_h handle);

// 获取指定下标的命令行参数key/value
// 成功返回0，下标idx超出范围则返回-1
int32_t clargs_get(clargs_h handle, uint32_t idx, const char** key, const char** value);

// 获取指定下标的命令行参数key/value
// value转为整数值
// 成功返回0，下标idx超出范围则返回-1，value字符串无法转为合法整数值则返回-2
int32_t clargs_get_integer(clargs_h handle, uint32_t idx, const char** key, int32_t* res);

#ifdef __cplusplus
}
#endif

#endif // ROKID_COMMAND_LINE_ARGS_H

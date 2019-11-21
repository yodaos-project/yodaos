# 序列化工具Caps

版本: 1

作者: chen.zhang@rokid.com

更新时间: 2018.08.08 17:09

## 概述

* 支持顺序读写的序列化反序列化工具
* c/c++接口实现
* 不支持线程安全

**c++接口序列化示例**

```
char data[4] = { 0, 1, 2, 3};
char odata[256];
int32_t len;

shared_ptr<Caps> obj = Caps::new_instance();
obj->write((int32_t)100);
obj->write((int64_t)10000);
obj->write("hello world");
obj->write(data, sizeof(data));
shared_ptr<Caps> subobj = Caps::new_instance();
subobj->write(0.0f);
obj->write(subobj);
len = obj->serialize(odata, sizeof(odata);
```

**c接口序列化示例**

```
caps_t obj = caps_create();
char data[4] = { 0, 1, 2, 3 };
char odata[256];
int32_t len;

caps_write_integer(obj, 100);
caps_write_long(obj, 10000);
caps_write_string(obj, "hello world");
caps_write_binary(obj, data, sizeof(data));
caps_t subobj = caps_create();
caps_write_float(subobj, 0.0f);
caps_write_object(obj, subobj);
len = caps_serialize(obj, odata, sizeof(odata));
caps_destroy(subobj);
caps_destroy(obj);
```

**c++接口反序列化示例**

```
shared_ptr<Caps> obj;
int32_t i;
int64_t l;
string str;
string data;
shared_ptr<Caps> subobj;
float f;

if (Caps::parse(odata, len, obj) != CAPS_SUCCESS) {
    // TODO: 错误处理
}
obj->read(i);
obj->read(l);
obj->read_string(str);
obj->read_binary(data);
obj->read(subobj);
subobj->read(f);
```

**c接口反序列化示例**

```
caps_t obj;
int32_t i;
int64_t l;
const char* str;
const void* data;
uint32_t data_len;
caps_t subobj;
float f;

if (caps_parse(odata, len, &obj) != CAPS_SUCCESS) {
    // TODO: 错误处理
}
caps_read_integer(obj, &i);
caps_read_long(obj, &l);
caps_read_string(obj, &str);
caps_read_binary(obj, &data, &data_len);
caps_read_object(obj, &subobj);
caps_read_float(subobj, &f);
caps_destroy(subobj);
caps_destroy(obj);
```

## 接口定义

### c++ Caps类静态成员函数

名称 | 描述 | 返回类型 | | 参数 | |
--- | --- | --- | --- | --- | ---
new_instance | 创建对象，生成的对象只写 | shared_ptr\<Caps> | | | |
parse | 反序列化，生成的对象只读 | int32_t | [错误码](#anchor13) | void* | 二进制数据(由caps_serialize生成的合法数据)
 | | | | uint32 | 数据长度
 | | | | shared_ptr\<Caps>& | 生成的caps对象

### c++ Caps类非静态成员函数

名称 | 描述 | 返回类型 | | 参数 | |
--- | --- | --- | --- | --- | ---
serialize | 序列化 | int32 | 序列化产生的数据长度或错误码 | void* | 序列化生成数据存储区
 | | | | uint32 | 存储区长度
type | 获取caps类型 | int32 | [caps类型](#anchor14) | |
binary_size | 获取caps二进制数据长度 | uint32 | 数据长度 | |
write | 向对象添加成员 | int32 | [错误码](#anchor13) | int32 | 向对象添加的整数值
write | 向对象添加成员 | int32 | [错误码](#anchor13) | int64 | 向对象添加的整数值
write | 向对象添加成员 | int32 | [错误码](#anchor13) | float | 向对象添加的浮点值
write | 向对象添加成员 | int32 | [错误码](#anchor13) | double | 向对象添加的浮点值
write | 向对象添加成员 | int32 | [错误码](#anchor13) | char* | 向对象添加的字符串
write | 向对象添加成员 | int32 | [错误码](#anchor13) | void* | 向对象添加的二进制数据
 | | | | uint32 | 数据长度
write | 向对象添加成员 | int32 | [错误码](#anchor13) | shared_ptr\<Caps> | 向caps对象添加的caps子对象
read | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | int32_t& | 读取到的值
read | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | int64_t& | 读取到的值
read | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | float& | 读取到的值
read | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | double& | 读取到的值
read\_string | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | std::string& | 读取到的值
read\_binary | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | std::string& | 读取到的值
read | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | shared_ptr\<Caps>& | 读取到的子对象

### c接口

名称 | 描述 | 返回类型 | | 参数 | |
--- | --- | --- | --- | --- | ---
caps_create | 创建对象，生成的对象只写 | caps_t | caps对象 | | |
caps_parse | 反序列化，生成的对象只读 | int32_t | [错误码](#anchor13) | void* | 二进制数据(由caps_serialize生成的合法数据)
 | | | | uint32 | 数据长度
 | | | | caps_t* | 生成的caps对象
caps_serialize | 序列化 | int32 | 序列化产生的数据长度或错误码 | caps_t | caps对象
 | | | | void* | 序列化生成数据存储区
 | | | | uint32 | 存储区长度
caps\_write\_integer | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | int32 | 向对象添加的整数值
caps\_write\_long | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | int64 | 向对象添加的整数值
caps\_write\_float | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | float | 向对象添加的浮点值
caps\_write\_double | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | double | 向对象添加的浮点值
caps\_write\_string | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | char* | 向对象添加的字符串
caps\_write\_binary | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | void* | 向对象添加的二进制数据
 | | | | uint32 | 数据长度
caps\_write\_object | 向对象添加成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | caps_t | 向caps对象添加的caps子对象
caps\_read\_integer | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | int32_t* | 读取到的值
caps\_read\_long | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | int64_t* | 读取到的值
caps\_read\_float | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | float* | 读取到的值
caps\_read\_double | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | double* | 读取到的值
caps\_read\_string | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | char** | 读取到的值
caps\_read\_binary | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | void** | 读取到的值
 | | | | uint32_t* | 读取到的数据长度
caps\_read\_object | 按顺序读取对象成员 | int32 | [错误码](#anchor13) | caps_t | caps对象
 | | | | caps_t* | 读取到的子对象
caps_destroy | 销毁对象 | void | | caps_t | caps对象

### <a id="anchor13"></a>错误码

名称 | 值 | 描述
--- | --- | ---
SUCCESS | 0 | 成功
INVAL | -1 | 参数非法
CORRUPTED | -2 | 反序列化二进制数据格式有误
VERSION_UNSUPP | -3 | 反序列化二进制数据版本号高于反序列化工具版本
WRONLY | -4 | caps对象只写
RDONLY | -5 | caps对象只读
INCORRECT_TYPE | -6 | caps读取当前值时类型不匹配
EOO | -7 | 读取到对象末尾了

### <a id="anchor14"></a>caps类型

名称 | 值 | 描述
--- | --- | ---
CAPS\_TYPE\_WRITER | 0 | 只能用于调用write方法
CAPS\_TYPE\_READER | 1 | 只能用于调用read方法

## 数据结构

### Object

类型 | 描述
--- | ---
[Header](#anchor03) | 头部数据结构
[MemberDeclaration](#anchor04) | 成员变量声明 - 4字节对齐
[NumberSection](#anchor05) | 32位整数或浮点数区 - 4字节对齐
[StringInfoSection](#anchor08) | 字符串信息区 - 4字节对齐
[BinaryInfoSection](#anchor09) | 二进制数据信息区 - 4字节对齐
[LongDataSection](#anchor06) | long及double数据值区 - 8字节对齐
[BinarySection](#anchor10) | 二进制数据区 - 4字节对齐(每一段数据)
[StringSection](#anchor07) | 字符串区 - 无对齐

### <a id="anchor03"></a>Header

类型 | 描述
--- | ---
uint32 | 魔数及版本
uint32 | 总长度(字节)
uint16 | [NumberSection](#anchor05)偏移量
uint16 | [LongDataSection](#anchor06)偏移量
uint16 | [StringInfoSection](#anchor08)偏移量
uint16 | [BinaryInfoSection](#anchor12)偏移量
uint32 | [StringSection](#anchor07)偏移量
uint32 | [BinarySection](#anchor10)偏移量

### <a id="anchor04"></a>MemberDeclaration

类型 | 描述
--- | ---
uint8 | 长度(字节)
char[] | 成员变量类型(详见[成员变量类型对照表](#anchor01))

### <a id="anchor01"></a>成员变量类型对照表

值 | 类型
--- | ---
'i' | 32位整数
'l' | 64位整数
'f' | 32位浮点
'd' | 64位浮点
'S' | 字符串(以零结尾)
'B' | 二进制数据
'O' | caps对象

### <a id="anchor05"></a>NumberSection

类型 | 描述　
--- | ---
uint32[] | 数据int或float类型

### <a id="anchor06"></a>LongDataSection

类型 | 描述
--- | ---
int64[] | 数据 long或double类型

### <a id="anchor08"></a>StringInfoSection

类型 | 描述
--- | ---
[StringInfo](#anchor11)[] | string长度及实际数据位置偏移量

### <a id="anchor11"></a>StringInfo

类型 | 描述
--- | ---
uint32 | 字符串长度
uint32 | 偏移量(字节)

### <a id="anchor12"></a>BinaryInfoSection

类型 | 描述
--- | ---
[BinaryInfo](#anchor09)[] | binary长度及实际数据位置偏移量

### <a id="anchor09"></a>BinaryInfo

类型 | 描述
--- | ---
uint32 | 长度
uint32 | 偏移量(字节)

### <a id="anchor07"></a>StringSection

类型 | 描述
--- | ---
char[] | 字符串值

### <a id="anchor10"></a>BinarySection

类型 | 描述
--- | ---
byte[] | 二进制数据块

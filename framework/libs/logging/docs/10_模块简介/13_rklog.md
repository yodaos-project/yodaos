# RKLog概要设计说明
RKLog提供了基础的底层log功能，为应用提供C/C++接口函数及宏定义，主要包括三部分基本功能：

* log归一化输出到标准输出stdout/logd，自动添加日期，时间，文件名，行号，log等级等信息；
* 归一化后的log输出到本地socket端口，并提供rklogger工具接收log；
* 集成了阿里云log sdk，将本地log发送到云端（使用阿里云log服务）。

## 宏定义及函数接口说明(RKLog.h)

1.对外提供5个log输出等级，由低到高分别为：verbose，debug，info，warn，error，对应提供了5个宏定义接口打印log：RKLogv(...),RKLogd(...),RKLogi(...),RKLogw(...),RKLoge(...)，支持格式化输出，用法与printf相同。

同时，为了更加清晰明了，还定义了如下宏定义：

		#define RKLog RKLogv
		#define RKNotice RKLogd
		#define RKInfo RKLogi
		#define RKWarn RKLogw
		#define RKError RKLoge

另外，还将ALOGV，ALOGD，ALOGI，ALOGW，ALOGE进行了重定义，方便从Android平台移植代码的log输出。

支持自定义LOG_TAG，将自动填入log开头，如无定义，则为NULL，不添加logtag。

2.对外提供了一个函数形式的log接口：

`extern void jslog(int level, const char *file, int line, const char *logtag, const char *format, ...);`

* level：取值1~5，对应verbose~error五个等级；
* file：函数名，如无，填NULL；
* line：行号，如file为NULL，这里填0；
* logtag：添加的log tag，如无，填NULL;
* format和...: 格式化输出参数，要打印log的具体内容；

函数形式接口主要为上层语言提供封装接口，无法自动获取上层语言的函数名和行号等信息，因此需要将其作为参数填入。如C/C++源码中使用，建议直接使用宏定义形式的接口。

3.为阿里云log开关封装了一个接口：

`extern void set_cloudlog_on_with_auth(unsigned int on, const char *auth);`

* on: 取值0~5，0表示关闭log上云，1~5表示level值，低于此level值的log将不会发送到云端。
* auth：authorization字符串，httpgw校验用。

这个是阿里云log开关接口，开关和level信息将由flora发送到log2cloud服务。由log2cloud服务实现log的上云控制。关于阿里云log请参考阿里云log设计文档(3rd/linux_lib/aliyunlog-lite/RKCLOUD\_README.md)

## RKLog使用说明
RKLog模块的使用较为简单：

* CMakeLists.txt中添加rklog库依赖；
* 源码中#include "rklog/RKLog.h" 头文件；
* 使用RKLog("xxx")等宏进行log输出。

具体可以参考sample下的例子，下面主要说明下3个基本功能的实现及如何查看log：

### log输出到stdout/logd功能

log添加时间日期等信息后，会先判断stdout是否是终端，如果是，则直接输出到stdout，如果不是，则调用syslog函数将log输出到logd中。

输出到logd中的log，可以通过在adb shell下输入

**`logread -f`**

查看log。

**注**:直接调用printf而不是RKLog接口打印的log，想要通过这种方式查看也可以，需要在启动进程脚本加入一行：

`procd_set_param stdout 1`

但是没有文件名、行号等自动添加的内容。

### log输出到socket端口

RKLog提供将log输出到socket端口的功能，端口号通过getenv("ROKID\_LUA\_PORT")得到，因此，要使用此功能，需要在进程启动脚本中配置环境变量：

`procd_set_param env "ROKID_LUA_PORT=15006"`

配置好端口号后，则可以使用“rklogger 端口号” 命令获取log，如

**`rklogger 15006`**

**注**:此功能好处是不依赖系统工具，可独立使用; RKLog作为socket server端; 如不配置此环境变量值，则默认不使能此功能。

### 使用阿里云log服务，log发送到阿里云服务器

RKLog集成了aliyun-log-c-sdk lite版本，提供一个服务，将log发送到阿里云log服务器，方便远程分析问题。log上云服务中已默认配置好了rokid的endpoint，其中access\_id,access\_key等采用STS token校验方式，从Rokid的sts 服务器获取临时token，如需更改此配置，参考”阿里云LOG概要设计说明“(3rd/linux_lib/aliyunlog-lite/RKCLOUD\_README.md)和github上[阿里云log c sdk lite](https://github.com/aliyun/aliyun-log-c-sdk/tree/lite).

logstore的配置由`ro.rokid.device`属性获取，不同产品有不同的logstore，增加新产品时，需要在云端创建新的logstore，并修改`openwrt/package/rokid/property/Makefile`，修改`ro.rokid.device`属性的值。

云端创建新的logstore请联系PP，修改属性值的方法请参考`openwrt/package/rokid/property/Makefile`

### LOG打印等级的配置

支持静态配置和动态配置两种方式：

* 静态配置：defconfig文件中添加配置CONFIG\_ROKIDOS\_BOARDCONFIG\_LOG\_LEVEL=level, level取值为1~5，对应上文中的5个等级，低于此配置的log将不会输出。无此配置，则默认为最低等级1，即所有log都会输出。
* 动态配置：提供了一个动态配置log等级的命令：set\_log\_level -[level], level取"v d i w e"中其一，对应上文中5个等级，设置完后，低于此等级的log将不会输出。例如：set\_log\_level -i, 则低于info等级的log(RKLogv和RKLogd)将不会输出。

**注**:动态配置优先级高于静态配置，重启设备后动态配置失效。

### LOG本地存储

考虑到data分区较小，并且写入效率较低，因此默认不提供log落盘功能。如果测试时需要，可以借助openwrt的logd和logread机制实现。方法为更改openwrt/target/linux/leo/base-files/etc/config/system文件中的

```
option log_file '/var/log/messages.logd'
option log_size 1000
```

两项。其中log\_file即log文件存储地址，改为/data/log/messages.logd即可存在data分区。log\_size为存储log大小，单位为K。log滚动存储，达到配置大小后即保存一个messages.logd.old文件, messages.log则继续缓存新内容，满了之后覆盖原old文件。 也就是说，这里配置为1000K，实际需要2x1000K的空间，最多缓存2x1000K大小的log。

注：log size不宜配置过大，否则可能影响OTA功能；要存储log的进程，启动脚本需要配置procd\_set\_param stdout 1

## RKLog.hpp中提供的接口

RKLog.hpp中提供了C++接口，除RKLog.h中的定义外，提供了重载的log函数，除了支持可变参数外，还支持vector\<char *>容器类型的参数。如果有这种需求，可以使用此文件提供的接口，具体可以参考源文件。默认C++接口已不再对外开放，如需使用，需要更改Makefile中的`Build/InstallDev`，将hpp拷贝到编译环境。
[rokid_gitbook_path]: # (10_模块简介/httpsession.md)
# httpsession

## 1. 简介

`httpsession` 封装 `libcurl` [multi](https://curl.haxx.se/libcurl/c/libcurl-multi.html) 接口，进行并发，异步的 HTTP 请求。

## 2. API 说明

### 2.1 常量说明

#### 2.1.1 HttpSession::SSLVerifyPolicy

SSL 校验策略，掩码形式，可以选择检验单个或多个，所有值如下

* VERIFY_NONE: 完全不做校验
* VERIFY_PEER: 校验证书
* VERIFY_HOST: 校验域名是否匹配
* VERIFY_STATUS: 校验状态
* VERIFY_ALL: 以上都校验

#### 2.1.2 HttpSession::Status

请求的状态，枚举类型，所有值如下

* INIT: 初始状态，调用 request 之后，真正开始请求之前的状态
* PULLING: 请求中
* FINISHED: 请求成功，查看 Ticket.response 读取返回信息
* CANCELED: 请求被取消
* FAILED: 请求失败，查看 Ticket.errorCode 和 Ticket.errorMessage 获取失败原因

### 2.2 类和结构体说明

#### 2.2.1 HttpSession

每个 `HttpSession` 对象包含一个 `CURLM` 实例，N (N >= 0)个 easy handle（CURL)，使用 `select` 函数监听所有端口；`select` 函数和所有 `libcurl` 接口都在**同一个线程**中调用，所有回调也都在这个线程中执行。

应用可以创建一个或多个 `HttpSession` 实例，但是通常只需要创建一个，因为单个实例中没有并发请求的个数限制，而且实例中会共享 DNS 解析缓存，连接池等数据(see: [libcurl tutorial](https://curl.haxx.se/libcurl/c/libcurl-tutorial.html) Sharing Data Between Easy Handles)。

#### 2.2.2 HttpSession::Options

    struct Options
    {
        string baseUrl;
        int timeout;
        bool verbose;
    };

* baseUrl: 请求时 baseUrl 会加到 Request.url 前，比如 baseUrl 值为 `https://example.com`，Request.url 值为 `/echo`，那么最终请求的 url 为 `https://example.com/echo`。可以按 host 创建不同的 HttpSession 实例，请求的时候不需要关心具体的 host 值。
* timeout: 超时时间，单位是**秒**，小于等于0表示不设置超时时间
* verbose: 是否输出日志，httpsession 中没有使用 rklog

#### 2.2.3 HttpSession::Ticket

每次成功**开始**的请求都会返回一个 Ticket，其中包含这次请求所有的信息，包括 Request，Response，Listener 等；第二是作为这次请求的句柄，可以使用 Ticket 取消这次请求。主要的属性

属性 | 类型 | 说明
---|---|---
request | HttpSession::Request | 请求信息，详见下文
response | HttpSession::Response | 请求结果，status 为 `FINISHED` 时值有效，详见下文
status | HttpSession::Status | 这次请求的状态，详见上文
errorCode | int | 这次请求的 `libcurl` 错误码，status 为 `FAILED` 时值有效
errorMessage | const char * | 这次请求的 `libcurl` 错误信息，只有 status == FAILED 时值才有效
userdata | void * | 任何时候都可以为 Ticket 设置 userdata，与 Request.userdata 独立
startTime | double | 请求的开始时间，实现的原因，可能早于真正 `libcurl` 开始请求的时间
endTime | double | 请求的结束时间，status 为 `FINISHED` 时值有效
duration | double | 请求的持续时长，endTime - startTime，status 为 `FINISHED` 时值有效
info | HttpSession::Info | 更具体的耗时信息，比如 DNS，连接，SSL 握手等阶段的耗时，status 为 `FINISHED` 或 `FAILED` 时值有效，详见下文

#### 2.2.4 HttpSession::Ticket::Deleter

    typedef void (*Deleter)(Ticket *tic);

请求时传入，Ticket 被析构时会调用该函数，使应用可以做一些最后的处理；该函数会在释放 body 前调用。

#### 2.2.5 HttpSession::Request

单次请求的所有参数

属性 | 类型 | 默认值 | 说明
---|---|---|---
url | string | | 请求的url
method | string | GET | HTTP method
headers | map<string, string> | | HTTP header
timeout | int | 0 | 超时时间，单位是秒，等于0时使用 HttpSession 的超时设置
body | const char * | NULL | HTTP body
length | size_t | -1 | body 长度，body 不为 `NULL` 并且 length 小于0时使用 `strlen` 函数计算出 length
releaseBody | bool | false | 请求失败或者结束后是否自动释放 body
userdata | void * | NULL |
verifyPolicy | HttpSession::SSLVerifyPolicy | VERIFY_ALL | SSL 校验策略，详见上文
sslCert | string | | ssl 证书路径，见[CURLOPT_SSLCERT](https://curl.haxx.se/libcurl/c/CURLOPT_SSLCERT.html)
sslCertType | string | | ssl 证书类型，见[CURLOPT_SSLCERTTYPE](https://curl.haxx.se/libcurl/c/CURLOPT_SSLCERTTYPE.html)
sslKey | string | | ssl 证书 key，见[CURLOPT_SSLKEY](https://curl.haxx.se/libcurl/c/CURLOPT_SSLKEY.html)
sslPassword | string | | ssl 证书密码，见[CURLOPT_KEYPASSWD](https://curl.haxx.se/libcurl/c/CURLOPT_KEYPASSWD.html)

#### 2.2.6 HttpSession::Response

请求返回的结果，请求的返回过程中，个别属性就开始被设置，特别是 code 在返回数据的第一行；处理返回信息之前应该检查 Ticket.status() 为 `FINIHSED` 状态。

属性 | 类型 | 默认值 | 说明
---|---|---|---
majorVersion | int | 0 | HTTP 协议主版本
minorVersion | int | 0 | HTTP 协议次版本，比如 HTTP/1.1 协议，majorVersion 为1，minorVersion 也为1
code | int | 0 | HTTP status code
reason | string | | HTTP reason，可能为空
headers | map<string, string> | | HTTP response header
body | const char * | NULL | HTTP response body
contentLength | size_t | -1 | HTTP Content-Length

majorVersion，minorVersion，code，reason 都是从第一行 [status-line](https://tools.ietf.org/html/rfc2616#section-6.1) 解析出来的，大部分情况下是

    HTTP/1.1 200 OK

#### 2.2.7 HttpSession::Info

使用[curl_easy_getinfo 函数](https://curl.haxx.se/libcurl/c/curl_easy_getinfo.html)获取这次请求从开始到一些关键事件消耗的时长，成功和失败都会填写该结构体，HttpSession::Options verbose 为 true 时会打印这些信息。所有属性如下

属性 | 类型 | 默认值 | 说明
---|---|---|---
totalTime | double | 0 | 从开始到请求结束的时长，可能比 Ticket.duration 短
nameLookupTime | double | 0 | 从开始到解析域名结束的时长
connectTime | double | 0 | 从开始到连接成功的时长
handshakeTime | double | 0 | 从开始到握手成功的时长
pretransferTime | double | 0 | 从开始到数据传输前的时长
startTransferTime | double | 0 | 从开始到收到第一个字节返回的时长
redirectTime | double | 0 | 请求过程中所有重定向消耗的时长

#### 2.2.8 HttpSessionRequestListenerInterface

请求的 listener 接口定义，现在只有两个函数，成功或失败时调用 `onRequestFinished`，被取消时调用 `onRequestCanceled`。单个 HttpSession 实例中所有请求的回调都在同一个线程中执行。

    virtual void onRequestFinished(HttpSession *session, HttpSession::Ticket *tic, HttpSession::Response *resp)
    virtual void onRequestCanceled(HttpSession *session, HttpSession::Ticket *tic)

### 2.3 函数说明

#### 2.3.1 HttpSession::request

    shared_ptr<Ticket> request(Request &req, HttpSessionRequestListenerInterface *listener, Ticket::Deleter deleter = NULL);

开始请求。如果 HttpSession 必要的初始化执行失败，会返回无效的 shared_ptr，否则都会返回有效的 Ｔicket，如果 url 或 method 不合法，会执行正常的请求失败流程。

返回的 shared_ptr<Ticket> 可以不保持，HttpSession 中会有强引用，直到请求结束。

参数列表

* req: 请求的url和参数
* listener: 请求的回调
* deleter: Ｔicket 被析构时调用

#### 2.3.2 HttpSession::cancel

    int cancel(shared_ptr<Ticket> tic);

取消某次请求。tic 无效或 session 不在运行状态返回-1，否则返回0；注意返回0不代表取消成功，取消成功是会调用 `onRequestCanceled` 回调(在统一的线程中)。

#### 2.3.3 HttpSession::stop

    int stop(shared_ptr<Ticket> tic);

停止某次请求。tic 无效或 session 不在运行状态返回-1，否则返回0；与 `cancel` 不同的是调用 `stop` 后不会再收到任何回调调用。

## 3. 使用说明

### 3.1 工具使用说明

#### 3.1.1 httpsession_cli

该工具使用 libhttpsession.so 接口，可以作为测试工具使用。该工具支持的选项

* -m: HTTP method
* -h: HTTP header，使用 k1=v1,k2=v2,k3=v3 的格式
* -b: HTTP body，其后的字符串参数会作为该次请求的 body
* -c: 单个 url 请求的次数
* -t: 超时时长
* -V: verbose 模式

选项后可以有一个或多个 url，比如

    httpsession_cli -m POST -b hello -c 100 -t 2 http://example.com/foo http://example.com/bar

会使用 POST 方法，重复请求 `http://example.com/foo` 和 `http://example.com/bar` 100次，每次同时请求这两个 url，每次等待2秒。

### 3.2 示例代码说明

示例请参考 src/cli/main.cpp。





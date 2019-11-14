/*
 * log to cloud service, based on logread, use aliyunlog-lite sdk, use flora for IPC.
 */
#include <aliyunlog-lite/aloglite.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cjson/cJSON.h>
#include <libubox/ustream.h>
#include <libubox/blobmsg_json.h>
#include <libubox/usock.h>
#include <libubox/uloop.h>
#include "libubus.h"

#include <cutils/properties.h>
#include <curl/curl.h>
#include <flora-cli.h>
#define LOG_TAG "log2cloud"
#include "RKLog.h"

//#define LOGD_CONNECT_RETRY	5
#define STS_TOKEN_SIZE 10 * 1024
#define HTTP_AUTH_SIZE 512


enum {
    LOG_MSG,
    LOG_ID,
    LOG_PRIO,
    LOG_SOURCE,
    LOG_TIME,
    __LOG_MAX
};

static const struct blobmsg_policy log_policy[] = {
    [LOG_MSG] = { .name = "msg", .type = BLOBMSG_TYPE_STRING },
    [LOG_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
    [LOG_PRIO] = { .name = "priority", .type = BLOBMSG_TYPE_INT32 },
    [LOG_SOURCE] = { .name = "source", .type = BLOBMSG_TYPE_INT32 },
    [LOG_TIME] = { .name = "time", .type = BLOBMSG_TYPE_INT64 },
};

static flora_cli_t cli;
static char gHttpauth[HTTP_AUTH_SIZE] = {0};
static long gTimeoutTime = 0;

//static int logd_conn_tries = LOGD_CONNECT_RETRY;
static rokid_cloudloglite_client *gCloudClient = NULL;
static int cloudlog_level = 0;
static char deviceId[PROP_VALUE_MAX] = {0};
static int gSanbox = 0;


void myRequestFinished(const char *body);
void httpRequestToken();
static void setLogOn(int enable, const char *auth)
{
    memset(gHttpauth,0,HTTP_AUTH_SIZE);
    if (enable != 0)
    {
        strcpy(gHttpauth,"Authorization: ");
        if (auth != NULL)
        {
            strncat(gHttpauth, auth, HTTP_AUTH_SIZE-16);
        }
        char sanbox[PROP_VALUE_MAX];
        property_get("persist.sys.rokid.sandbox", sanbox, "0");
        gSanbox = atoi(sanbox);
    }
    RKLogi("set cloudlog on with auth:%d, auth=%s\n", enable,gHttpauth);
    cloudlog_level = enable;
}

void myRequestFinished(const char *body)
{
    cJSON *root = NULL;
    cJSON *stsInfo = NULL;
    cJSON *item = NULL;
    RKLogi("-request finished-\n");
    root = cJSON_Parse(body);
    if (!root)
    {
        RKLoge("Error before: [%s]\n", cJSON_GetErrorPtr());
        sleep(5); //sleep 5 then retry, to avoid high cpu load,
        return;
    }

    stsInfo = cJSON_GetObjectItem(root, "stsInfo");
    if (!stsInfo) //no stsInfo,means error
    {
        RKLoge("Error before: [%s]\n", cJSON_GetErrorPtr());
        sleep(5);
    }
    else
    {
        struct tm tminfo;
        if (strptime(cJSON_GetObjectItem(stsInfo, "expiration")->valuestring, "%Y-%m-%dT%H:%M:%SZ", &tminfo) != NULL)
        {
            gTimeoutTime = mktime(&tminfo) + 28800;
            RKLogi("time of exptime is %ld\n", gTimeoutTime);
            rokid_reset_sts_token(gCloudClient, cJSON_GetObjectItem(stsInfo, "accessKeyId")->valuestring, 
                                                cJSON_GetObjectItem(stsInfo, "accessKeySecret")->valuestring, 
                                                cJSON_GetObjectItem(stsInfo, "token")->valuestring);
        }
        else
        {
            RKLoge("reset sts get time error\n");
            sleep(5);
        }
    }
    cJSON_Delete(root);
    return;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp){
    char *src = (char*)buffer;
    char *dest = (char*)userp;
    int ret = 0;
    if (src != NULL){
        strncat(dest,src,size*nmemb);
        ret = size*nmemb;
    }
    return ret;
}

void httpRequestToken()
{
    RKLogi("request token\n");
    const char *body = "{}";
    const char *requrl;
    char result[STS_TOKEN_SIZE] = {0};
    if (gSanbox == 0)
    {
        requrl = "https://cloudapigw.open.rokid.com/v1/sts/StsService/GetToken";
    }
    else
    {
        requrl = "https://cloudapigw-dev.open.rokid.com/v1/sts/StsService/GetToken";
    }
    CURL *hnd = curl_easy_init();

    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, requrl);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, gHttpauth);
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, "{}");
    curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, &write_data);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, result);

    CURLcode ret = curl_easy_perform(hnd);
    if (ret != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(ret));
    }
    else
    {
        RKLogi("%s\n",result);
        myRequestFinished(result);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(hnd);

    return;
}

static int log_notify(struct blob_attr *msg)
{
    struct blob_attr *tb[__LOG_MAX];
    struct stat s;
    uint32_t p;
    time_t t;
    char *c, *m;

    blobmsg_parse(log_policy, ARRAY_SIZE(log_policy), tb, blob_data(msg), blob_len(msg));
    if (!tb[LOG_ID] || !tb[LOG_PRIO] || !tb[LOG_SOURCE] || !tb[LOG_TIME] || !tb[LOG_MSG])
        return 1;

    m = blobmsg_get_string(tb[LOG_MSG]);
    t = blobmsg_get_u64(tb[LOG_TIME]) / 1000;
    c = ctime(&t);
    p = blobmsg_get_u32(tb[LOG_PRIO]);
    c[strlen(c) - 1] = '\0';
    if((p > 16 - cloudlog_level) || (cloudlog_level == 0))
    {
        //RKLoge("loglevel:%d,cloudlog level:%d----drop\n",p,cloudlog_level);
        return 0;
    }
    if (gCloudClient == NULL)
    {
        char logstore[PROP_VALUE_MAX];
        property_get("ro.rokid.device", logstore, "rokid_me");
        property_get("ro.boot.serialno", deviceId, "unknown");
        if(strcmp(logstore, "xxxxxxxx") == 0)
        {
            strncpy(logstore, "rokid_me", 8);
        }
        rokid_log_configs conf = {
            .project = "rokid-linux",
            .logstore = logstore,
            .name = "Rokid",
            .endpoint = "https://cn-hangzhou.log.aliyuncs.com",
            .log_bytes_per_package = 255 * 1024,
            .log_count_per_package = 4096,
            .package_timeout_ms = 30000,
            .max_buffer_bytes = 512 * 1024,
        };
        gCloudClient = create_client(&conf);
        if (gCloudClient == NULL)
        {
            return 0;
        }
        RKLogi("create client done\n");
    }
    else
    {
        if (gTimeoutTime - t < 300) //exp time <5min,try to update sts token
        {
            RKLogi("reset sts token,current time:%ld,timeout:%ld\n", t, gTimeoutTime);
            httpRequestToken();
        }
        rkloglite_cloud(gCloudClient, 6,
                        "__sn__", deviceId,
                        "__timestamp__", c,
                        "__zcontent__", m);
    }
    return 0;
}

static void logread_fd_data_cb(struct ustream *s, int bytes)
{
    while (true) {
        struct blob_attr *a;
        int len, cur_len;
        a = (void*) ustream_get_read_buf(s, &len);
        if (len < sizeof(*a))
            break;

        cur_len = blob_len(a) + sizeof(*a);
        if (len < cur_len)
            break;

        log_notify(a);
        ustream_consume(s, cur_len);
    }
}

static void logread_fd_state_cb(struct ustream *s)
{
    //logd_conn_tries = LOGD_CONNECT_RETRY;
    uloop_end();
}

static void logread_fd_cb(struct ubus_request *req, int fd)
{
    static struct ustream_fd test_fd;

    memset(&test_fd, 0, sizeof(test_fd));

    test_fd.stream.notify_read = logread_fd_data_cb;
    test_fd.stream.notify_state = logread_fd_state_cb;
    ustream_fd_init(&test_fd, fd);
}


static void log_service_command_callback(const char* name, uint32_t msgtype, caps_t msg, void* arg) {
    const char *buf = NULL;
    int log2cloud;
    caps_read_integer(msg,&log2cloud);
    if(0 != log2cloud)
    {
        caps_read_string(msg, &buf);
    }
    setLogOn(log2cloud,buf);

}

static void conn_disconnected(void* arg)
{
    RKLogi("disconnected\n");
}

int log_flora_init() {
    flora_cli_callback_t cb;

    memset(&cb, 0, sizeof(cb));

    cb.recv_post = log_service_command_callback;
    //cb.recv_get = NULL;
    cb.disconnected = conn_disconnected;

    while (1) {
        if (flora_cli_connect("unix:/var/run/flora.sock", &cb, NULL, 0, &cli) != FLORA_CLI_SUCCESS) {
            RKLoge("connect to flora failed, will try again after 1s");
            sleep(1);
        } else {
            flora_cli_subscribe(cli, "log2cloud.on");
            break;
        }
    }
    RKLogi("log_flora_init done\n");
    return 0;
}

int main(int argc, char **argv)
{
    struct ubus_context *ctx;
    uint32_t id;
    const char *ubus_socket = NULL;
    int ch, ret/* , lines = 0 */;
    static struct blob_buf b;

    signal(SIGPIPE, SIG_IGN);

    log_flora_init();

    uloop_init();
    ctx = ubus_connect(ubus_socket);
    if (!ctx) {
        fprintf(stderr, "Failed to connect to ubus\n");
        return -1;
    }
    ubus_add_uloop(ctx);

    blob_buf_init(&b, 0);
    blobmsg_add_u8(&b, "stream", 1);
    blobmsg_add_u8(&b, "oneshot", 0);
    blobmsg_add_u32(&b, "lines", 0);

    RKLogi("set log on,contunue...\n");
    do {
        struct ubus_request req = { 0 };

        ret = ubus_lookup_id(ctx, "log", &id);
        if (ret) {
            fprintf(stderr, "Failed to find log object: %s\n", ubus_strerror(ret));
            sleep(5);
            continue;
        }
        //logd_conn_tries = 0;

        ubus_invoke_async(ctx, id, "read", b.head, &req);
        req.fd_cb = logread_fd_cb;
        ubus_complete_request_async(ctx, &req);

        uloop_run();

    } while (1);

    ubus_free(ctx);
    uloop_done();

    if (gCloudClient != NULL)
    {
        destory_client(gCloudClient);
        gCloudClient = NULL;
    }
    return ret;
}

#include <inttypes.h>
#include <log/logd.h>

int __android_log_security_bswrite(int32_t tag, const char *payload)
{
#if 0
    struct iovec vec[4];
    char type = EVENT_TYPE_STRING;
    uint32_t len = strlen(payload);

    vec[0].iov_base = &tag;
    vec[0].iov_len = sizeof(tag);
    vec[1].iov_base = &type;
    vec[1].iov_len = sizeof(type);
    vec[2].iov_base = &len;
    vec[2].iov_len = sizeof(len);
    vec[3].iov_base = (void*)payload;
    vec[3].iov_len = len;

    return write_to_log(LOG_ID_SECURITY, vec, 4);
#endif
    return 0;
}

int __android_log_print(int prio, const char *tag,  const char *fmt, ...)
{
#if 0
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    return __android_log_write(prio, tag, buf);
#endif
    return 0;
}

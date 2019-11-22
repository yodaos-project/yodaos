#ifndef IOTJS_MODULE_TLS_BIO_H
#define IOTJS_MODULE_TLS_BIO_H

#include "mbedtls/ssl.h"
#include "stdlib.h"
#include "string.h"
#include "strings.h"

enum { /* ssl Constants */
       SSL_ERROR_NONE = 0,
       SSL_FAILURE = 0,
       SSL_SUCCESS = 1,
       SSL_SHUTDOWN_NOT_DONE = 2,

       SSL_ALPN_NOT_FOUND = -9,
       SSL_BAD_CERTTYPE = -8,
       SSL_BAD_STAT = -7,
       SSL_BAD_PATH = -6,
       SSL_BAD_FILETYPE = -5,
       SSL_BAD_FILE = -4,
       SSL_NOT_IMPLEMENTED = -3,
       SSL_UNKNOWN = -2,
       SSL_FATAL_ERROR = -1,

       SSL_FILETYPE_ASN1 = 2,
       SSL_FILETYPE_PEM = 1,
       SSL_FILETYPE_DEFAULT = 2, /* ASN1 */
       SSL_FILETYPE_RAW = 3,     /* NTRU raw key blob */

       SSL_VERIFY_NONE = 0,
       SSL_VERIFY_PEER = 1,
       SSL_VERIFY_FAIL_IF_NO_PEER_CERT = 2,
       SSL_VERIFY_CLIENT_ONCE = 4,
       SSL_VERIFY_FAIL_EXCEPT_PSK = 8,

       SSL_SESS_CACHE_OFF = 0x0000,
       SSL_SESS_CACHE_CLIENT = 0x0001,
       SSL_SESS_CACHE_SERVER = 0x0002,
       SSL_SESS_CACHE_BOTH = 0x0003,
       SSL_SESS_CACHE_NO_AUTO_CLEAR = 0x0008,
       SSL_SESS_CACHE_NO_INTERNAL_LOOKUP = 0x0100,
       SSL_SESS_CACHE_NO_INTERNAL_STORE = 0x0200,
       SSL_SESS_CACHE_NO_INTERNAL = 0x0300,

       SSL_ERROR_WANT_READ = 2,
       SSL_ERROR_WANT_WRITE = 3,
       SSL_ERROR_WANT_CONNECT = 7,
       SSL_ERROR_WANT_ACCEPT = 8,
       SSL_ERROR_SYSCALL = 5,
       SSL_ERROR_WANT_X509_LOOKUP = 83,
       SSL_ERROR_ZERO_RETURN = 6,
       SSL_ERROR_SSL = 85,

       SSL_SENT_SHUTDOWN = 1,
       SSL_RECEIVED_SHUTDOWN = 2,
       SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER = 4,
       SSL_OP_NO_SSLv2 = 8,

       SSL_R_SSL_HANDSHAKE_FAILURE = 101,
       SSL_R_TLSV1_ALERT_UNKNOWN_CA = 102,
       SSL_R_SSLV3_ALERT_CERTIFICATE_UNKNOWN = 103,
       SSL_R_SSLV3_ALERT_BAD_CERTIFICATE = 104,
       PEM_BUFSIZE = 1024
};

typedef unsigned char BYTE;

struct _BIO;
typedef struct _BIO BIO;

struct _BIO {
  BIO* prev;  /* previous in chain */
  BIO* next;  /* next in chain */
  BIO* pair;  /* BIO paired with */
  BYTE* mem;  /* memory buffer */
  int wrSz;   /* write buffer size (mem) */
  int wrIdx;  /* current index for write buffer */
  int rdIdx;  /* current read index */
  int readRq; /* read request */
  int memLen; /* memory buffer length */
  int type;   /* method type */
};

enum {
  SSL_BIO_ERROR = -1,
  SSL_BIO_UNSET = -2,
  SSL_BIO_SIZE = 17000 /* default BIO write size if not set */
};

enum BIO_TYPE {
  BIO_BUFFER = 1,
  BIO_SOCKET = 2,
  BIO_SSL = 3,
  BIO_MEMORY = 4,
  BIO_BIO = 5,
  BIO_FILE = 6
};

BIO* iotjs_ssl_bio_new(int type);
int iotjs_bio_make_bio_pair(BIO* b1, BIO* b2);

size_t iotjs_bio_ctrl_pending(BIO* bio);
int iotjs_bio_read(BIO* bio, const char* buf, size_t size);
int iotjs_bio_write(BIO* bio, const char* buf, size_t size);

int iotjs_bio_reset(BIO* bio);
int iotjs_bio_net_recv(void* ctx, unsigned char* buf, size_t len);
int iotjs_bio_net_send(void* ctx, const unsigned char* buf, size_t len);
int iotjs_bio_free_all(BIO* bio);
int iotjs_bio_free(BIO* bio);

#endif // IOTJS_MODULE_TLS_BIO_H

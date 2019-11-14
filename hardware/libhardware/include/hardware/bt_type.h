#ifndef __BT_TYPE_H__
#define __BT_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define BT_BIT(n)  (1<<n)

#ifndef BT_LIKELY
#ifdef __GNUC__
#define BT_LIKELY(x) (__builtin_expect(!!(x),1))
#define BT_UNLIKELY(x) (__builtin_expect(!!(x),0))
#else
#define BT_LIKELY(x) (x)
#define BT_UNLIKELY(x) (x)
#endif
#endif

typedef uint8_t BTAddr[6];

typedef void*           bt_handle;

#define MAX_LEN_UUID_STR 37

typedef struct {
        enum {
		BT_UUID_INVALID = 0,
                BT_UUID_STRING = 1,
		BT_UUID_16 = 16,
		BT_UUID_32 = 32,
		BT_UUID_128 = 128,
	} type;
	union {
		char uuid[MAX_LEN_UUID_STR];
		uint16_t  uuid16;
		uint32_t  uuid32;
                uint8_t  uuid128[16];
	} value;
} BT_UUID;

typedef struct BTDevice
{
    BTAddr bd_addr; /* BD address peer device. */
    char name[249]; /* Name of peer device. */
    int rssi;       /* RSSI of peer device. */
} BTDevice;


typedef struct BT_disc_device
{
    int in_use;
    BTDevice device;
} BT_disc_device;


enum bt_profile_type {
    BT_A2DP_SOURCE = 0,
    BT_A2DP_SINK,
    BT_BLE,
    BT_HS,
    BT_HH,
    BT_PERIPHERAL,
    BT_BR_EDR,
    BT_ALL,
};

#ifdef __cplusplus
}
#endif

#endif /* __BT_TYPE_H__ */

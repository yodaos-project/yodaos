#ifndef __A2DP_H__
#define __A2DP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <bsa_rokid/bsa_api.h>
#include <hardware/bt_common.h>

typedef struct A2dpCtx_t A2dpCtx;
A2dpCtx *a2dp_create(void *caller);
int a2dp_destroy(A2dpCtx *ac);

int  a2dp_is_enabled(A2dpCtx *ac);
int  a2dp_is_opened(A2dpCtx *ac);

int  a2dp_enable(A2dpCtx *ac);
int  a2dp_open(A2dpCtx *ac, BD_ADDR bd_addr);
int  a2dp_start(A2dpCtx *ac);
int  a2dp_start_with_aptx(A2dpCtx *ac);
int a2dp_stop(A2dpCtx *ac);

int  a2dp_close(A2dpCtx *ac);
int  a2dp_close_device(A2dpCtx *ac, BD_ADDR bd_addr);
int a2dp_disable(A2dpCtx *ac);
int a2dp_set_listener(A2dpCtx *ac, a2dp_callbacks_t listener, void *listener_handle);
int a2dp_get_connected_devices(A2dpCtx *ac, BTDevice *dev_array, int arr_len);
int a2dp_send_rc_command(A2dpCtx *ac, uint8_t cmd);

#ifdef __cplusplus
}
#endif
#endif /* __A2DP_H__ */

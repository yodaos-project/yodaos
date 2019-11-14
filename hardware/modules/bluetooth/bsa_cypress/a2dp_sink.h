#ifndef _A2DP_SINK_H_
#define _A2DP_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <bsa_rokid/bsa_api.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/def.h>
#include <pulse/sample.h>
#include "aac_latm_decoder.h"
#include <hardware/bt_common.h>

#define MAX_CONNECTIONS 1
#define A2DP_MIN_ABS_VOLUME        0x00   /* Min and max absolute vol */
#define A2DP_MAX_ABS_VOLUME        0x7F

#define A2DP_DUMP_DATA
//#define  A2DP_DEBUG_TIME

/* data type for the MPEG-2, 4 AAC Codec Information Element*/
typedef struct
{
    uint8_t   obj_type;   /* Object type */
    uint16_t  samp_freq;  /* Sampling frequency */
    uint8_t   chnl;       /* Channel mode */
    uint8_t vbr;        /* Variable Bit Rate */
    uint32_t  bitrate;    /* Bit rate index */
} AAC_M24;

typedef struct
{
    BTAddr bda_connected;  /* peer BDA*/
    uint8_t in_use;         /* TRUE if this connection block is in use */
    int index;              /* Index of this connection block in the array */
    char name[249]; /* Name of peer device. */

    uint8_t ccb_handle;       /* AVK (A2DP) handle */
    uint8_t is_open;        /* TRUE if AVK (signaling channel) is open */

    uint8_t rc_handle;        /* AVRCP handle */
    uint8_t is_rc_open;     /* TRUE if AVRCP is open */
    uint16_t peer_features;       /* peer AVRCP features mask */
    uint16_t peer_version;    /* Peer AVRCP version */

    uint8_t is_streaming_chl_open; /* TRUE is streaming channel is open */
    uint8_t is_started_streaming;     /* TRUE if streaming has started */
    uint8_t format;
    uint16_t sample_rate;
    uint8_t num_channel;
    uint8_t bit_per_sample;

    bool m_bAbsVolumeSupported;
    UINT8 volChangeLabel;   /* label used for volume change registration. */
    AAC_M24 aac;
} BTConnection;

struct a2dp_aac_remain
{
    int len;
    int need_len;
    UINT8 buffer[4096];
};

struct A2dpSink_t
{
#ifdef A2DP_DUMP_DATA
    FILE *fp;
#endif
    void *caller;
    int uipc_channel;
    //int volume;

    int is_enabled;
    int is_opened;
    int is_started;
    int is_playing;
    bool mute;
    uint8_t volume;

    //BD_ADDR bd_addr;

    pa_simple *pas;

    BTConnection connections[MAX_CONNECTIONS];
    int conn_index;

    a2dp_sink_callbacks_t listener;
    void *          listener_handle;
    pthread_mutex_t mutex;

    int transaction_label;
    //int sig_chnl_open_status;
    //int sig_chnl_close_status;
    int open_pending;
    BD_ADDR open_pending_addr;
    UINT8 need_parse_aac_config;
    void *aac_decoder;
    UINT8 *aac_buff;
    struct latm_parse_data latmctx;
    struct a2dp_aac_remain aac_remain;
};

typedef struct A2dpSink_t A2dpSink;

A2dpSink *a2dpk_create(void *caller);
int a2dpk_destroy(A2dpSink *as);

int a2dpk_set_listener(A2dpSink *as, a2dp_sink_callbacks_t listener, void *listener_handle);
int  a2dpk_enable(A2dpSink *as);
int a2dpk_disable(A2dpSink *as);

int  a2dpk_open(A2dpSink *as, BD_ADDR bd_addr);
int a2dpk_close_device(A2dpSink *as, BD_ADDR bd_addr);
int a2dpk_close(A2dpSink *as);


int a2dpk_get_connected_devices(A2dpSink *as, BTDevice *dev_array, int arr_len);

int a2dpk_rc_send_getstatus(A2dpSink *as);
int a2dpk_rc_send_cmd(A2dpSink *as, UINT8 command);
int a2dpk_rc_send_play(A2dpSink *as);
int a2dpk_rc_send_stop(A2dpSink *as);
int a2dpk_rc_send_pause(A2dpSink *as);
int a2dpk_rc_send_forward(A2dpSink *as);
int a2dpk_rc_send_backward(A2dpSink *as);
int a2dpk_get_element_attr_command(A2dpSink *as);

int a2dpk_set_abs_vol(A2dpSink *as, UINT8 volume);
int a2dpk_set_mute(A2dpSink *as, bool mute);
int a2dpk_get_enabled(A2dpSink *as);
int a2dpk_get_opened(A2dpSink *as);
int a2dpk_get_playing(A2dpSink *as);
int a2dpk_get_open_pending_addr(A2dpSink *as, BD_ADDR bd_addr);
#ifdef __cplusplus
}
#endif
#endif

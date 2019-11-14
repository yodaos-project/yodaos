/*****************************************************************************
**
**  Name:               hfp.h
**
**  Description:        This is the header file for the Headset application
**
**  Copyright (c) 2009, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

/* idempotency */
#ifndef __BT_HFP_H__
#define __BT_HFP_H__

#include "bt_type.h"
/* self-sufficiency */

#ifdef  __cplusplus
extern "C"
{
#endif

typedef enum {
    BT_HS_USE_PCM,
    BT_HS_USE_UART,
    BT_HS_USE_I2S
} bt_hfp_data_type;

typedef enum {
    BT_HS_VOL_SPK = 0,
    BT_HS_VOL_MIC = 1
} BT_HS_VOLUME_TYPE;

/*  call state */
enum
{
    BT_HS_CALL_NONE,
    BT_HS_CALL_INC,
    BT_HS_CALL_OUT,
    BT_HS_CALL_CONN
};

#define BT_HS_ST_CONNECTABLE        0x0001
#define BT_HS_ST_CONNECT            0x0002
#define BT_HS_ST_SCALLSETUP         0x0004 /* if value set , outgoingcall and incomming call value are valid*/
#define BT_HS_ST_OUTGOINGCALL       0x0008 /* 0 => incomming call*/
#define BT_HS_ST_SCOOPEN            0x0010 /* 0 => sco close */
#define BT_HS_ST_VOICEDIALACT       0x0020
#define BT_HS_ST_RINGACT            0x0040
#define BT_HS_ST_WAITCALL           0x0080
#define BT_HS_ST_CALLACTIVE         0x0100
#define BT_HS_ST_INBANDRINGACT      0x0200
#define BT_HS_ST_NRECACT            0x0400
#define BT_HS_ST_CLIPACT            0x0800
#define BT_HS_ST_MUTE               0x1000
#define BT_HS_ST_3WAY_HELD          0x2000
#define BT_HS_ST_CONN_SUSPENDED     0x4000
#define BT_HS_ST_IN_CALL            (BT_HS_ST_OUTGOINGCALL | BT_HS_ST_RINGACT | BT_HS_ST_CALLACTIVE)
#define BT_HS_ST_ALL                0x1fff

#define BT_HS_MAX_CL_IDX            3
#define BT_HS_CL_BUF_SIZE           32

#define BT_HS_MAX_NUM_CONN          1


typedef enum
{
    BT_HS_STATUS_UNKNOWN          = 0,     /* UNKNOWN  STATUS.*/
    BT_HS_SERVICE_OFF           = 1,     /* implies no service. No Home/Roam network available.*/
    BT_HS_SERVICE_ON          = 2,     /* implies presence of service. Home/Roam network available */

    BT_HS_CALL_OFF          = 3,     /* means there are no calls in progress */
    BT_HS_CALL_ON           = 4,     /* means at least one call is in progress */

    BT_HS_CALLSETUP_DONE        = 5,     /* means not currently in call set up. */
    BT_HS_CALLSETUP_INCOMING       = 6,     /* means an incoming call process ongoing */
    BT_HS_CALLSETUP_OUTGOING     = 7,     /* means an outgoing call set up is ongoing */
    BT_HS_CALLSETUP_REMOTE_ALERTED     = 8,     /* means remote party being alerted in an outgoing call. */

    BT_HS_CALLHELD_NONE            = 9,    /* No calls held */
    BT_HS_CALLHELD_HOLD_WITH_ACTIVE= 10,    /* Call is placed on hold or active/held calls swapped (The AG has both an active AND a held call)*/
    BT_HS_CALLHELD_HOLD        = 11,    /* Call on hold, no active call */

    BT_HS_BATTCHG0       = 12,    /* battery info*/
    BT_HS_BATTCHG1           = 13,    /* stream channel opened */
    BT_HS_BATTCHG2           = 14,    /* stream channel suspended */
    BT_HS_BATTCHG3            = 15,    /* stream channel closed */
    BT_HS_BATTCHG4         = 16,    /* remote control channel open */
    BT_HS_BATTCHG5    = 17,    /* remote control channel open by peer */

    BT_HS_SIGNAL0        = 18,    /* signal info*/
    BT_HS_SIGNAL1     = 19,
    BT_HS_SIGNAL2     = 20,
    BT_HS_SIGNAL3            = 21,
    BT_HS_SIGNAL4           = 22,
    BT_HS_SIGNAL5           = 23,

    BT_HS_ROAM_OFF             = 24,/* roam off*/
    BT_HS_ROAM_ON         = 25,
}BT_HFP_CIEV_STATUS;


struct bt_hfp_ciev_status_id
{
     uint8_t             call_ind_id;
     uint8_t             call_setup_ind_id;
     uint8_t             service_ind_id;
     uint8_t             battery_ind_id;
     uint8_t             signal_strength_ind_id;
     uint8_t             callheld_ind_id;
     uint8_t             roam_ind_id;

     uint8_t             curr_call_ind;
     uint8_t             curr_call_setup_ind;
     uint8_t             curr_service_ind;
     uint8_t             curr_battery_ind;
     uint8_t             curr_signal_strength_ind;
     uint8_t             curr_callheld_ind;
     uint8_t             curr_roam_ind;
};

typedef struct
{
    uint16_t status;
    BTAddr bd_addr;
    uint8_t idx;              /* connect idx device*/
    char name[249]; /* Name of peer device. */
} BT_HFP_OPEN_MSG;

typedef BT_HFP_OPEN_MSG BT_HFP_CLOSE_MSG;

typedef struct
{
    uint16_t status;
    BTAddr bd_addr;
    uint8_t idx;              /* connect idx device*/
    char name[249]; /* Name of peer device. */
    uint32_t service;    /* profile used for connection */
    uint16_t peer_features;
} BT_HFP_CONN_MSG;

typedef struct
{
    struct bt_hfp_ciev_status_id status;
} BT_HFP_CIND_MSG;

typedef struct
{
    BT_HFP_CIEV_STATUS status;
} BT_HFP_CIEV_MSG;

typedef struct
{
    char *data;
    int need_len;
    int get_len;
    int bits;
    int channel;
    int rate;
} BT_HFP_FEED_MIC_MSG;

/* union of data associated with HFP callback */
typedef union
{
    BT_HFP_OPEN_MSG open;
    BT_HFP_CLOSE_MSG close;
    BT_HFP_CONN_MSG conn;
    BT_HFP_FEED_MIC_MSG feed_mic;
    BT_HFP_CIND_MSG cind;
    BT_HFP_CIEV_MSG ciev;
} BT_HFP_MSG;


/*HS callback events */
typedef enum
{
    BT_HS_OPEN_EVT,                    /* HS connection open or connection attempt failed  */
    BT_HS_CLOSE_EVT,                   /* HS connection closed */
    BT_HS_CONN_EVT,                    /* HS Service Level Connection is UP */
    BT_HS_CONN_LOSS_EVT,               /* Link loss of connection to audio gateway happened */
    BT_HS_AUDIO_OPEN_REQ_EVT,          /* Audio open request*/
    BT_HS_AUDIO_OPEN_EVT,              /* Audio connection open */
    BT_HS_AUDIO_CLOSE_EVT,             /* Audio connection closed */
    BT_HS_FEED_MIC_EVT,

    BT_HS_CIND_EVT  = 11, /* Indicator string from AG */
    BT_HS_CIEV_EVT, /* Indicator status from AG */
    BT_HS_RING_EVT, /* RING alert from AG */
    BT_HS_CLIP_EVT, /* Calling subscriber information from AG */
    BT_HS_BSIR_EVT, /* In band ring tone setting */
    BT_HS_BVRA_EVT, /* Voice recognition activation/deactivation */
    BT_HS_CCWA_EVT, /* Call waiting notification */
    BT_HS_CHLD_EVT, /* Call hold and multi party service in AG */
    BT_HS_VGM_EVT,  /* MIC volume setting */
    BT_HS_VGS_EVT,  /* Speaker volume setting */
    BT_HS_BINP_EVT, /* Input data response from AG */
    BT_HS_BTRH_EVT, /* CCAP incoming call hold */
    BT_HS_CNUM_EVT, /* Subscriber number */
    BT_HS_COPS_EVT, /* Operator selection info from AG */
    BT_HS_CMEE_EVT, /* Enhanced error result from AG */
    BT_HS_CLCC_EVT, /* Current active call list info */
    BT_HS_UNAT_EVT, /* AT command response fro AG which is not specified in HFP or HSP */
    BT_HS_OK_EVT,   /* OK response */
    BT_HS_ERROR_EVT, /* ERROR response */
    BT_HS_BCS_EVT,  /* Codec selection from AG */
} BT_HS_EVT;


#ifdef  __cplusplus
}
#endif

#endif /* ____BT_HFP_H____ */

#ifndef __BT_A2DP_H__
#define __BT_A2DP_H__

#include "bt_type.h"
#ifdef __cplusplus
extern "C" {
#endif



typedef enum
{
    BT_A2DP_CODEC_TYPE_PCM,
    BT_A2DP_CODEC_TYPE_SBC,
    BT_A2DP_CODEC_TYPE_APTX,
    BT_A2DP_CODEC_TYPE_AAC,
    BT_A2DP_CODEC_TYPE_SEC,
} BT_A2DP_CODEC_TYPE;


/* data associated with BT_A2DP_OPEN_EVT */
typedef struct
{
    uint16_t status;
    BTDevice dev;
    //bool cp_supported;
    //bool aptx_supported; /* apt-X supported */
    //bool aptx_cp_supported; /* apt-X supported */
    //bool sec_supported; /* SEC supported */
} BT_A2DP_OPEN_MSG;

typedef struct
{
    BTAddr         BTAddr;
} BT_A2DP_PEND_MSG;

/* data associated with BT_A2DP_CLOSE_EVT */
typedef struct
{
    uint16_t status;
    BTDevice dev;
} BT_A2DP_CLOSE_MSG;

/* data associated with BT_A2DP_DELAY_RPT_EVT */
typedef struct
{
    //uint8_t channel;
    //uint8_t handle;
    uint16_t delay;
} BT_A2DP_DELAY_MSG;

/* data associated with BT_A2DP_START_EVT */
typedef struct
{
    uint16_t status;
    //uint8_t channel;
    //uint8_t uipc_channel;
    //tBT_A2DP_MEDIA_FEEDINGS media_feeding;
    //bool cp_enabled;
    //uint8_t cp_flag;
} BT_A2DP_START_MSG;

/* data associated with BT_A2DP_STOP_EVT */
typedef struct
{
    uint16_t status;
    //uint8_t channel;
    bool pause;
    //uint8_t uipc_channel;
} BT_A2DP_STOP_MSG;

/* data associated with BT_A2DP_RC_OPEN_EVT */
typedef struct
{
    uint16_t status;
    //uint8_t rc_handle;
    //uint16_t peer_features;
    //BTAddr peer_addr;
} BT_A2DP_RC_OPEN_MSG;

/* data associated with BT_A2DP_RC_CLOSE_EVT */
typedef struct
{
    uint16_t status;
    //uint8_t rc_handle;
    //BTAddr peer_addr;
} BT_A2DP_RC_CLOSE_MSG;

/* data associated with BT_A2DP_REMOTE_CMD_EVT */
typedef struct
{
    //uint8_t rc_handle;
    uint8_t rc_id;
    //uint8_t key_state;
} BT_A2DP_REMOTE_CMD_MSG;

/* data associated with BT_A2DP_REMOTE_RSP_EVT */
typedef struct
{
    //uint8_t rc_handle;
    uint8_t rc_id;
    //uint8_t key_state;
} BT_A2DP_REMOTE_RSP_MSG;

/* data associated with BT_A2DP_VENDOR_CMD_EVT */
typedef struct
{
    //uint8_t           rc_handle;
    uint16_t          len;            /* Max vendor dependent message is BT_A2DP_VENDOR_SIZE_MAX */
    uint8_t           label;
    uint8_t     code;
    uint32_t          company_id;
    //uint8_t           data[BT_A2DP_VENDOR_SIZE_MAX];
} BT_A2DP_VENDOR_CMD_MSG;

typedef BT_A2DP_VENDOR_CMD_MSG BT_A2DP_VENDOR_RSP_MSG;

/* GetElemAttrs */
typedef struct
{
    uint8_t       pdu;
    uint8_t       opcode;                         /* Op Code (assigned by AVRC_BldCommand according to pdu) */
    uint8_t       num_attr;
    uint8_t       label;
    //uint32_t      attrs[BSA_AVRC_MAX_ATTR_COUNT];
} BT_A2DP_META_GET_ELEM_ATTRS_CMD;

/* GetFolderItems */
typedef struct
{
    uint8_t       scope;
    uint32_t      start_item;
    uint32_t      end_item;
    uint8_t       attr_count;
} BT_A2DP_META_GET_FOLDER_ITEMS_CMD;

/* RegNotify */
typedef struct
{
    uint8_t       event_id;
    uint32_t      param;
} BT_A2DP_REG_NOTIF_CMD;

/* GetItemAttr */
typedef struct
{
    uint8_t       pdu;
    uint8_t       opcode;                         /* Op Code (assigned by AVRC_BldCommand according to pdu) */
    uint8_t       scope;
    //tBSA_UID   uid;
    uint16_t      uid_counter;
    uint8_t       num_attr;
    uint8_t       label;
    //uint32_t      attrs[BSA_AVRC_MAX_ATTR_COUNT];
} BT_A2DP_META_GET_ITEM_ATTRS_CMD;

/* data associated with BT_A2DP_META_MSG_EVT */
typedef struct
{
    //uint8_t rc_handle;
    uint8_t           opcode;                         /* Op Code (assigned by AVRC_BldCommand according to pdu) */
    uint8_t           pdu;
    uint8_t           label;
    uint32_t          company_id;
    uint16_t          player_id;

    union{
    BT_A2DP_META_GET_ELEM_ATTRS_CMD     get_elem_attrs;
    BT_A2DP_META_GET_FOLDER_ITEMS_CMD   get_folder_items;
    BT_A2DP_REG_NOTIF_CMD               reg_notif;
    //tBSA_CHG_PATH_CMD                  change_path;
    //tBSA_PLAY_ITEM_CMD                 play_item;
    BT_A2DP_META_GET_ITEM_ATTRS_CMD     get_item_attrs;
    }param;

} BT_A2DP_META_MSG_MSG;

typedef struct
{
    uint8_t              rc_handle;
    uint16_t       peer_features;
    //tBTA_AV_RC_INFO    tg;                 /* Peer TG role info */
    //tBTA_AV_RC_INFO    ct;                 /* Peer CT role info */
} BT_A2DP_RC_FEAT_MSG;


/* union of data associated with AV callback */
typedef union
{
    BT_A2DP_OPEN_MSG open;
    BT_A2DP_CLOSE_MSG close;
    BT_A2DP_START_MSG start;
    BT_A2DP_STOP_MSG stop;
    BT_A2DP_RC_OPEN_MSG rc_open;
    BT_A2DP_RC_CLOSE_MSG rc_close;
    BT_A2DP_REMOTE_CMD_MSG remote_cmd;
    BT_A2DP_REMOTE_RSP_MSG remote_rsp;
    BT_A2DP_VENDOR_CMD_MSG vendor_cmd;
    BT_A2DP_VENDOR_RSP_MSG vendor_rsp;
    BT_A2DP_META_MSG_MSG meta_msg;
    BT_A2DP_PEND_MSG       pend;
    BT_A2DP_RC_FEAT_MSG rc_feat;
    BT_A2DP_DELAY_MSG delay;
} BT_A2DP_MSG;

/*A2DP callback events */
typedef enum
{
    BT_A2DP_OPEN_EVT,                /* connection opened */
    BT_A2DP_CLOSE_EVT,               /* connection closed */
    BT_A2DP_START_EVT,               /* stream data transfer started */
    BT_A2DP_STOP_EVT,                /* stream data transfer stopped */
    BT_A2DP_RC_OPEN_EVT,             /* remote control channel open */
    BT_A2DP_RC_CLOSE_EVT,            /* remote control channel closed */
    BT_A2DP_REMOTE_CMD_EVT,          /* remote control command */
    BT_A2DP_REMOTE_RSP_EVT,          /* remote control response */
    BT_A2DP_VENDOR_CMD_EVT,          /* vendor specific command */
    BT_A2DP_VENDOR_RSP_EVT,          /* vendor specific response */
    BT_A2DP_META_MSG_EVT,           /* metadata messages */
    BT_A2DP_PENDING_EVT,           /* incoming connection pending:
                                         * signal channel is open and stream is not open
                                         * after BTA_AV_SIG_TIME_VAL ms */
    BT_A2DP_FEAT_EVT,               /* Peer feature event */
    BT_A2DP_DELAY_RPT_EVT,          /* Delay report event */
} BT_A2DP_EVT;

#ifdef __cplusplus
}
#endif
#endif /* __BT_A2DP_H__ */

#ifndef __BT_A2DP_SINK_H__
#define __BT_A2DP_SINK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "bt_type.h"
#include "bt_avrc.h"


/* AVK signaling channel open */
/* data associated with BT_AVK_OPEN_MSG */
typedef struct
{
    uint16_t status;
    BTAddr bd_addr;
    uint8_t idx;              /* connect idx device*/
    char name[249]; /* Name of peer device. */
} BT_AVK_OPEN_MSG;

/* AVK signaling channel close */
/* data associated with BT_AVK_CLOSE_MSG */
typedef BT_AVK_OPEN_MSG BT_AVK_CLOSE_MSG;
typedef BT_AVK_OPEN_MSG BT_AVK_STR_OPEN_MSG;
typedef BT_AVK_OPEN_MSG BT_AVK_STR_CLOSE_MSG;

/* AVK start streaming */
/* data associated with BT_AVK_START_MSG */
typedef struct
{
    uint16_t status;
    //tBSA_AVK_MEDIA_RECEIVING media_receiving;
    BTAddr         bd_addr;
    uint8_t idx;              /* connect idx device*/
    //bool     discarded; /* TRUE if received stream is discarded because another stream is active */
} BT_AVK_START_MSG;

/* AVK stop streaming */
/* data associated with BT_AVK_STOP_MSG */
typedef struct
{
    uint16_t  status;
    BTAddr         bd_addr;
    bool     suspended;   /* TRUE if the event occured as a result of stream got suspended */
    uint8_t idx;              /* connect idx device*/
} BT_AVK_STOP_MSG;

/* data associated with BT_AVK_RC_OPEN_MSG */
typedef struct
{
    //uint16_t peer_features;
    //uint16_t peer_version; /* peer AVRCP version */
    uint16_t  status;
    BTAddr  bd_addr;
    uint8_t idx;              /* connect idx device*/
} BT_AVK_RC_OPEN_MSG;

/* data associated with BT_AVK_RC_CLOSE_MSG */
typedef BT_AVK_RC_OPEN_MSG BT_AVK_RC_CLOSE_MSG;


/* data associated with BT_AVK_REMOTE_RSP_MSG */
typedef struct
{
    uint8_t rc_id;
    uint8_t key_state;
    uint8_t len;
} BT_AVK_REMOTE_RSP_MSG;


/* Data for BT_AVK_REMOTE_CMD_EVT */
typedef struct
{
    uint8_t           label;
    uint8_t           op_id;          /* Operation ID (see AVRC_ID_* defintions in avrc_defs.h) */
    uint8_t           key_state;      /* AVRC_STATE_PRESS or AVRC_STATE_RELEASE */
    //tAVRC_HDR       hdr;            /* Message header. */
    uint8_t           len;
    //uint8_t           data[BT_AVK_REMOTE_CMD_MAX];
} BT_AVK_REMOTE_REQ_MSG;

/* data type for BTA_AVK_API_VENDOR_CMD_EVT and RSP */
typedef struct
{
    uint16_t          len;
    uint8_t           label;
    uint8_t    code;
    uint32_t          company_id;
    //uint8_t           data[BT_AVK_VENDOR_SIZE_MAX];
} BT_AVK_VENDOR_CMD_MSG;

typedef BT_AVK_VENDOR_CMD_MSG BT_AVK_VENDOR_RSP_MSG;

/* data associated with BT_AVK_CP_INFO_MSG */
typedef struct
{
    uint16_t id;
    union {
        uint8_t  scmst_flag;
    } info;
} BT_AVK_CP_INFO_MSG;

#define BT_AVK_MEDIA_MAX_BUF_LEN (128*14)

/* data associated with UIPC media data */
typedef struct
{
    //BT_HDR          hdr;
    //uint8_t           multimedia[BT_AVK_MEDIA_MAX_BUF_LEN];
    uint8_t                   handle;
} BT_AVK_MEDIA;

/* data for meta data items */
#define BT_AVK_ATTR_STR_LEN_MAX 102

/* attibute entry */
typedef struct
{
    uint32_t             attr_id;
    char    data[BT_AVK_ATTR_STR_LEN_MAX];
} BT_AVK_ATTR_ENTRY;

/* data for get element attribute response */
#define BT_AVK_ELEMENT_ATTR_MAX 7

typedef struct
{
    uint8_t                   num_attr;
    BT_AVK_ATTR_ENTRY     attr_entry[BT_AVK_ELEMENT_ATTR_MAX];
} BT_AVK_GET_ELEMENT_ATTR_MSG;

/* data for current app settings response */
#define BT_AVK_APP_SETTING_MAX 6

typedef struct
{
    uint8_t       pdu;
    uint8_t   status;
    uint8_t       opcode;         /* Op Code (copied from avrc_cmd.opcode by AVRC_BldResponse user. invalid one to generate according to pdu) */
    uint8_t       num_val;
    //tAVRC_APP_SETTING   vals[BT_AVK_APP_SETTING_MAX];
} BT_GET_CUR_APP_VALUE_MSG;

/* data for set browsed player response */
typedef struct
{
    uint8_t               status;
    uint16_t                  uid_counter;
    uint32_t                  num_items;
    uint8_t                   folder_depth;
    //tBT_AVK_STRING         folder;
    bool                 final; /*true if last entry*/
} BT_AVK_SET_BR_PLAYER_MSG;

/* player item data */
typedef struct
{
    uint16_t              player_id;      /* A unique identifier for this media player.*/
    uint8_t               major_type;     /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    uint32_t              sub_type;       /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    uint8_t               play_status;    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
                                            AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    //tAVRC_FEATURE_MASK  features;       /* Supported feature bit mask*/
    //tBT_AVK_STRING     name;           /* The player name, name length and character set id.*/
} BT_AVRC_ITEM_PLAYER;

/* folder item data */
typedef struct
{
    //tAVRC_UID           uid;            /* The uid of this folder */
    uint8_t               type;           /* Use AVRC_FOLDER_TYPE_MIXED, AVRC_FOLDER_TYPE_TITLES,
                                           AVRC_FOLDER_TYPE_ALNUMS, AVRC_FOLDER_TYPE_ARTISTS, AVRC_FOLDER_TYPE_GENRES,
                                           AVRC_FOLDER_TYPE_PLAYLISTS, or AVRC_FOLDER_TYPE_YEARS.*/
    bool             playable;       /* TRUE, if the folder can be played. */
    //tBT_AVK_STRING     name;           /* The folder name, name length and character set id. */
} BT_AVRC_ITEM_FOLDER;

/* media item data */
typedef struct
{
    //tAVRC_UID           uid;            /* The uid of this media element item */
    uint8_t               type;           /* Use AVRC_MEDIA_TYPE_AUDIO or AVRC_MEDIA_TYPE_VIDEO. */
    //tBT_AVK_STRING     name;           /* The media name, name length and character set id. */
    uint8_t               attr_count;     /* The number of attributes in p_attr_list */
    //tBT_AVK_ATTR_ENTRY attr_entry[BT_AVK_ELEMENT_ATTR_MAX];    /* Attribute entry list. */
} BT_AVRC_ITEM_MEDIA;

/* avrcp item (player,folder or media) */
typedef struct
{
    uint8_t                   item_type;  /* AVRC_ITEM_PLAYER, AVRC_ITEM_FOLDER, or AVRC_ITEM_MEDIA */
    union
    {
        BT_AVRC_ITEM_PLAYER   player;     /* The properties of a media player item.*/
        BT_AVRC_ITEM_FOLDER   folder;     /* The properties of a folder item.*/
        BT_AVRC_ITEM_MEDIA    media;      /* The properties of a media item.*/
    } u;
} BT_AVRC_ITEM;

/* data for get folder items response */
typedef struct
{
    uint8_t               status;
    uint16_t                  uid_counter;
    uint16_t                  item_count;
    BT_AVRC_ITEM          item;
    bool                 final; /*true if last entry*/
} BT_AVK_GET_ITEMS_MSG;

/* data for registered notification response */
typedef struct
{
    BT_AVRC_REG_NOTIF_RSP     rsp;
} BT_AVK_REG_NOTIF_MSG;

/* data for list app attr response */
typedef struct
{
    //tAVRC_LIST_APP_ATTR_RSP     rsp;
    uint8_t                   handle;
} BT_AVK_LIST_APP_ATTR_MSG;

/* data for list app attr value response */
typedef struct
{
    //tAVRC_LIST_APP_VALUES_RSP     rsp;
    uint8_t                   handle;
} BT_AVK_LIST_APP_VALUES_MSG;

/* data for set player app value response */
typedef struct
{
    //tAVRC_RSP     rsp;
    uint8_t         handle;
} BT_AVK_SET_PLAYER_APP_VALUE_MSG;

/* data for set play status response */
typedef struct
{
    uint8_t         play_status;
} BT_AVK_GET_PLAY_STATUS_MSG;

/* data for set addressed player response */
typedef struct
{
    //tAVRC_RSP     rsp;
    uint8_t         handle;
} BT_AVK_SET_ADDRESSED_PLAYER_MSG;

/* data for set change path response */
typedef struct
{
    //tAVRC_CHG_PATH_RSP  rsp;
    uint8_t               handle;
} BT_AVK_CHG_PATH_MSG;

/* data for get element attr response */
typedef struct
{
    BT_AVK_GET_ELEMENT_ATTR_MSG  rsp;
} BT_AVK_GET_ITEM_ATTR_MSG;

/* data for play item response */
typedef struct
{
    //tAVRC_RSP  rsp;
    uint8_t      handle;
} BT_AVK_PLAY_ITEM_MSG;

/* data for now playing response */
typedef struct
{
    //tAVRC_RSP  rsp;
    uint8_t      handle;
} BT_AVK_NOW_PLAYING_MSG;

/* data for abs volume command */
typedef struct
{
    //uint8_t    pdu;
    //uint8_t       opcode;         /* Op Code (assigned by AVRC_BldCommand according to pdu) */
    uint8_t       volume;
} BT_AVK_ABS_VOLUME_CMD_MSG;

/* data for register notification cmd */
typedef struct
{
    //tAVRC_REG_NOTIF_CMD reg_notif_cmd;
    uint8_t               label;
} BT_AVK_REG_NOTIF_CMD_MSG;

/* union of data associated with AV callback */
typedef union
{
    BT_AVK_OPEN_MSG sig_chnl_open;
    BT_AVK_CLOSE_MSG sig_chnl_close;
    BT_AVK_STR_OPEN_MSG stream_chnl_open;
    BT_AVK_STR_CLOSE_MSG stream_chnl_close;
    BT_AVK_START_MSG start_streaming;
    BT_AVK_STOP_MSG stop_streaming;
    BT_AVK_RC_OPEN_MSG rc_open;
    BT_AVK_RC_CLOSE_MSG rc_close;
    BT_AVK_REMOTE_REQ_MSG remote_cmd;
    BT_AVK_REMOTE_RSP_MSG remote_rsp;
    BT_AVK_VENDOR_CMD_MSG vendor_cmd;
    BT_AVK_VENDOR_RSP_MSG vendor_rsp;
    BT_AVK_CP_INFO_MSG cp_info;

    /* meta commands responses */
    BT_AVK_REG_NOTIF_MSG              reg_notif;
    BT_AVK_REG_NOTIF_CMD_MSG          reg_notif_cmd;
    BT_AVK_LIST_APP_ATTR_MSG          list_app_attr;
    BT_AVK_LIST_APP_VALUES_MSG        list_app_values;
    BT_AVK_SET_PLAYER_APP_VALUE_MSG   set_app_val;
    BT_GET_CUR_APP_VALUE_MSG          get_cur_app_val;
    BT_AVK_GET_ELEMENT_ATTR_MSG       elem_attr;
    BT_AVK_GET_PLAY_STATUS_MSG        get_play_status;
    BT_AVK_ABS_VOLUME_CMD_MSG         abs_volume;

    /* browsing command responses */
    BT_AVK_SET_ADDRESSED_PLAYER_MSG   addr_player;
    BT_AVK_SET_BR_PLAYER_MSG          br_player;
    BT_AVK_GET_ITEMS_MSG              get_items;
    BT_AVK_CHG_PATH_MSG               chg_path;
    BT_AVK_GET_ITEM_ATTR_MSG          item_attr;
    BT_AVK_PLAY_ITEM_MSG              play_item;
    BT_AVK_NOW_PLAYING_MSG            now_playing;
} BT_AVK_MSG;

/*A2DP_SINK callback events */
typedef enum
{
    BT_AVK_OPEN_EVT =         2,       /* connection opened */
    BT_AVK_CLOSE_EVT =        3,       /* connection closed */
    BT_AVK_START_EVT =        4,       /* stream data transfer started */
    BT_AVK_STOP_EVT =         5,       /* stream data transfer stopped */
    BT_AVK_STR_OPEN_EVT =     6,       /* stream channel opened */
    BT_AVK_STR_CLOSE_EVT =    7,       /* stream channel closed */
    BT_AVK_RC_OPEN_EVT =      8,       /* remote control channel open */
    //BT_AVK_RC_PEER_OPEN_EVT = 9,       /* remote control channel open by peer */
    BT_AVK_RC_CLOSE_EVT =     10,      /* remote control channel closed */
    BT_AVK_REMOTE_CMD_EVT =   11,      /* remote control command */
    BT_AVK_REMOTE_RSP_EVT =   12,      /* remote control response */
    BT_AVK_VENDOR_CMD_EVT =   13,      /* vendor dependent remote control command */
    BT_AVK_VENDOR_RSP_EVT =   14,      /* vendor dependent remote control response */
    BT_AVK_CP_INFO_EVT =      18,      /* content protection message */

    BT_AVK_REGISTER_NOTIFICATION_EVT =   19,      /* reg notfn response */
    BT_AVK_LIST_PLAYER_APP_ATTR_EVT =    20,     /* list player attr response */
    BT_AVK_LIST_PLAYER_APP_VALUES_EVT =  21,      /* list player value response */
    BT_AVK_SET_PLAYER_APP_VALUE_EVT =    22,      /* set player value response */
    BT_AVK_GET_PLAYER_APP_VALUE_EVT =    23,      /* get player value response */
    BT_AVK_GET_PLAYER_ATTR_TEXT_EVT =    24,      /* get player attr text response */
    BT_AVK_GET_PLAYER_ATTR_VALUE_EVT =   25,      /* get player value text response */
    BT_AVK_GET_ELEM_ATTR_EVT =           26,      /* get element attrubute response */
    BT_AVK_GET_PLAY_STATUS_EVT =         27,      /* get plays status response */
    BT_AVK_SET_ABSOLUTE_VOLUME_EVT =     28,      /* set abs vol esponse */
    BT_AVK_SET_ADDRESSED_PLAYER_EVT =    29,      /* set addressed player response */
    BT_AVK_SET_BROWSED_PLAYER_EVT =      30,      /* set browsed player response */
    BT_AVK_GET_FOLDER_ITEMS_EVT =        31,      /* get folder items response */
    BT_AVK_CHANGE_PATH_EVT =             32,      /* change path response */
    BT_AVK_GET_ITEM_ATTR_EVT =           33,      /* get item attr response */
    BT_AVK_PLAY_ITEM_EVT =               34,      /* play item response */
    BT_AVK_ADD_TO_NOW_PLAYING_EVT =      36,      /* add to now playing response */
    BT_AVK_SET_ABS_VOL_CMD_EVT =         37,      /* set abs vol command */
    BT_AVK_REG_NOTIFICATION_CMD_EVT =    38,      /* reg notification cmd */
} BT_A2DP_SINK_EVT;

#ifdef __cplusplus
}
#endif
#endif/* __BT_A2DP_SINK_H__ */

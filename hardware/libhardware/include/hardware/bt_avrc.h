#ifndef __BT_AVRC_H__
#define __BT_AVRC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "bt_type.h"

/* Define the events that can be registered for notifications
*/
#define BT_AVRC_EVT_PLAY_STATUS_CHANGE             0x01
#define BT_AVRC_EVT_TRACK_CHANGE                   0x02
#define BT_AVRC_EVT_TRACK_REACHED_END              0x03
#define BT_AVRC_EVT_TRACK_REACHED_START            0x04
#define BT_AVRC_EVT_PLAY_POS_CHANGED               0x05
#define BT_AVRC_EVT_BATTERY_STATUS_CHANGE          0x06
#define BT_AVRC_EVT_SYSTEM_STATUS_CHANGE           0x07
#define BT_AVRC_EVT_APP_SETTING_CHANGE             0x08
/* added in AVRCP 1.4 */
#define BT_AVRC_EVT_NOW_PLAYING_CHANGE             0x09
#define BT_AVRC_EVT_AVAL_PLAYERS_CHANGE            0x0a
#define BT_AVRC_EVT_ADDR_PLAYER_CHANGE             0x0b
#define BT_AVRC_EVT_UIDS_CHANGE                    0x0c
#define BT_AVRC_EVT_VOLUME_CHANGE                  0x0d
typedef uint8_t BT_AVRC_EVT;

/* operation id list for RemoteCmd */
#define BT_AVRC_SELECT        0x00      /* select */
#define BT_AVRC_UP            0x01          /* up */
#define BT_AVRC_DOWN          0x02        /* down */
#define BT_AVRC_LEFT          0x03        /* left */
#define BT_AVRC_RIGHT         0x04       /* right */
#define BT_AVRC_RIGHT_UP      0x05    /* right-up */
#define BT_AVRC_RIGHT_DOWN    0x06  /* right-down */
#define BT_AVRC_LEFT_UP       0x07     /* left-up */
#define BT_AVRC_LEFT_DOWN     0x08   /* left-down */
#define BT_AVRC_ROOT_MENU     0x09   /* root menu */
#define BT_AVRC_SETUP_MENU    0x0A  /* setup menu */
#define BT_AVRC_CONT_MENU     0x0B   /* contents menu */
#define BT_AVRC_FAV_MENU      0x0C    /* favorite menu */
#define BT_AVRC_EXIT          0x0D        /* exit */
#define BT_AVRC_0             0x20           /* 0 */
#define BT_AVRC_1             0x21           /* 1 */
#define BT_AVRC_2             0x22           /* 2 */
#define BT_AVRC_3             0x23           /* 3 */
#define BT_AVRC_4             0x24           /* 4 */
#define BT_AVRC_5             0x25           /* 5 */
#define BT_AVRC_6             0x26           /* 6 */
#define BT_AVRC_7             0x27           /* 7 */
#define BT_AVRC_8             0x28           /* 8 */
#define BT_AVRC_9             0x29           /* 9 */
#define BT_AVRC_DOT           0x2A         /* dot */
#define BT_AVRC_ENTER         0x2B       /* enter */
#define BT_AVRC_CLEAR         0x2C       /* clear */
#define BT_AVRC_CHAN_UP       0x30     /* channel up */
#define BT_AVRC_CHAN_DOWN     0x31   /* channel down */
#define BT_AVRC_PREV_CHAN     0x32   /* previous channel */
#define BT_AVRC_SOUND_SEL     0x33   /* sound select */
#define BT_AVRC_INPUT_SEL     0x34   /* input select */
#define BT_AVRC_DISP_INFO     0x35   /* display information */
#define BT_AVRC_HELP          0x36        /* help */
#define BT_AVRC_PAGE_UP       0x37     /* page up */
#define BT_AVRC_PAGE_DOWN     0x38   /* page down */
#define BT_AVRC_POWER         0x40       /* power */
#define BT_AVRC_VOL_UP        0x41      /* volume up */
#define BT_AVRC_VOL_DOWN      0x42    /* volume down */
#define BT_AVRC_MUTE          0x43        /* mute */
#define BT_AVRC_PLAY          0x44        /* play */
#define BT_AVRC_STOP          0x45        /* stop */
#define BT_AVRC_PAUSE         0x46       /* pause */
#define BT_AVRC_RECORD        0x47      /* record */
#define BT_AVRC_REWIND        0x48      /* rewind */
#define BT_AVRC_FAST_FOR      0x49    /* fast forward */
#define BT_AVRC_EJECT         0x4A       /* eject */
#define BT_AVRC_FORWARD       0x4B     /* forward */
#define BT_AVRC_BACKWARD      0x4C    /* backward */
#define BT_AVRC_ANGLE         0x50       /* angle */
#define BT_AVRC_SUBPICT       0x51     /* subpicture */
#define BT_AVRC_F1            0x71          /* F1 */
#define BT_AVRC_F2            0x72          /* F2 */
#define BT_AVRC_F3            0x73          /* F3 */
#define BT_AVRC_F4            0x74          /* F4 */
#define BT_AVRC_F5            0x75          /* F5 */


/* Define the possible values of play state
*/
#define BT_AVRC_PLAYSTATE_RESP_MSG_SIZE            9
#define BT_AVRC_PLAYSTATE_STOPPED                  0x00    /* Stopped */
#define BT_AVRC_PLAYSTATE_PLAYING                  0x01    /* Playing */
#define BT_AVRC_PLAYSTATE_PAUSED                   0x02    /* Paused  */
#define BT_AVRC_PLAYSTATE_FWD_SEEK                 0x03    /* Fwd Seek*/
#define BT_AVRC_PLAYSTATE_REV_SEEK                 0x04    /* Rev Seek*/
#define BT_AVRC_PLAYSTATE_ERROR                    0xFF    /* Error   */



#define BT_AVRC_MEDIA_ATTR_ID_TITLE                 0x00000001
#define BT_AVRC_MEDIA_ATTR_ID_ARTIST                0x00000002
#define BT_AVRC_MEDIA_ATTR_ID_ALBUM                 0x00000003
#define BT_AVRC_MEDIA_ATTR_ID_TRACK_NUM             0x00000004
#define BT_AVRC_MEDIA_ATTR_ID_NUM_TRACKS            0x00000005
#define BT_AVRC_MEDIA_ATTR_ID_GENRE                 0x00000006
#define BT_AVRC_MEDIA_ATTR_ID_PLAYING_TIME          0x00000007        /* in miliseconds */



typedef struct
{
    uint8_t               num_attr;
    uint8_t               attr_id[8];
    uint8_t               attr_value[8];
} BT_AVRC_PLAYER_APP_PARAM;

typedef struct
{
    uint16_t              player_id;
    uint16_t              uid_counter;
} BT_AVRC_ADDR_PLAYER_PARAM;

typedef union
{
    uint8_t         play_status;
    uint8_t               track[8];
    uint32_t                  play_pos;
    uint8_t    battery_status;
    uint8_t       system_status;
    BT_AVRC_PLAYER_APP_PARAM  player_setting;
    BT_AVRC_ADDR_PLAYER_PARAM addr_player;
    uint16_t                  uid_counter;
    uint8_t                   volume;
} BT_AVRC_NOTIF_RSP_PARAM;

/* RegNotify */
typedef struct
{
    uint8_t                   pdu;
    uint8_t               status;
    uint8_t                   opcode;         /* Op Code (copied from avrc_cmd.opcode by AVRC_BldResponse user. invalid one to generate according to pdu) */
    uint8_t                   event_id;
    BT_AVRC_NOTIF_RSP_PARAM   param;
} BT_AVRC_REG_NOTIF_RSP;


#ifdef __cplusplus
}
#endif
#endif/* __BT_AVRC_H__ */

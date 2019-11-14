#ifndef __BT_HH_H__
#define __BT_HH_H__

#include "bt_type.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    uint16_t length;
    void *data;
} BT_HH_REPORT_DATA;


typedef struct
{
    //uint16_t report_len;
    BTAddr bd_addr;
    //uint8_t sub_class;
    //uint8_t ctry_code;
    BT_HH_REPORT_DATA report_data;
} BT_HH_REPORT_MSG;


/* callback event data for BSA_HH_OPEN_EVT event */
typedef struct
{
    uint16_t status; /* operation status */
    char name[249];
    BTAddr bd_addr; /* HID device bd address */
    uint16_t mode; /* Operation mode (Report/Boot) */
    //uint8_t sub_class;         /* sub class */
    //uint16_t attr_mask; /* attribute mask */
    //bool le_hid;          /* LE hid */
} BT_HH_OPEN_MSG;

/* callback event data for BSA_HH_CLOSE_EVT event */
typedef struct
{
    uint16_t status; /* operation status */
    BTAddr bd_addr; 
} BT_HH_CLOSE_MSG;

/* Union of data associated with HD callback */
typedef union
{
    BT_HH_REPORT_MSG         report;/* BT_HH_REPORT_EVT */

    BT_HH_OPEN_MSG open; /* BT_HH_OPEN_EVT */
    BT_HH_CLOSE_MSG close; /* BT_HH_CLOSE_EVT */
} BT_HH_MSG;


/*hh callback events */
typedef enum
{
    BT_HH_REPORT_EVT,
    BT_HH_OPEN_EVT,
    BT_HH_CLOSE_EVT,
    BT_HH_GET_REPORT_EVT,
    BT_HH_SET_REPORT_EVT,
} BT_HH_EVT;


#ifdef __cplusplus
}
#endif
#endif /* __BT_HH_H__ */


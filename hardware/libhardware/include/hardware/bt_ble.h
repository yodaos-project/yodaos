#ifndef __BT_BLE_H__
#define __BT_BLE_H__

#include "bt_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_CHAR_ID1 0x2A06
#define BLE_CHAR_ID2 0x2A07


/* callback event data for BT_BLE_CL_OPEN_EVT event */
typedef struct
{
    uint16_t         status; /* operation status */
    BTAddr             bd_addr;
    char name[249]; /* Name of peer device. */
} BT_BLE_CL_OPEN_MSG;

/* callback event data for BT_BLE_CL_CLOSE_EVT event */
typedef struct
{
    uint16_t         status;
    BTAddr             bd_addr;
    char name[249]; /* Name of peer device. */
} BT_BLE_CL_CLOSE_MSG;

/* callback event data for BT_BLE_CL_DEREGISTER_EVT event */
typedef struct
{
    uint16_t         status; /* operation status */
    uint8_t         client_if; /* Client interface ID */
} BT_BLE_CL_DEREGISTER_MSG;

/* callback event data for BT_BLE_CL_SEARCH_RES_EVT event */
typedef struct
{
    uint16_t              conn_id;
    //tBTA_GATT_SRVC_ID   service_uuid;
} BT_BLE_CL_SEARCH_RES_MSG;

/* callback event data for BT_BLE_CL_SEARCH_CMPL_EVT event */
typedef struct
{
    uint16_t         status; /* operation status */
    uint16_t              conn_id;
} BT_BLE_CL_SEARCH_CMPL_MSG;

#define MAX_READ_LEN 100
/* callback event data for BT_BLE_CL_READ_EVT event */
typedef struct
{
    uint16_t         status;
    BTAddr             bda;
    char service[MAX_LEN_UUID_STR];
    char char_id[MAX_LEN_UUID_STR];
    char desc_id[MAX_LEN_UUID_STR];
    uint16_t              len;
    uint8_t               value[MAX_READ_LEN];
} BT_BLE_CL_READ_MSG;

/* callback event data for BT_BLE_CL_WRITE_EVT event */
typedef struct
{
    uint16_t         status;
    //tBTA_GATT_SRVC_ID   srvc_id;
    //tBSA_BLE_ID         char_id;
    //tBTA_GATT_ID        descr_type;
} BT_BLE_CL_WRITE_MSG;

#define BT_BLE_MAX_ATTR_LEN 600
/* callback event data for BT_BLE_CL_NOTIF_EVT event */
typedef struct
{
    BTAddr             bda;
    char service[MAX_LEN_UUID_STR];
    char char_id[MAX_LEN_UUID_STR];
    char desc_id[MAX_LEN_UUID_STR];
    uint16_t              len;
    uint8_t               value[BT_BLE_MAX_ATTR_LEN];
} BT_BLE_CL_NOTIF_MSG;

typedef struct
{
    uint16_t              conn_id;
    bool             congested; /* congestion indicator */
}BT_BLE_CL_CONGEST_MSG;

#define BT_BLE_CL_NV_LOAD_MAX   10
/* callback event data for BT_BLE_CL_CACHE_SAVE_EVT event */
/* attributes in one service */
typedef struct
{
    uint16_t               evt;
    uint16_t               num_attr;
    uint16_t               attr_index;
    uint16_t               conn_id;
    BTAddr              bd_addr;
    //tBTA_GATTC_NV_ATTR   attr[BT_BLE_CL_NV_LOAD_MAX];
} BT_BLE_CL_CACHE_SAVE_MSG;

/* callback event data for BT_BLE_CL_CACHE_LOAD_EVT event */
typedef struct
{
    uint16_t              conn_id;
    uint16_t         status;
    BTAddr             bd_addr;
} BT_BLE_CL_CACHE_LOAD_MSG;

/* callback event data for BT_BLE_CL_CFG_MTU_EVT event */
typedef struct
{
    uint16_t         status;
    uint16_t              conn_id;
    uint16_t              mtu;
} BT_BLE_CL_CFG_MTU_MSG;


/* BSA BLE Server Host callback events */
/* Server callback function events */

/* callback event data for BT_BLE_SE_DEREGISTER_EVT event */
typedef struct
{
    uint16_t         status; /* operation status */
    uint8_t         server_if; /* Server interface ID */
} BT_BLE_SE_DEREGISTER_MSG;

/* callback event data for BT_BLE_SE_CREATE_EVT event */
typedef struct
{
    uint8_t         server_if;
    uint16_t              service_id;
    uint16_t         status;
} BT_BLE_SE_CREATE_MSG;

/* callback event data for BT_BLE_SE_ADDCHAR_EVT event */
typedef struct
{
    uint8_t         server_if;
    uint16_t              service_id;
    uint16_t              attr_id;
    uint16_t         status;
    bool             is_discr;
} BT_BLE_SE_ADDCHAR_MSG;

/* callback event data for BT_BLE_SE_START_EVT event */
typedef struct
{
    uint8_t         server_if;
    uint16_t              service_id;
    uint16_t         status;
} BT_BLE_SE_START_MSG;

typedef struct
{
    uint16_t    status;
    uint8_t    server_if;
    uint16_t         service_id;
} BT_BLE_SE_STOP_MSG;

typedef struct
{
    BTAddr       remote_bda;
    uint32_t        trans_id;
    uint16_t        conn_id;
    uint16_t        offset;
    bool       is_long;
    uint16_t   status;
} BT_BLE_SE_READ_MSG;

typedef struct
{
    //BTAddr             remote_bda;
    //uint32_t              trans_id;
    //uint16_t              conn_id;
    //uint16_t              handle;     /* attribute handle */
    uint16_t             uuid;
    /* attribute value offset, if no offset is needed for the command, ignore it */
    uint16_t              offset;
    uint16_t              len;        /* length of attribute value */
    uint8_t               value[BT_BLE_MAX_ATTR_LEN];  /* the actual attribute value */
    //bool             need_rsp;   /* need write response */
    //bool             is_prep;    /* is prepare write */
    uint16_t         status;
} BT_BLE_SE_WRITE_MSG;

typedef struct
{
    BTAddr     remote_bda;
    uint32_t      trans_id;
    uint16_t      conn_id;
    uint8_t     exec_write;
    uint16_t     status;
} BT_BLE_SE_EXEC_WRITE_MSG;

typedef struct
{
    //uint8_t        server_if;
    //BTAddr            remote_bda;
    uint16_t             conn_id;
    uint16_t    reason;
} BT_BLE_SE_OPEN_MSG;

typedef BT_BLE_SE_OPEN_MSG BT_BLE_SE_CLOSE_MSG;

typedef struct
{
    uint16_t              conn_id;
    bool             congested; /* congestion indicator */
}BT_BLE_SE_CONGEST_MSG;

typedef struct
{
    uint16_t             uuid;
    uint16_t              conn_id;    /* connection ID */
    uint8_t    status;     /* notification/indication status */
} BT_BLE_SE_CONF_MSG;

typedef struct
{
    uint16_t              conn_id;
    uint16_t              mtu;
}BT_BLE_SE_MTU_MSG;

/* Union of data associated with HD callback */
typedef union
{
    BT_BLE_CL_OPEN_MSG         cli_open;          /* BT_BLE_CL_OPEN_EVT */
    BT_BLE_CL_SEARCH_RES_MSG   cli_search_res;    /* BT_BLE_CL_SEARCH_RES_EVT */
    BT_BLE_CL_SEARCH_CMPL_MSG  cli_search_cmpl;   /* BT_BLE_CL_SEARCH_CMPL_EVT */
    BT_BLE_CL_READ_MSG         cli_read;          /* BT_BLE_CL_READ_EVT */
    BT_BLE_CL_WRITE_MSG        cli_write;         /* BT_BLE_CL_WRITE_EVT */
    BT_BLE_CL_NOTIF_MSG        cli_notif;         /* BT_BLE_CL_NOTIF_EVT */
    BT_BLE_CL_CONGEST_MSG      cli_congest;       /* BT_BLE_CL_CONGEST_EVT */
    BT_BLE_CL_CLOSE_MSG        cli_close;         /* BT_BLE_CL_CLOSE_EVT */
    BT_BLE_CL_DEREGISTER_MSG   cli_deregister;    /* BT_BLE_CL_DEREGISTER_EVT */
    BT_BLE_CL_CACHE_SAVE_MSG   cli_cache_save;    /* BT_BLE_SE_CACHE_SAVE_EVT */
    BT_BLE_CL_CACHE_LOAD_MSG   cli_cache_load;    /* BT_BLE_SE_CACHE_LOAD_EVT */
    BT_BLE_CL_CFG_MTU_MSG      cli_cfg_mtu;       /* BT_BLE_CL_CFG_MTU_EVT */

    BT_BLE_SE_DEREGISTER_MSG   ser_deregister;    /* BT_BLE_SE_DEREGISTER_EVT */
    BT_BLE_SE_CREATE_MSG       ser_create;        /* BT_BLE_SE_CREATE_EVT */
    BT_BLE_SE_ADDCHAR_MSG      ser_addchar;       /* BT_BLE_SE_ADDCHAR_EVT */
    BT_BLE_SE_START_MSG        ser_start;         /* BT_BLE_SE_START_EVT */
    BT_BLE_SE_STOP_MSG         ser_stop;          /* BT_BLE_SE_STOP_EVT */
    BT_BLE_SE_READ_MSG         ser_read;          /* BT_BLE_SE_READ_EVT */
    BT_BLE_SE_WRITE_MSG        ser_write;         /* BT_BLE_SE_WRITE_EVT */
    BT_BLE_SE_OPEN_MSG         ser_open;          /* BT_BLE_SE_OPEN_EVT */
    BT_BLE_SE_EXEC_WRITE_MSG   ser_exec_write;    /* BT_BLE_SE_EXEC_WRITE_EVT */
    BT_BLE_SE_CLOSE_MSG        ser_close;         /* BT_BLE_SE_CLOSE_EVT */
    BT_BLE_SE_CONGEST_MSG      ser_congest;       /* BT_BLE_CL_CONGEST_EVT */
    BT_BLE_SE_CONF_MSG         ser_conf;         /* BT_BLE_SE_CONF_EVT */
    BT_BLE_SE_MTU_MSG          ser_mtu;
} BT_BLE_MSG;


/*BLE callback events */
typedef enum
{
    /* BLE Client events */
    BT_BLE_CL_DEREGISTER_EVT,    /* BLE client is registered. */
    BT_BLE_CL_OPEN_EVT,          /* BLE open request status  event */
    BT_BLE_CL_READ_EVT,          /* BLE read characteristic/descriptor event */
    BT_BLE_CL_WRITE_EVT,         /* BLE write characteristic/descriptor event */
    BT_BLE_CL_CLOSE_EVT,         /* GATTC  close request status event */
    BT_BLE_CL_SEARCH_CMPL_EVT,   /* GATT discovery complete event */
    BT_BLE_CL_SEARCH_RES_EVT,    /* GATT discovery result event */
    BT_BLE_CL_NOTIF_EVT,         /* GATT attribute notification event */
    BT_BLE_CL_CONGEST_EVT,       /* GATT congestion/uncongestion event */
    BT_BLE_CL_CACHE_SAVE_EVT,
    BT_BLE_CL_CACHE_LOAD_EVT,
    BT_BLE_CL_CFG_MTU_EVT,       /* configure MTU complete event */

    /* BLE Server events */
    BT_BLE_SE_DEREGISTER_EVT,    /* BLE Server is deregistered */
    BT_BLE_SE_CREATE_EVT,        /* Service is created */
    BT_BLE_SE_ADDCHAR_EVT,       /* char data is added */
    BT_BLE_SE_START_EVT,         /* Service is started */
    BT_BLE_SE_STOP_EVT,          /* Service is stopped */
    BT_BLE_SE_WRITE_EVT,         /* Write request from client */
    BT_BLE_SE_CONGEST_EVT,       /* Congestion event */
    BT_BLE_SE_READ_EVT,          /* Read request from client */
    BT_BLE_SE_EXEC_WRITE_EVT,    /* Exec write request from client */
    BT_BLE_SE_OPEN_EVT,          /* Connect request from client */
    BT_BLE_SE_CLOSE_EVT,         /* Disconnect request from client */
    BT_BLE_SE_CONF_EVT,
    BT_BLE_SE_MTU_EVT,
} BT_BLE_EVT;


#ifdef __cplusplus
}
#endif
#endif /* __BT_BLE_H__ */


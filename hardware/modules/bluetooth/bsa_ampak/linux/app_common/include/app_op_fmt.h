/*****************************************************************************
**
**  Name:           app_op_fmt.h
**
**  Description:    This is the interface file for common functions and data
**                  types used by the OPP object formatting functions.
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**
*****************************************************************************/
#ifndef APP_OP_FMT_H
#define APP_OP_FMT_H

#include "app_op_api.h"

/*****************************************************************************
** Constants
*****************************************************************************/

#define APP_OP_PROP_OVHD        3

/*****************************************************************************
** Data types
*****************************************************************************/

typedef struct
{
    const char              *p_name;
    UINT8                   len;
} tAPP_OP_PARAM;

typedef struct
{
    const char              *p_name;
    const tAPP_OP_PARAM     *p_param_tbl;
    UINT8                   len;
} tAPP_OP_PROP_TBL;


typedef struct
{
    const UINT32            *p_prop_filter_mask;
    UINT8                   len;
} tAPP_OP_PROP_FILTER_MASK_TBL;

typedef struct
{
    const tAPP_OP_PROP_TBL  *p_tbl;
    tAPP_OP_FMT             fmt;
    const char              *p_begin_str;
    const char              *p_end_str;
    UINT8                   begin_len;
    UINT8                   end_len;
    UINT8                   min_len;
    const tAPP_OP_PROP_FILTER_MASK_TBL  *p_prop_filter_mask_tbl;
} tAPP_OP_OBJ_TBL;

typedef struct
{
    const char              *media_name;
    UINT8                   len;
} tAPP_OP_PROP_MEDIA;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern tAPP_OP_STATUS app_op_build_obj(const tAPP_OP_OBJ_TBL *p_bld,
                                       UINT8 *p_data, UINT16 *p_len,
                                       tAPP_OP_PROP *p_prop, UINT8 num_prop);

extern tAPP_OP_STATUS app_op_parse_obj(const tAPP_OP_OBJ_TBL *p_prs,
                                       tAPP_OP_PROP *p_prop, UINT8 *p_num_prop,
                                       UINT8 *p_data, UINT32 len);
extern tAPP_OP_STATUS app_op_check_property_by_selector(const tAPP_OP_PROP_TBL *p_tbl, UINT8 *p_name,
                                tAPP_OP_PROP *p_prop, UINT8 num_prop);
extern tAPP_OP_STATUS app_op_get_property_by_name(const tAPP_OP_PROP_TBL *p_tbl, UINT8 *p_name,
                                tAPP_OP_PROP *p_prop, UINT8 num_prop, UINT8 *p_data,
                                UINT16 *p_len);
extern UINT8 *app_op_scanstr(UINT8 *p, UINT8 *p_end, const char *p_str);
extern void app_op_set_prop_filter_mask(UINT64 mask);

#endif /* APP_OP_FMT_H */


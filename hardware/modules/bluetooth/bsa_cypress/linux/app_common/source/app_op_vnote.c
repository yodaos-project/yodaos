/*****************************************************************************
**
**  Name:           app_op_vnote.c
**
**  Description:    This file contains functions for parsing and building
**                  vNote objects.
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**
*****************************************************************************/

#include "app_op_api.h"
#include "app_op_fmt.h"

/*****************************************************************************
** Constants
*****************************************************************************/

#define APP_OP_VNOTE_BEGIN_LEN      26
#define APP_OP_VNOTE_END_LEN        11

#define APP_OP_VNOTE_MIN_LEN        (APP_OP_VNOTE_BEGIN_LEN + APP_OP_VNOTE_END_LEN)

const char app_op_vnote_prs_begin[] = "BEGIN:VNOTE\r\n";

const char app_op_vnote_begin[] = "BEGIN:VNOTE\r\nVERSION:1.1\r\n";

const char app_op_vnote_end[] = "END:VNOTE\r\n";

const tAPP_OP_PROP_TBL app_op_vnote_tbl[] =
{
    {NULL,          NULL,   2},    /* Number of elements in array */
    {"BODY",        NULL,   7},    /* APP_OP_VNOTE_BODY */
    {"X-IRMC-LUID", NULL,   14}    /* APP_OP_VNOTE_LUID */
};

const tAPP_OP_OBJ_TBL app_op_vnote_bld =
{
    app_op_vnote_tbl,
    APP_OP_VNOTE_FMT,
    app_op_vnote_begin,
    app_op_vnote_end,
    APP_OP_VNOTE_BEGIN_LEN,
    APP_OP_VNOTE_END_LEN,
    APP_OP_VNOTE_MIN_LEN,
	NULL
};

const tAPP_OP_OBJ_TBL app_op_vnote_prs =
{
    app_op_vnote_tbl,
    APP_OP_VNOTE_FMT,
    app_op_vnote_prs_begin,
    app_op_vnote_end,
    APP_OP_VNOTE_BEGIN_LEN,
    APP_OP_VNOTE_END_LEN,
    APP_OP_VNOTE_MIN_LEN,
	NULL
};

/*******************************************************************************
**
** Function         app_op_build_note
**
** Description      Build a vNote object.  The input to this function is an
**                  array of vNote properties and a pointer to memory to store
**                  the card.  The output is a formatted vNote.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_build_note(UINT8 *p_note, UINT16 *p_len, tAPP_OP_PROP *p_prop,
                               UINT8 num_prop)
{
    return app_op_build_obj(&app_op_vnote_bld, p_note, p_len, p_prop, num_prop);
}

/*******************************************************************************
**
** Function         app_op_parse_note
**
** Description      Parse a vNote object.  The input to this function is a
**                  pointer to vNote data.  The output is an array of parsed
**                  vNote properties.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete parsing.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_parse_note(tAPP_OP_PROP *p_prop, UINT8 *p_num_prop, UINT8 *p_note,
                               UINT32 len)
{
    return app_op_parse_obj(&app_op_vnote_prs, p_prop, p_num_prop, p_note, len);
}


/*****************************************************************************
**
**  Name:           app_op_vcal.c
**
**  Description:    This file contains functions for parsing and building
**                  vCal objects.
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**
*****************************************************************************/

#include "app_op_api.h"
#include "app_op_fmt.h"

/*****************************************************************************
** Constants
*****************************************************************************/

#define APP_OP_TODO_BEGIN_LEN       43
#define APP_OP_TODO_END_LEN         26

#define APP_OP_TODO_MIN_LEN         (APP_OP_TODO_BEGIN_LEN + APP_OP_TODO_END_LEN)

#define APP_OP_EVENT_BEGIN_LEN      44
#define APP_OP_EVENT_END_LEN        27

#define APP_OP_EVENT_MIN_LEN        (APP_OP_EVENT_BEGIN_LEN + APP_OP_EVENT_END_LEN)

#define APP_OP_VCAL_BEGIN_LEN       17
#define APP_OP_VCAL_END_LEN         15
#define APP_OP_VCAL_MIN_LEN         (APP_OP_VCAL_BEGIN_LEN + APP_OP_VCAL_END_LEN)
#define APP_OP_BEGIN_OFFSET         30

const char app_op_vcal_begin[] = "BEGIN:VCALENDAR\r\n";

const char app_op_todo_begin[] = "BEGIN:VCALENDAR\r\nVERSION:1.0\r\nBEGIN:VTODO\r\n";

const char app_op_todo_end[] = "END:VTODO\r\nEND:VCALENDAR\r\n";

const char app_op_event_begin[] = "BEGIN:VCALENDAR\r\nVERSION:1.0\r\nBEGIN:VEVENT\r\n";

const char app_op_event_end[] = "END:VEVENT\r\nEND:VCALENDAR\r\n";

const tAPP_OP_PROP_TBL app_op_vcal_tbl[] =
{
    {NULL,          NULL, 11},      /* Number of elements in array */
    {"CATEGORIES",  NULL, 13},      /* APP_OP_VCAL_CATEGORIES */
    {"COMPLETED",   NULL, 12},      /* APP_OP_VCAL_COMPLETED */
    {"DESCRIPTION", NULL, 14},      /* APP_OP_VCAL_DESCRIPTION */
    {"DTEND",       NULL,  8},      /* APP_OP_VCAL_DTEND */
    {"DTSTART",     NULL, 10},      /* APP_OP_VCAL_DTSTART */
    {"DUE",         NULL,  6},      /* APP_OP_VCAL_DUE */
    {"LOCATION",    NULL, 11},      /* APP_OP_VCAL_LOCATION */
    {"PRIORITY",    NULL, 11},      /* APP_OP_VCAL_PRIORITY */
    {"STATUS",      NULL,  9},      /* APP_OP_VCAL_STATUS */
    {"SUMMARY",     NULL, 10},      /* APP_OP_VCAL_SUMMARY */
    {"X-IRMC-LUID", NULL, 14}       /* APP_OP_VCAL_LUID */
};

const tAPP_OP_OBJ_TBL app_op_todo_bld =
{
    app_op_vcal_tbl,
    APP_OP_VCAL_FMT,
    app_op_todo_begin,
    app_op_todo_end,
    APP_OP_TODO_BEGIN_LEN,
    APP_OP_TODO_END_LEN,
    APP_OP_TODO_MIN_LEN,
	NULL
};

const tAPP_OP_OBJ_TBL app_op_event_bld =
{
    app_op_vcal_tbl,
    APP_OP_VCAL_FMT,
    app_op_event_begin,
    app_op_event_end,
    APP_OP_EVENT_BEGIN_LEN,
    APP_OP_EVENT_END_LEN,
    APP_OP_EVENT_MIN_LEN,
	NULL
};

const tAPP_OP_OBJ_TBL app_op_vcal_prs =
{
    app_op_vcal_tbl,
    APP_OP_VCAL_FMT,
    app_op_vcal_begin,
    NULL,
    APP_OP_VCAL_BEGIN_LEN,
    APP_OP_VCAL_END_LEN,
    APP_OP_VCAL_MIN_LEN,
	NULL
};

/*******************************************************************************
**
** Function         app_op_build_cal
**
** Description      Build a vCal 1.0 object.  The input to this function is an
**                  array of vCaalproperties and a pointer to memory to store
**                  the card.  The output is a formatted vCal.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_build_cal(UINT8 *p_cal, UINT16 *p_len, tAPP_OP_PROP *p_prop,
                              UINT8 num_prop, tAPP_OP_VCAL vcal_type)
{
    tAPP_OP_OBJ_TBL *p_bld;

    if (vcal_type == APP_OP_VCAL_EVENT)
    {
        p_bld = (tAPP_OP_OBJ_TBL *) &app_op_event_bld;
    }
    else
    {
        p_bld = ( tAPP_OP_OBJ_TBL *) &app_op_todo_bld;
    }

    return app_op_build_obj(p_bld, p_cal, p_len, p_prop, num_prop);
}

/*******************************************************************************
**
** Function         app_op_parse_cal
**
** Description      Parse a vCal object.  The input to this function is a
**                  pointer to vCal data.  The output is an array of parsed
**                  vCal properties.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete parsing.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_parse_cal(tAPP_OP_PROP *p_prop, UINT8 *p_num_prop, UINT8 *p_cal,
                              UINT32 len, tAPP_OP_VCAL *p_vcal_type)
{
    if (app_op_scanstr(p_cal, (p_cal + len), &app_op_todo_begin[APP_OP_BEGIN_OFFSET]) != NULL)
    {
        *p_vcal_type = APP_OP_VCAL_TODO;
    }
    else if (app_op_scanstr(p_cal, (p_cal + len), &app_op_event_begin[APP_OP_BEGIN_OFFSET]) != NULL)
    {
        *p_vcal_type = APP_OP_VCAL_EVENT;
    }
    else
    {
        *p_num_prop = 0;
        return APP_OP_FAIL;
    }

    return app_op_parse_obj(&app_op_vcal_prs, p_prop, p_num_prop, p_cal, len);
}


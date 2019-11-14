/*****************************************************************************
**
**  Name:           app_op_vcard.c
**
**  Description:    This file contains functions for parsing and building
**                  vCard objects.
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**
*****************************************************************************/
#include <string.h>
#include <ctype.h>
#include "app_op_api.h"
#include "app_op_fmt.h"

/*****************************************************************************
** Constants
*****************************************************************************/

#define APP_OP_VCARD_BEGIN_LEN      26
#define APP_OP_VCARD_END_LEN        11
#define APP_OP_NUM_FILTERS          32

#define APP_OP_VCARD_MIN_LEN        (APP_OP_VCARD_BEGIN_LEN + APP_OP_VCARD_END_LEN)

const char app_op_vcard_prs_begin[] = "BEGIN:VCARD\r\n";

const char app_op_vcard_21_begin[] = "BEGIN:VCARD\r\nVERSION:2.1\r\n";

const char app_op_vcard_30_begin[] = "BEGIN:VCARD\r\nVERSION:3.0\r\n";

const char app_op_vcard_end[] = "END:VCARD\r\n";

/* All the types for each attribute is defined in RFC2426*/
const tAPP_OP_PARAM app_op_vcard_adr[] =
{
    {NULL,       6},          /* Number of elements in array */
    {"DOM",      4},          /* APP_OP_ADR_DOM */
    {"INTL",     5},          /* APP_OP_ADR_INTL */
    {"POSTAL",   7},          /* APP_OP_ADR_POSTAL */
    {"PARCEL",   7},          /* APP_OP_ADR_PARCEL */
    {"HOME",     5},          /* APP_OP_ADR_HOME */
    {"WORK",     5}           /* APP_OP_ADR_WORK */
};

const tAPP_OP_PARAM app_op_vcard_email[] =
{
    {NULL,       3},          /* Number of elements in array */
    {"PREF",     5},          /* APP_OP_EMAIL_PREF */
    {"INTERNET", 9},          /* APP_OP_EMAIL_INTERNET */
    {"X400",     5}           /* APP_OP_EMAIL_X400 */
};


const tAPP_OP_PARAM app_op_vcard_label[] =
{
    {NULL,       7},          /* Number of elements in array */
    {"DOM",      4},          /* APP_OP_LABEL_DOM */
    {"INTL",     5},          /* APP_OP_LABEL_INTL */
    {"POSTAL",   7},          /* APP_OP_LABEL_POSTAL */
    {"PARCEL",   7},          /* APP_OP_LABEL_PARCEL */
    {"HOME",     5},          /* APP_OP_LABEL_HOME */
    {"WORK",     5},          /* APP_OP_LABEL_WORK */
    {"PREF",     5}           /* APP_OP_LABEL_PREF */
};

const tAPP_OP_PARAM app_op_vcard_logo[] =
{
    {NULL,       3},          /* Number of elements in array */
    {"JPEG",     5},          /* APP_OP_LABEL_JPEG */
    {"TIFF",     5},          /* APP_OP_LABEL_TIFF */
    {"GIF",      4}           /* APP_OP_LABEL_GIF */
};


const tAPP_OP_PARAM app_op_vcard_tel[] =
{
    {NULL,       8},          /* Number of elements in array */
    {"PREF",     5},          /* APP_OP_TEL_PREF */
    {"WORK",     5},          /* APP_OP_TEL_WORK */
    {"HOME",     5},          /* APP_OP_TEL_HOME */
    {"VOICE",    6},          /* APP_OP_TEL_VOICE */
    {"FAX",      4},          /* APP_OP_TEL_FAX */
    {"MSG",      4},          /* APP_OP_TEL_MSG */
    {"CELL",     5},          /* APP_OP_TEL_CELL */
    {"PAGER",    6}           /* APP_OP_TEL_PAGER */
};

const tAPP_OP_PARAM app_op_vcard_photo[] =
{
    {NULL,       4},          /* Number of elements in array */
    {"VALUE=URI", 10},        /* APP_OP_PHOTO_VALUE_URI */
    {"VALUE=URL", 10},        /* APP_OP_PHOTO_VALUE_URL */
    {"JPEG",     5},          /* APP_OP_PHOTO_TYPE_JPEG */
    {"GIF",      4}           /* APP_OP_PHOTO_TYPE_GIF */
};

const tAPP_OP_PARAM app_op_vcard_sound[] =
{
    {NULL,       4},          /* Number of elements in array */
    {"VALUE=URI", 10},        /* APP_OP_SOUND_VALUE_URI */
    {"VALUE=URL", 10},        /* APP_OP_SOUND_VALUE_URL */
    {"BASIC",     6},         /* APP_OP_SOUND_TYPE_BASIC */
    {"WAVE",      5}          /* APP_OP_SOUND_TYPE_WAVE */
};

const tAPP_OP_PARAM app_op_vcard_call_his_ext[] =
{
    {NULL,          3},         /* Number of elements in array */
    {"MISSED",      7},         /* APP_OP_CALL_MISSED */
    {"RECEIVED",    9},         /* APP_OP_CALL_RECEIVED */
    {"DIALED",      7}          /* APP_OP_CALL_DIALED */
};

const tAPP_OP_PROP_TBL app_op_vcard_tbl[APP_OP_NUM_FILTERS] =
{
    {NULL,          NULL,               31},   /* Number of elements in array, it has to match the following items */
    {"ADR",         app_op_vcard_adr,   6},    /* APP_OP_VCARD_ADR */
    {"AGENT",       NULL,               8},    /* APP_OP_VCARD_AGENT */
    {"BDAY",        NULL,               7},    /* APP_OP_VCARD_BDAY */
    {"CATEGORIES",  NULL,               13},   /* APP_OP_VCARD_CATEGORIES */
    {"CLASS",       NULL,               8},    /* APP_OP_VCARD_CLASS */
    {"EMAIL",       app_op_vcard_email, 8},    /* APP_OP_VCARD_EMAIL */
    {"FN",          NULL,               5},    /* APP_OP_VCARD_FN */
    {"GEO",         NULL,               6},    /* APP_OP_VCARD_GEO */
    {"KEY",         NULL,               6},    /* APP_OP_VCARD_KEY */
    {"LABEL",       app_op_vcard_label, 8},    /* APP_OP_VCARD_LABEL */
    {"LOGO",        app_op_vcard_logo,  7},    /* APP_OP_VCARD_LOGO */
    {"MAILER",      NULL,               9},    /* APP_OP_VCARD_MAILER */
    {"NOTE",        NULL,               7},    /* APP_OP_VCARD_NOTE */
    {"NICKNAME",    NULL,               11},   /* APP_OP_VACRD_NICKNAME */
    {"N",           NULL,               4},    /* APP_OP_VCARD_N */
    {"ORG",         NULL,               6},    /* APP_OP_VCARD_ORG */
    {"PHOTO",       app_op_vcard_photo, 8},    /* APP_OP_VCARD_PHOTO */
    {"PROID",       NULL,               8},    /* APP_OP_VCARD_PROID */
    {"REV",         NULL,               6},    /* APP_OP_VCARD_REV */
    {"ROLE",        NULL,               7},    /* APP_OP_VCARD_ROLE */
    {"SORT-STRING", NULL,               14},   /* APP_OP_VCARD_SORT_STRING */
    {"SOUND",       app_op_vcard_sound, 8},    /* APP_OP_VCARD_SOUND */
    {"TEL",         app_op_vcard_tel,   6},    /* APP_OP_VCARD_TEL */
    {"TITLE",       NULL,               8},    /* APP_OP_VCARD_TITLE */
    {"TZ",          NULL,               5},    /* APP_OP_VCARD_TZ */
    {"X-IRMC-LUID", NULL,               14},   /* APP_OP_VCARD_UID */
    {"URL",         NULL,               6},    /* APP_OP_VCARD_URL */
    {"X-IRMC-CALL-DATETIME", app_op_vcard_call_his_ext,      23},   /* APP_OP_VCARD_CALL */
    {"X-BT-SPEEDDIALKEY", NULL,         20},    /* APP_OP_VCARD_SPD */
    {"X-BT-UCI",    NULL,               11},    /* APP_OP_VCARD_UCI */
    {"X-BT-UID",    NULL,               11}     /* APP_OP_VCARD_BT_UID */
};

const UINT32 app_op_vcard_prop_filter_mask[APP_OP_NUM_FILTERS] =
{
/* table index should be the same as the prop table above */
    APP_OP_FILTER_ALL,
    APP_OP_FILTER_ADR,
    APP_OP_FILTER_AGENT,
    APP_OP_FILTER_BDAY,
    APP_OP_FILTER_CATEGORIES,
    APP_OP_FILTER_CLASS,
    APP_OP_FILTER_EMAIL,
    APP_OP_FILTER_FN,
    APP_OP_FILTER_GEO,
    APP_OP_FILTER_KEY,
    APP_OP_FILTER_LABEL,
    APP_OP_FILTER_LOGO,
    APP_OP_FILTER_MAILER,
    APP_OP_FILTER_NOTE,
    APP_OP_FILTER_NICKNAME,
    APP_OP_FILTER_N,
    APP_OP_FILTER_ORG,
    APP_OP_FILTER_PHOTO,
    APP_OP_FILTER_PROID,
    APP_OP_FILTER_REV,
    APP_OP_FILTER_ROLE,
    APP_OP_FILTER_SORT_STRING,
    APP_OP_FILTER_SOUND,
    APP_OP_FILTER_TEL,
    APP_OP_FILTER_TITLE,
    APP_OP_FILTER_TZ,
    APP_OP_FILTER_UID,
    APP_OP_FILTER_URL,
    APP_OP_FILTER_TIME_STAMP,
    APP_OP_FILTER_X_BT_SPEEDDIALKEY,
    APP_OP_FILTER_X_BT_UCI,
    APP_OP_FILTER_X_BT_UID

};

const tAPP_OP_PROP_FILTER_MASK_TBL app_op_vcard_prop_filter_mask_tbl =
{
    app_op_vcard_prop_filter_mask,
    APP_OP_NUM_FILTERS
};

const tAPP_OP_OBJ_TBL app_op_vcard_21_bld =
{
    app_op_vcard_tbl,
    APP_OP_VCARD21_FMT,
    app_op_vcard_21_begin,
    app_op_vcard_end,
    APP_OP_VCARD_BEGIN_LEN,
    APP_OP_VCARD_END_LEN,
    APP_OP_VCARD_MIN_LEN,
    &app_op_vcard_prop_filter_mask_tbl
};

const tAPP_OP_OBJ_TBL app_op_vcard_30_bld =
{
    app_op_vcard_tbl,
    APP_OP_VCARD30_FMT,
    app_op_vcard_30_begin,
    app_op_vcard_end,
    APP_OP_VCARD_BEGIN_LEN,
    APP_OP_VCARD_END_LEN,
    APP_OP_VCARD_MIN_LEN,
    &app_op_vcard_prop_filter_mask_tbl
};

const tAPP_OP_OBJ_TBL app_op_vcard_21_prs =
{
    app_op_vcard_tbl,
    APP_OP_VCARD21_FMT,
    app_op_vcard_prs_begin,
    app_op_vcard_end,
    APP_OP_VCARD_BEGIN_LEN,
    APP_OP_VCARD_END_LEN,
    APP_OP_VCARD_MIN_LEN,
    NULL
};

const tAPP_OP_OBJ_TBL app_op_vcard_30_prs =
{
    app_op_vcard_tbl,
    APP_OP_VCARD30_FMT,
    app_op_vcard_prs_begin,
    app_op_vcard_end,
    APP_OP_VCARD_BEGIN_LEN,
    APP_OP_VCARD_END_LEN,
    APP_OP_VCARD_MIN_LEN,
    NULL
};

static UINT64 app_op_selector = 0;
static UINT8  app_op_selector_op = 0;

/*******************************************************************************
**
** Function         app_op_get_card_fmt
**
** Description      Finds the vCard format contained in buffer pointed by p_card
**
**
** Returns          Vcard Format APP_OP_VCARD21_FMT/APP_OP_VCARD30_FMT else
**                  APP_OP_OTHER_FMT
**
*******************************************************************************/
static tAPP_OP_FMT app_op_get_card_fmt(UINT8 *p_data, UINT32 len)
{
    UINT8           *p_end = p_data + len;

    if (app_op_scanstr(p_data, p_end, app_op_vcard_21_begin) != NULL)
    {
        return APP_OP_VCARD21_FMT;
    }
    else if (app_op_scanstr(p_data, p_end, app_op_vcard_30_begin) != NULL)
    {
        return APP_OP_VCARD30_FMT;
    }
    else
    {
        return APP_OP_OTHER_FMT;
    }
}

/*******************************************************************************
**
** Function         app_op_build_card
**
** Description      Build a vCard object.  The input to this function is
**                  requested format(2.1/3.0), an array of vCard properties
**                  and a pointer to memory to store the card.
**                  The output is a formatted vCard.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_build_card(UINT8 *p_card, UINT16 *p_len, tAPP_OP_FMT fmt,
                               tAPP_OP_PROP *p_prop, UINT8 num_prop)
{
    if(fmt == APP_OP_VCARD21_FMT)
    {
        return app_op_build_obj(&app_op_vcard_21_bld, p_card, p_len, p_prop, num_prop);
    }
    else if(fmt == APP_OP_VCARD30_FMT)
    {
        return app_op_build_obj(&app_op_vcard_30_bld, p_card, p_len, p_prop, num_prop);
    }
    else
    {
        *p_len = 0;
        return APP_OP_FAIL;
    }
}
/*******************************************************************************
**
** Function         app_op_set_card_selector_operator
**
** Description      Set vCardSelector and SelectorOperator
**
**
** Returns
**
*******************************************************************************/
void app_op_set_card_selector_operator(UINT64 selector, UINT8 selector_op)
{
    app_op_selector = selector;
    app_op_selector_op = selector_op;

}

/*******************************************************************************
**
** Function         app_op_check_card
**
** Description      Go through the properties of the vcard and check whether it
**                  contains the properties meets the requirement of vCardSelector and
**                  vCardSelectorOperator
**
**
** Returns          APP_OP_OK   if it does meets the requirements
**                  APP_OP_FAIL otherwise
**
*******************************************************************************/
tAPP_OP_STATUS app_op_check_card(tAPP_OP_PROP *p_prop, UINT8 num_prop)
{
    UINT8   i;
    tAPP_OP_STATUS status;

    if (app_op_selector != 0 )
    {
        /* And operation */
        if (app_op_selector_op == 1)
        {
            for ( i = 0; i < app_op_vcard_prop_filter_mask_tbl.len; i++)
            {
                if (app_op_vcard_prop_filter_mask_tbl.p_prop_filter_mask[i] & app_op_selector)
                {
                    status = app_op_check_property_by_selector(app_op_vcard_tbl, (UINT8 *) app_op_vcard_tbl[i].p_name, (tAPP_OP_PROP*)p_prop, num_prop);
                     /* Return failure as long as one property cannot be found */
                    if (status == APP_OP_FAIL)
                    {
                        return APP_OP_FAIL;
                    }


                }
            }
            return APP_OP_OK;
        }
        else
        {
            for ( i = 0; i < app_op_vcard_prop_filter_mask_tbl.len; i++)
            {
                if (app_op_vcard_prop_filter_mask_tbl.p_prop_filter_mask[i] & app_op_selector)
                {
                    status = app_op_check_property_by_selector(app_op_vcard_tbl,(UINT8 *) app_op_vcard_tbl[i].p_name, (tAPP_OP_PROP*)p_prop, num_prop);

                    /* Return ok as long as one property cannot be found */
                    if (status == APP_OP_OK)
                    {
                        return APP_OP_OK;
                    }

                }

            }

        }
        return APP_OP_FAIL;

    }
    else
    {
        return APP_OP_OK;
    }


}

/*******************************************************************************
**
** Function         app_op_app_nextline
**
** Description      Scan to beginning of next property text line.
**
**
** Returns          Pointer to beginning of property or NULL if end of
**                  data reached.
**
*******************************************************************************/
static UINT8 *app_op_app_nextline(UINT8 *p, UINT8 *p_end)
{
    if ((p_end - p) > 3)
    {
        p_end -= 3;
        while (p < p_end)
        {
            if (*(++p) == '\r')
            {
                if (*(p - 1) == '=') continue;
                if (*(p + 1) == '\n')
                {
                    if ((*(p + 2) != ' ') && (*(p + 2) != '\t'))
                    {
                        return (p + 2);
                    }
                }
            }
        }
    }
    return NULL;
}


/*******************************************************************************
**
** Function         app_op_app_scanstr
**
** Description      Scan for a matching string.
**
**
** Returns          Pointer to end of match or NULL if end of data reached.
**
*******************************************************************************/
static UINT8 *app_op_app_scanstr(UINT8 *p, UINT8 *p_end, const char *p_str)
{
    int len = strlen(p_str);

    for (;;)
    {
        /* check for match */
        if (strncmp((char *) p, p_str, len) == 0)
        {
            p += len;
            break;
        }
        /* no match; skip to next line, checking for end */
        else if ((p = app_op_app_nextline(p, p_end)) == NULL)
        {
            break;
        }
    }
    return p;
}

/*******************************************************************************
**
** Function         app_op_get_pb_size
**
** Description      Get the phonebook size.
**
**
** Returns          Pointer to end of match or NULL if end of data reached.
**
*******************************************************************************/
void app_op_get_pb_size(UINT8 *p_start, UINT8 *p_end, UINT16 *pb_size)
{
    UINT8 *p_next_start = p_start;
    UINT8 *p_next_end = NULL;
    UINT8 *p_search = NULL;
    UINT8   i;

    *pb_size = 0;

    do
    {
        p_next_start = app_op_app_scanstr(p_next_start, p_end, app_op_vcard_prs_begin);
        if (p_next_start)
        {
            p_next_end   = app_op_app_scanstr(p_next_start, p_end, app_op_vcard_end);
        }
        else
        {
            break;
        }

        if (app_op_selector == 0)
        {
            (*pb_size) ++;
        }
        else if (app_op_selector_op == 0)
        {
            for ( i = 0; i < app_op_vcard_prop_filter_mask_tbl.len; i++)
            {
                if (app_op_vcard_prop_filter_mask_tbl.p_prop_filter_mask[i] & app_op_selector)
                {
                    p_search = app_op_app_scanstr(p_next_start, p_end, app_op_vcard_tbl[i].p_name);
                     /* break as long as one property is found */
                    if (p_search < p_next_end && p_search != NULL)
                    {
                        (*pb_size) ++;
                        break;
                    }
                }
            }

        }
        else
        {
            for ( i = 0; i < app_op_vcard_prop_filter_mask_tbl.len; i++)
            {
                if (app_op_vcard_prop_filter_mask_tbl.p_prop_filter_mask[i] & app_op_selector)
                {
                    p_search = app_op_app_scanstr(p_next_start, p_end, app_op_vcard_tbl[i].p_name);
                    /* break as long as one property is not found */
                    if (p_search > p_next_end || p_search == NULL)
                    {
                        break;
                    }
                }

            }

            if ( i == app_op_vcard_prop_filter_mask_tbl.len )
            {
                (*pb_size) ++;
            }

        }

    } while (p_next_start);

}

/*******************************************************************************
**
** Function         app_op_set_card_prop_filter_mask
**
** Description      Set Property Filter Mask
**
**
** Returns
**
*******************************************************************************/
void app_op_set_card_prop_filter_mask(UINT64 mask)
{
    app_op_set_prop_filter_mask(mask);
    return;
}

/*******************************************************************************
**
** Function         app_op_parse_card
**
** Description      Parse a vCard 2.1 object.  The input to this function is
**                  a pointer to vCard data.  The output is an array of parsed
**                  vCard properties.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete parsing.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_parse_card(tAPP_OP_PROP *p_prop, UINT8 *p_num_prop,
                               UINT8 *p_card, UINT32 len)
{
    tAPP_OP_FMT fmt = app_op_get_card_fmt(p_card, len);

    if (fmt == APP_OP_VCARD21_FMT)
    {
        return app_op_parse_obj(&app_op_vcard_21_prs, p_prop, p_num_prop, p_card, len);
    }
    else if(fmt == APP_OP_VCARD30_FMT)
    {
        return app_op_parse_obj(&app_op_vcard_30_prs, p_prop, p_num_prop, p_card, len);
    }
    else
    {
        *p_num_prop = 0;
        return APP_OP_FAIL;
    }
}

/*******************************************************************************
**
** Function         app_op_get_card_property
**
** Description      Get Card property value by name.  The input to this function is
**                  property name.  The output is property value and len
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_get_card_property(UINT8 *p_value, UINT16 *p_len, tAPP_OP_PROP *p_prop,
                                     UINT8 num_prop, UINT8 *p_name)
{
    return app_op_get_property_by_name(app_op_vcard_tbl, p_name,
                                       p_prop, num_prop, p_value, p_len);
}



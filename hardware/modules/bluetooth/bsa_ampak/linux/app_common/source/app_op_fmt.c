/*****************************************************************************
**
**  Name:           app_op_fmt.c
**
**  Description:    This file contains common functions and data structures
**                  used by the OPP object formatting functions.
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

#define APP_OP_ENCODING_LEN         9
#define APP_OP_CHARSET_LEN          8
#define APP_OP_PARAM_TYPE_HDR_LEN   5
#define APP_OP_NUM_NONINLINE_MEDIA  2

const char app_op_encoding[] = "ENCODING=";

const tAPP_OP_PARAM app_op_encodings[] =
{/* the len is (APP_OP_ENCODING_LEN + 1 + strlen(p_name)) */
    {NULL,                   0},
    {"QUOTED-PRINTABLE",    26},    /* APP_OP_ENC_QUOTED_PRINTABLE */
    {"8BIT",                14},    /* APP_OP_ENC_8BIT */
    {"BASE64",              16},    /* APP_OP_ENC_BASE64 */
    {"b",                   11}     /* APP_OP_ENC_BINARY */

};

#define APP_OP_NUM_ENCODINGS    4
#define APP_OP_QP_IDX           1    /* array index for quoted printable in app_op_encodings */

const char app_op_charset[] = "CHARSET=";

const char app_op_param_type_hdr[] = "TYPE=";

const char BASE64[] = "BASE64";

#define APP_OP_PROP_NAME_PHOTO  17

const tAPP_OP_PARAM app_op_charsets[] =
{/* the len is (APP_OP_CHARSET_LEN + 1 + strlen(p_name)) */
    {NULL,               0},
    {"BIG5",            13},   /* APP_OP_CHAR_BIG5 */
    {"EUC-JP",          15},   /* APP_OP_CHAR_EUC_JP */
    {"EUC-KR",          15},   /* APP_OP_CHAR_EUC_KR */
    {"GB2312",          15},   /* APP_OP_CHAR_GB2312 */
    {"ISO-2022-JP",     20},   /* APP_OP_CHAR_ISO_2022_JP */
    {"ISO-8859-1",      19},   /* APP_OP_CHAR_ISO_8859_1 */
    {"ISO-8859-2",      19},   /* APP_OP_CHAR_ISO_8859_2 */
    {"ISO-8859-3",      19},   /* APP_OP_CHAR_ISO_8859_3 */
    {"ISO-8859-4",      19},   /* APP_OP_CHAR_ISO_8859_4 */
    {"ISO-8859-5",      19},   /* APP_OP_CHAR_ISO_8859_5 */
    {"ISO-8859-6",      19},   /* APP_OP_CHAR_ISO_8859_6 */
    {"ISO-8859-7",      19},   /* APP_OP_CHAR_ISO_8859_7 */
    {"ISO-8859-8",      19},   /* APP_OP_CHAR_ISO_8859_8 */
    {"KOI8-R",          15},   /* APP_OP_CHAR_KOI8_R */
    {"SHIFT_JIS",       18},   /* APP_OP_CHAR_SHIFT_JIS */
    {"UTF-8",           14}    /* APP_OP_CHAR_UTF_8 */
};

#define APP_OP_NUM_CHARSETS     16

/* Structure of the 32-bit parameters mask:
** (same comment is in app_op_api.h)
**                  + property-specific
** +reserved        |        + character set
** |                |        |     + encoding
** |                |        |     |
** 0000000000000000 00000000 00000 000
*/
#define APP_OP_GET_PARAM(param, encod, charset, specific) \
    encod = (UINT8) (param) & 0x0000000F; \
    charset = (UINT8) ((param) >> 3) & 0x0000001F; \
    specific = (UINT8) ((param) >> 8) & 0x000000FF;

#define APP_OP_GET_PARAM_ENCOD(param, encod) \
    encod = (UINT8) (param) & 0x0000000F;

#define APP_OP_GET_PARAM_ENCOD_CHARSET(param, encod, charset) \
    encod = (UINT8) (param) & 0x0000000F; \
    charset = (UINT8) ((param) >> 3) & 0x0000001F;

#define APP_OP_GET_PARAM_ENCOD_SPECIF(param, encod, specific) \
    encod = (UINT8) (param) & 0x0000000F; \
    specific = (UINT8) ((param) >> 8) & 0x000000FF;

/* mask for properties default 0, filter all */
static UINT64 app_op_prop_filter_mask = 0;

const tAPP_OP_PROP_MEDIA app_op_media[] =
{
    {NULL,                   2},
    {"PHOTO",                5},
    {"SOUND",                5}
};

/* Place holder constant for safe string functions since there's no way to know how
** memory is remaining for input parameter to build property.
**  Note:  The BCM_STRCPY_S functions should be changed to know how much memory
**      is remaining.  When completed this constant can be removed.  Also, if safe
**      string functions are not used then this parameter is ignored anyway!
*/
#ifndef APP_OP_REM_MEMORY
#define APP_OP_REM_MEMORY   8228
#endif

/*******************************************************************************
**
** Function         app_op_strnicmp
**
** Description      Case insensitive strncmp.
**
**
** Returns          void
**
*******************************************************************************/
INT16 app_op_strnicmp(const char *pStr1, const char *pStr2, size_t Count)
{
    char c1, c2;
    INT16 v;

    if (Count == 0)
        return 0;

    do {
        c1 = *pStr1++;
        c2 = *pStr2++;
        /* the casts are necessary when pStr1 is shorter & char is signed */
        v = (UINT16) tolower(c1) - (UINT16) tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (--Count > 0));


    return v;
}

/*******************************************************************************
**
** Function         app_op_set_prop_mask
**
** Description      Set property mask
**
**
** Returns          void
**
*******************************************************************************/
void app_op_set_prop_filter_mask(UINT64 mask)
{
    app_op_prop_filter_mask = mask;
    return;
}

/*******************************************************************************
**
** Function         app_op_prop_len
**
** Description      Calculate the length of a property string through lookup
**                  tables.
**
**
** Returns          Length of string in bytes.
**
*******************************************************************************/
UINT16 app_op_prop_len(const tAPP_OP_PROP_TBL *p_tbl, tAPP_OP_PROP *p_prop)
{
    UINT8   i_e, i_c, i_s;
    UINT8   len_s = 0;
    int     i;

    /* parse parameters mask */
    APP_OP_GET_PARAM(p_prop->parameters, i_e, i_c, i_s);

    /* calculate length of property-specific parameters, if any */
    if (p_tbl[p_prop->name].p_param_tbl != NULL)
    {
        for (i = 1; (i_s != 0) && (i <= p_tbl[p_prop->name].p_param_tbl[0].len); i++)
        {
            if (i_s & 1)
            {
                len_s += p_tbl[p_prop->name].p_param_tbl[i].len;
            }
            i_s >>= 1;
        }
    }

    return (p_tbl[p_prop->name].len + len_s + app_op_charsets[i_c].len +
            app_op_encodings[i_e].len + p_prop->len);
}

/*******************************************************************************
**
** Function         app_op_param_conflict
**
** Description      Check if the parameters of the property for the format
**                  conflict/not allowed.
**
**
** Returns          TRUE if not allowed, else FALSE
**
*******************************************************************************/
BOOLEAN app_op_param_conflict(tAPP_OP_FMT fmt, tAPP_OP_PROP *p_prop)
{
    UINT8   i_e, i_c;
    BOOLEAN conflict = FALSE;

    if (p_prop->parameters != 0)
    {
        APP_OP_GET_PARAM_ENCOD_CHARSET(p_prop->parameters, i_e, i_c);

        if (fmt == APP_OP_VCARD30_FMT)
        {
            /* VCard 3.0 does not support CHARSET. If it is present, we
               should ignore the whole property in the vCard build process */
            if (app_op_charsets[i_c].p_name != NULL)
                conflict = TRUE;

            /* VCard 3.0 only supports 'b' encoding. If any other encoding
               ex. quoted-printable, is present, then whole property is ignored */
            if ((app_op_encodings[i_e].p_name != NULL) && strcmp(app_op_encodings[i_e].p_name, "b"))
                conflict = TRUE;
        }
    }

    return conflict;
}
/*******************************************************************************
**
** Function         app_op_add_param
**
** Description      Add parameter strings to a property string.
**
**
** Returns          Pointer to the next byte after the end of the string.
**
*******************************************************************************/
UINT8 *app_op_add_param(tAPP_OP_FMT fmt, const tAPP_OP_PROP_TBL *p_tbl, UINT8 *p, tAPP_OP_PROP *p_prop)
{
    UINT8   i_e, i_c, i_s;
    int     i;
    UINT8   app_op_param_type_delimit = 0;

    if (p_prop->parameters != 0)
    {
        APP_OP_GET_PARAM(p_prop->parameters, i_e, i_c, i_s);

        /* add encoding parameter */
        if (app_op_encodings[i_e].p_name != NULL)
        {
            *p++ = ';';
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, app_op_encoding);
            p += APP_OP_ENCODING_LEN;
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, app_op_encodings[i_e].p_name);
            p += app_op_encodings[i_e].len - APP_OP_ENCODING_LEN - 1;
        }

        /* add character set parameter */
        if (app_op_charsets[i_c].p_name != NULL)
        {
            *p++ = ';';
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, app_op_charset);
            p += APP_OP_CHARSET_LEN;
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, app_op_charsets[i_c].p_name);
            p += app_op_charsets[i_c].len - APP_OP_CHARSET_LEN - 1;
        }

        /* add any property-specific parameters */
        if (p_tbl[p_prop->name].p_param_tbl != NULL)
        {
            for (i = 1; (i_s != 0) && (i <= p_tbl[p_prop->name].p_param_tbl[0].len); i++)
            {
                if (i_s & 1)
                {
                    if (app_op_param_type_delimit)
                    {
                        *p++ = ',';
                    }
                    else
                    {
                        *p++ = ';';

                        if (fmt == APP_OP_VCARD30_FMT)
                        {
                            BCM_STRNCPY_S((char *) p, APP_OP_PARAM_TYPE_HDR_LEN+1, app_op_param_type_hdr, APP_OP_PARAM_TYPE_HDR_LEN);
                            p += APP_OP_PARAM_TYPE_HDR_LEN;
                            app_op_param_type_delimit++;
                        }
                    }
                    BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, p_tbl[p_prop->name].p_param_tbl[i].p_name);
                    p += p_tbl[p_prop->name].p_param_tbl[i].len - 1;
                }
                i_s >>= 1;
            }
        }
    }
    else if (p_prop->p_param)
    {
        *p++ = ';';
        memcpy((char *) p, p_prop->p_param, p_prop->param_len);
        p += p_prop->param_len;
    }

    return p;
}

/*******************************************************************************
**
** Function         app_op_add_media_param
**
** Description      Add parameter strings to a media property string.
**
**
** Returns          Pointer to the next byte after the end of the string.
**
*******************************************************************************/
UINT8 *app_op_add_media_param(const tAPP_OP_PROP_TBL *p_tbl, UINT8 *p, tAPP_OP_PROP *p_prop)
{
    UINT8   i_e, i_s;
    int     i;

    if (p_prop->parameters != 0)
    {
        APP_OP_GET_PARAM_ENCOD_SPECIF(p_prop->parameters, i_e, i_s);

        /* add encoding parameter */
        if (app_op_encodings[i_e].p_name != NULL)
        {
            *p++ = ';';
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, app_op_encoding);
            p += APP_OP_ENCODING_LEN;
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, app_op_encodings[i_e].p_name);
            p += app_op_encodings[i_e].len - APP_OP_ENCODING_LEN - 1;
        }

        /* add any property-specific parameters */
        if (p_tbl[p_prop->name].p_param_tbl != NULL)
        {
            for (i = 1; (i_s != 0) && (i <= p_tbl[p_prop->name].p_param_tbl[0].len); i++)
            {
                if (i_s & 1)
                {
                    *p++ = ';';

                    /* Add "TYPE=" to non-referenced (inline) media */
                    if (i > APP_OP_NUM_NONINLINE_MEDIA)
                    {
                        BCM_STRNCPY_S((char *) p, APP_OP_PARAM_TYPE_HDR_LEN+1, app_op_param_type_hdr, APP_OP_PARAM_TYPE_HDR_LEN);
                        p += APP_OP_PARAM_TYPE_HDR_LEN;
                    }

                    BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, p_tbl[p_prop->name].p_param_tbl[i].p_name);
                    p += p_tbl[p_prop->name].p_param_tbl[i].len - 1;
                    break;
                }
                i_s >>= 1;
            }
        }
    }
    else if (p_prop->p_param)
    {
        *p++ = ';';
        memcpy((char *) p, p_prop->p_param, p_prop->param_len);
        p += p_prop->param_len;
    }

    return p;
}

/*******************************************************************************
**
** Function         app_op_check_property_by_selector
**
** Description      Check each property with selector.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if there is no property user data.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_check_property_by_selector(const tAPP_OP_PROP_TBL *p_tbl, UINT8 *p_name,
                                tAPP_OP_PROP *p_prop, UINT8 num_prop)
{
    int             i, j;
    tAPP_OP_STATUS  result = APP_OP_FAIL;

    /* for each property */
    for(i = 0; num_prop != 0; num_prop--, i++)
    {
            /* verify property is valid */
        if ((p_prop[i].name == 0) || (p_prop[i].name > p_tbl[0].len))
        {
             result = APP_OP_FAIL;
             break;
        }

        j = p_prop[i].name;

        if (app_op_strnicmp(p_tbl[j].p_name, (char *) p_name,
                (p_tbl[j].len - APP_OP_PROP_OVHD)) == 0)
        {
            result = APP_OP_OK;
            break;
        }
    }
    return result;

}

/*******************************************************************************
**
** Function         app_op_get_property_by_name
**
** Description      Get the property user data by name.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if there is no property user data.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_get_property_by_name(const tAPP_OP_PROP_TBL *p_tbl, UINT8 *p_name,
                                tAPP_OP_PROP *p_prop, UINT8 num_prop, UINT8 *p_data,
                                UINT16 *p_len)
{
    int             i, j;
    tAPP_OP_STATUS  result = APP_OP_FAIL;

    /* for each property */
    for(i = 0; num_prop != 0; num_prop--, i++)
    {
            /* verify property is valid */
        if ((p_prop[i].name == 0) || (p_prop[i].name > p_tbl[0].len))
        {
             result = APP_OP_FAIL;
             break;
        }

        j = p_prop[i].name;

        if (app_op_strnicmp(p_tbl[j].p_name, (char *) p_name,
                (p_tbl[j].len - APP_OP_PROP_OVHD)) == 0)
        {
            memcpy(p_data, p_prop[i].p_data, p_prop[i].len);
            p_data[p_prop[i].len] = 0;
            *p_len = p_prop[i].len;
            result = APP_OP_OK;
            break;
        }
    }
    return result;
}

/*******************************************************************************
**
** Function         app_op_build_obj
**
** Description      Build an object from property data supplied by the user.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid property data.
**                  APP_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_build_obj(const tAPP_OP_OBJ_TBL *p_bld, UINT8 *p_data,
                                UINT16 *p_len, tAPP_OP_PROP *p_prop, UINT8 num_prop)
{
    int             i,j;
    tAPP_OP_STATUS  result = APP_OP_OK;
    UINT8           *p = p_data;

    /* sanity check length */
    if (*p_len < p_bld->min_len)
    {
        result = APP_OP_MEM;
    }
    else
    {
        /* adjust p_len to amount of free space minus start and end */
        *p_len -= p_bld->min_len;

        /* add begin, version */
        BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, p_bld->p_begin_str);
        p += p_bld->begin_len;

        /* for each property */
        for(i = 0; num_prop != 0; num_prop--, i++)
        {
            /* verify property is valid */
            if ((p_prop[i].name == 0) || (p_prop[i].name > p_bld->p_tbl[0].len))
            {
                result = APP_OP_FAIL;
                break;
            }

            /* verify property will fit */
            if (app_op_prop_len(p_bld->p_tbl, &p_prop[i]) > (p_data + p_bld->begin_len + *p_len - p))
            {
                result = APP_OP_MEM;
                break;
            }

            /* check for filter */
            if (app_op_prop_filter_mask !=0 && p_bld->p_prop_filter_mask_tbl != NULL &&
                p_prop[i].name <= p_bld->p_prop_filter_mask_tbl->len)
            {

                /* APPL_TRACE_DEBUG3("filtermask 0x%016x  %d filter 0x%016x", app_op_prop_filter_mask, p_prop[i].name, p_bld->p_prop_filter_mask_tbl->p_prop_filter_mask[p_prop[i].name]); */
                if (!(app_op_prop_filter_mask & p_bld->p_prop_filter_mask_tbl->p_prop_filter_mask[p_prop[i].name]))
                    continue;

            }

            /* Check if the combination of parameters are allowed for
               the format we are building */
            if (app_op_param_conflict(p_bld->fmt, &p_prop[i]))
                continue;

            /* check whether property contains data or not, if there is no data skip the property */
            if (p_prop[i].len == 0)
            {
                continue;
            }

            /* add property string */
            BCM_STRCPY_S((char *) p, APP_OP_REM_MEMORY, p_bld->p_tbl[p_prop[i].name].p_name);
            p += p_bld->p_tbl[p_prop[i].name].len - APP_OP_PROP_OVHD;

            for (j= app_op_media[0].len; j > 0; j--)
            {
                if (!app_op_strnicmp(app_op_media[j].media_name, p_bld->p_tbl[p_prop[i].name].p_name, app_op_media[j].len))
                {
                    p = app_op_add_media_param(p_bld->p_tbl, p, &p_prop[i]);
                    break;
                }
            }

            if (!j)
            {
                /* add property parameters */
                p = app_op_add_param(p_bld->fmt, p_bld->p_tbl, p, &p_prop[i]);
            }

            /* add user data */
            *p++ = ':';
            memcpy(p, p_prop[i].p_data, p_prop[i].len);
            p += p_prop[i].len;
            *p++ = '\r';
            *p++ = '\n';
        }

        /* add in end */
        memcpy(p, p_bld->p_end_str, p_bld->end_len);
        p += p_bld->end_len;
    }

    *p_len = (UINT16) (p - p_data);
    return result;
}

/*******************************************************************************
**
** Function         app_op_nextline
**
** Description      Scan to beginning of next property text line.
**
**
** Returns          Pointer to beginning of property or NULL if end of
**                  data reached.
**
*******************************************************************************/
static UINT8 *app_op_nextline(UINT8 *p, UINT8 *p_end, BOOLEAN qp)
{
    /* check if the field is empty or not, return if empty */
    if (*p == '\r' && *(p+1) == '\n')
        return (p + 2);

    if ((p_end - p) > 3)
    {
        p_end -= 3;
        while (p < p_end)
        {
            if (*(++p) == '\r')
            {
                if (*(p + 1) == '\n')
                {
                    if (qp)
                    {
                        if (*(p - 1) == '=')
                        {
                            /* this is a soft break for quoted-printable*/
                            continue;
                        }
                    }

                    if ((*(p + 2) != ' ') && (*(p + 2) != '\t'))
                    {
                        return(p + 2);
                    }
                }
            }
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         app_op_scantok
**
** Description      Scan a line for one or more tokens.
**
**
** Returns          Pointer to token or NULL if end of data reached.
**
*******************************************************************************/
static UINT8 *app_op_scantok(UINT8 *p, UINT8 *p_end, const char *p_tok)
{
    int     i;
    UINT8   num_tok = strlen(p_tok);

    for (; p < p_end; p++)
    {
        for (i = 0; i < num_tok; i++)
        {
            if (*p == p_tok[i])
            {
                return p;
            }
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function         app_op_skip_illegal_line
**
** Description      Scan a line to detect illegal input.
**                  If until the end of one line (indicated by \r\n), there is no
**                  ':' ';' '.' then this line is illegal.
**                  If illegal line is detected, parser shall skip this line and move to
**                  the next line and scan again.
**
**
** Returns          Pointer to token or NULL if end of data reached.
**
*******************************************************************************/
static UINT8 *app_op_skip_illegal_line(UINT8 *p, UINT8 *p_end)
{
    int     counter = 0;
    int     skipped = 0;
    UINT8   *p_start = p;


    for (; p < p_end; p++)
    {
        /* If we find one of the following character in a line, we should move
         * the return pointer to the beginning of that line since the line is legal*/
        if ((*p == ';') || (*p == ':') || (*p =='.') )
        {
            return (p_start+skipped);
        }

        /* Check for line feed and carriage return RFC2425*/
        if ((*p =='\r') && (*(p+1) =='\n'))
        {
            if ((*(p + 2) != ' ') && (*(p + 2) != '\t'))
            {
                /* increment the counter and pointer position by an one for the '\n' */
                /* also keep track of the positions skipped */
                p++;
                skipped = counter + 2;
                APPL_TRACE_WARNING0("Skip one line");
                counter++;

            }

        }
        counter++;
    }

    return NULL;
}

/*******************************************************************************
**
** Function         app_op_scanstr
**
** Description      Scan for a matching string.
**
**
** Returns          Pointer to end of match or NULL if end of data reached.
**
*******************************************************************************/
UINT8 *app_op_scanstr(UINT8 *p, UINT8 *p_end, const char *p_str)
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
        else if ((p = app_op_nextline(p, p_end, FALSE)) == NULL)
        {
            break;
        }
    }
    return p;
}

/*******************************************************************************
**
** Function         app_op_find_photo_encoding_length
**
** Description      Find the length of the base64 encoded photo content
**
**
** Returns          length of the encoded photo conetent
**
*******************************************************************************/
static UINT16 app_op_find_photo_encoding_length(UINT8 *p, UINT8 *p_end)
{
    UINT16  counter = 1;

    while( p < p_end)
    {
        if ((*p =='\r') && (*(p+1) =='\n'))
        {
            if ((*(p + 2) != ' ') && (*(p + 2) != '\t') && (*(p + 2) != '\r'))
            {
                counter++;
                return counter;
            }

        }
        counter++;
        p++;
    }

    return counter;
}


/*******************************************************************************
**
** Function         app_op_parse_obj
**
** Description      Parse an object.
**
**
** Returns          APP_OP_OK if operation successful.
**                  APP_OP_FAIL if invalid data.
**                  APP_OP_MEM if not enough memory to complete build.
**
*******************************************************************************/
tAPP_OP_STATUS app_op_parse_obj(const tAPP_OP_OBJ_TBL *p_prs, tAPP_OP_PROP *p_prop,
                                UINT8 *p_num_prop, UINT8 *p_data, UINT32 len)
{
    UINT32          j;
    UINT8           *p_s, *p_e;
    tAPP_OP_STATUS  result = APP_OP_OK;
    UINT8           max_prop = *p_num_prop;
    UINT8           *p_end = p_data + len;
    UINT8           prop_name = 0;
    BOOLEAN         qp=FALSE;
    UINT8           i_e;

    *p_num_prop = 0;

    /* sanity check length */
    if (len < p_prs->min_len)
    {
        return APP_OP_FAIL;
    }

    /* find beginning */
    if ((p_s = app_op_scanstr(p_data, p_end, p_prs->p_begin_str)) == NULL)
    {
        return APP_OP_FAIL;
    }

    while (*p_num_prop < max_prop)
    {
        /* skip the illegal lines */
        if ((p_s = app_op_skip_illegal_line(p_s, p_end)) == NULL )
        {
            break;
        }

        /* scan for next delimiter */
        if ((p_e = app_op_scantok(p_s, p_end, ".;:")) == NULL)
        {
            break;
        }

        /* deal with grouping delimiter */
        if (*p_e == '.')
        {
            p_s = p_e + 1;
            if ((p_e = app_op_scantok(p_s, p_end, ";:")) == NULL)
            {
                break;
            }
        }

        /* we found a property; see if it matches anything in our list */
        for (j = 1; j <= p_prs->p_tbl[0].len; j++)
        {
            if (app_op_strnicmp(p_prs->p_tbl[j].p_name, (char *) p_s,
                (p_prs->p_tbl[j].len - APP_OP_PROP_OVHD)) == 0)
            {
                p_prop[*p_num_prop].name = prop_name = j;
                p_prop[*p_num_prop].parameters = 0;
                break;
            }
        }

        /* if not in our list we can't parse it; continue */
        if (j > p_prs->p_tbl[0].len)
        {
            if ((p_s = app_op_nextline(p_e, p_end, FALSE)) == NULL)
            {
                break;
            }
            continue;
        }


        /* now parse out all the parameters */
        if (*p_e == ';')
        {
            while ((*p_e == ';') || (*p_e == ','))
            {
                p_s = p_e + 1;

                if ((p_e = app_op_scantok(p_s, p_end, ",;:")) == NULL)
                {
                    break;
                }

                /* we found a parameter; see if it matches anything */

                /* check for encoding */
                qp = FALSE;
                if (app_op_strnicmp(app_op_encoding, (char *) p_s, APP_OP_ENCODING_LEN) == 0)
                {
                    p_s += APP_OP_ENCODING_LEN;
                    for (j = 1; j <= APP_OP_NUM_ENCODINGS; j++)
                    {
                        if (app_op_strnicmp(app_op_encodings[j].p_name, (char *) p_s,
                            (app_op_encodings[j].len - APP_OP_ENCODING_LEN - 1)) == 0)
                        {
                            if (j == APP_OP_QP_IDX)
                            {
                                /* encoding = quoted-printable*/
                                qp= TRUE;
                            }
                            p_prop[*p_num_prop].parameters |= j;
                            break;
                        }
                    }
                }
                /* check for charset */
                else if (app_op_strnicmp(app_op_charset, (char *) p_s, APP_OP_CHARSET_LEN) == 0)
                {
                    p_s += APP_OP_CHARSET_LEN;
                    for (j = 1; j <= APP_OP_NUM_CHARSETS; j++)
                    {
                        if (app_op_strnicmp(app_op_charsets[j].p_name, (char *) p_s,
                            (app_op_charsets[j].len - APP_OP_CHARSET_LEN - 1)) == 0)
                        {
                            p_prop[*p_num_prop].parameters |= j << 3;
                            break;
                        }
                    }
                }
                /* check for property-specific parameters */
                else if (p_prs->p_tbl[prop_name].p_param_tbl != NULL)
                {

                    /* Check for "TYPE=" */
                    if (!app_op_strnicmp((char *)p_s, app_op_param_type_hdr, APP_OP_PARAM_TYPE_HDR_LEN))
                    {
                        p_s += APP_OP_PARAM_TYPE_HDR_LEN;
                    }

                    for (j = p_prs->p_tbl[prop_name].p_param_tbl[0].len; j > 0; j--)
                    {
                        if (app_op_strnicmp(p_prs->p_tbl[prop_name].p_param_tbl[j].p_name,
                            (char *) p_s, (p_prs->p_tbl[prop_name].p_param_tbl[j].len - 1)) == 0)
                        {
                            p_prop[*p_num_prop].parameters |= ((UINT32) 1) << (j + 7);
                            break;
                        }
                    }
                }
                else
                {
                    /* if this the start of the param */
                    if (!p_prop[*p_num_prop].p_param)
                        p_prop[*p_num_prop].p_param = p_s;
                    if (*p_e == ':')
                        p_prop[*p_num_prop].param_len += (p_e - p_s);
                    else
                        p_prop[*p_num_prop].param_len += (p_e - p_s + 1);
                }
            }
        }

        if (p_e == NULL)
        {
            break;
        }

        /* go to start of next property */
        p_s = p_e + 1;
        if ((p_e = app_op_nextline(p_s, p_end, qp)) == NULL)
        {
            break;
        }
        /* save property info */
        p_prop[*p_num_prop].p_data = p_s;
        p_prop[*p_num_prop].len = (UINT16) (p_e - p_s - 2);

        p_s = p_e;

        APP_OP_GET_PARAM_ENCOD(p_prop[*p_num_prop].parameters, i_e);

        /* If it is a photo property with base64 encoding */
        if (((app_op_encodings[i_e].p_name != NULL) && !strcmp(app_op_encodings[i_e].p_name, BASE64)) && prop_name == APP_OP_PROP_NAME_PHOTO)
        {
            /* find the additional length */
            p_prop[*p_num_prop].len = app_op_find_photo_encoding_length(p_s, p_end);

        }

        (*p_num_prop)++;
    }
    return result;
}


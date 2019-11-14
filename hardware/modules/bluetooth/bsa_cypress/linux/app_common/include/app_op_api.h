/*****************************************************************************
**
**  Name:           app_op_api.h
**
**  Description:    This is the common API for OPP object parsing
**                  in BSA sample apps
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**
*****************************************************************************/
#ifndef APP_OP_API_H
#define APP_OP_API_H

#include <bsa_rokid/bsa_api.h>

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

/* Object format */
#define APP_OP_VCARD21_FMT          1       /* vCard 2.1 */
#define APP_OP_VCARD30_FMT          2       /* vCard 3.0 */
#define APP_OP_VCAL_FMT             3       /* vCal 1.0 */
#define APP_OP_ICAL_FMT             4       /* iCal 2.0 */
#define APP_OP_VNOTE_FMT            5       /* vNote */
#define APP_OP_VMSG_FMT             6       /* vMessage */
#define APP_OP_OTHER_FMT            0xFF    /* other format */

typedef UINT8 tAPP_OP_FMT;

/* Object format mask */
#define APP_OP_VCARD21_MASK         0x01    /* vCard 2.1 */
#define APP_OP_VCARD30_MASK         0x02    /* vCard 3.0 */
#define APP_OP_VCAL_MASK            0x04    /* vCal 1.0 */
#define APP_OP_ICAL_MASK            0x08    /* iCal 2.0 */
#define APP_OP_VNOTE_MASK           0x10    /* vNote */
#define APP_OP_VMSG_MASK            0x20    /* vMessage */
#define APP_OP_ANY_MASK             0x40    /* Any type of object. */

typedef UINT8 tAPP_OP_FMT_MASK;

/* Status */
#define APP_OP_OK                   0       /* Operation successful. */
#define APP_OP_FAIL                 1       /* Operation failed. */
#define APP_OP_MEM                  2       /* Not enough memory to complete operation. */

typedef UINT8 tAPP_OP_STATUS;

/* vCal Object Type */
#define APP_OP_VCAL_EVENT           0       /* Object is an "Event" object. */
#define APP_OP_VCAL_TODO            1       /* Object is a "ToDo" object. */

typedef UINT8 tAPP_OP_VCAL;

/* vCard Property Names */
#define APP_OP_VCARD_ADR            1       /* Address. */
#define APP_OP_VCARD_EMAIL          2       /* Email address. */
#define APP_OP_VCARD_FN             3       /* Formatted name. */
#define APP_OP_VCARD_NOTE           4       /* Note. */
#define APP_OP_VCARD_NICKNAME       5       /* Nickname. */
#define APP_OP_VCARD_N              6       /* Name. */
#define APP_OP_VCARD_ORG            7       /* Organization. */
#define APP_OP_VCARD_TEL            8       /* Telephone number. */
#define APP_OP_VCARD_TITLE          9       /* Title. */
#define APP_OP_VCARD_URL            10      /* URL. */
#define APP_OP_VCARD_UID            11      /* Globally Unique Identifier. */
#define APP_OP_VCARD_BDAY           12      /* Birthday. */
#define APP_OP_VCARD_PHOTO          13      /* Photo. */
#define APP_OP_VCARD_SOUND          14      /* Sound. */
#define APP_OP_VCARD_CALL           15      /* Call date-time */
#define APP_OP_VCARD_CATEGORIES     16      /* Categories */
#define APP_OP_VCARD_PROID          17      /* Product ID */
#define APP_OP_VCARD_CLASS          18      /* Class information */
#define APP_OP_VCARD_SORT_STRING    19      /* String used for sorting operation */
#define APP_OP_VCARD_SPD            20      /* Speed-dial shortcut */
#define APP_OP_VCARD_UCI            21      /* Uniform Caller Identifier filed */
#define APP_OP_VCARD_BT_UID         22      /* Bluetooth Contact Unique Identifier */
#define APP_OP_VCARD_VERSION        23      /* vCard Version */

/* vCal Property Names */
#define APP_OP_VCAL_CATEGORIES      1       /* Categories of event. */
#define APP_OP_VCAL_COMPLETED       2       /* Time event is completed. */
#define APP_OP_VCAL_DESCRIPTION     3       /* Description of event. */
#define APP_OP_VCAL_DTEND           4       /* End date and time of event. */
#define APP_OP_VCAL_DTSTART         5       /* Start date and time of event. */
#define APP_OP_VCAL_DUE             6       /* Due date and time of event. */
#define APP_OP_VCAL_LOCATION        7       /* Location of event. */
#define APP_OP_VCAL_PRIORITY        8       /* Priority of event. */
#define APP_OP_VCAL_STATUS          9       /* Status of event. */
#define APP_OP_VCAL_SUMMARY         10      /* Summary of event. */
#define APP_OP_VCAL_LUID            11      /* Locally Unique Identifier. */

/* vNote Property Names */
#define APP_OP_VNOTE_BODY           1       /* Message body text. */
#define APP_OP_VNOTE_LUID           2       /* Locally Unique Identifier. */

/* Structure of the 32-bit parameters mask:
**
**                  + property-specific
** +reserved        |        + character set
** |                |        |     + encoding
** |                |        |     |
** 0000000000000000 00000000 00000 000
*/

/* Encoding Parameter */
#define APP_OP_ENC_QUOTED_PRINTABLE (1<<0)  /* Quoted-Printable encoding. */
#define APP_OP_ENC_8BIT             (2<<0)  /* 8-bit encoding */
#define APP_OP_ENC_BASE64           (3<<0)  /* Base64 encoding */
#define APP_OP_ENC_BINARY           (4<<0)  /* Binary encoding */

/* Character Set Parameter */
#define APP_OP_CHAR_BIG5            (1<<3)  /* Big5 character set. */
#define APP_OP_CHAR_EUC_JP          (2<<3)  /* EUC-JP character set. */
#define APP_OP_CHAR_EUC_KR          (3<<3)  /* EUC-KR character set. */
#define APP_OP_CHAR_GB2312          (4<<3)  /* GB2312 character set. */
#define APP_OP_CHAR_ISO_2022_JP     (5<<3)  /* ISO-2022-JP character set. */
#define APP_OP_CHAR_ISO_8859_1      (6<<3)  /* ISO-8859-1 character set. */
#define APP_OP_CHAR_ISO_8859_2      (7<<3)  /* ISO-8859-2 character set. */
#define APP_OP_CHAR_ISO_8859_3      (8<<3)  /* ISO-8859-3 character set. */
#define APP_OP_CHAR_ISO_8859_4      (9<<3)  /* ISO-8859-4 character set. */
#define APP_OP_CHAR_ISO_8859_5      (10<<3) /* ISO-8859-5 character set. */
#define APP_OP_CHAR_ISO_8859_6      (11<<3) /* ISO-8859-6 character set. */
#define APP_OP_CHAR_ISO_8859_7      (12<<3) /* ISO-8859-7 character set. */
#define APP_OP_CHAR_ISO_8859_8      (13<<3) /* ISO-8859-8 character set. */
#define APP_OP_CHAR_KOI8_R          (14<<3) /* KOI8-R character set. */
#define APP_OP_CHAR_SHIFT_JIS       (15<<3) /* Shift_JIS character set. */
#define APP_OP_CHAR_UTF_8           (16<<3) /* UTF-8 character set. */

/* Address Type Parameter */
#define APP_OP_ADR_DOM              (1<<8)  /* Domestic address. */
#define APP_OP_ADR_INTL             (1<<9)  /* International address. */
#define APP_OP_ADR_POSTAL           (1<<10) /* Postal address. */
#define APP_OP_ADR_PARCEL           (1<<11) /* Parcel post address. */
#define APP_OP_ADR_HOME             (1<<12) /* Home address. */
#define APP_OP_ADR_WORK             (1<<13) /* Work address. */

/* EMAIL Type Parameter */
#define APP_OP_EMAIL_PREF           (1<<8)  /* Preferred email. */
#define APP_OP_EMAIL_INTERNET       (1<<9)  /* Internet email. */
#define APP_OP_EMAIL_X400           (1<<10) /* x400 emaill */

/* Telephone Number Type Parameter */
#define APP_OP_TEL_PREF             (1<<8)  /* Preferred number. */
#define APP_OP_TEL_WORK             (1<<9)  /* Work number. */
#define APP_OP_TEL_HOME             (1<<10) /* Home number. */
#define APP_OP_TEL_VOICE            (1<<11) /* Voice number. */
#define APP_OP_TEL_FAX              (1<<12) /* Fax number. */
#define APP_OP_TEL_MSG              (1<<13) /* Message number. */
#define APP_OP_TEL_CELL             (1<<14) /* Cell phone number. */
#define APP_OP_TEL_PAGER            (1<<15) /* Pager number. */

/* Photo Parameter */
#define APP_OP_PHOTO_VALUE_URI      (1<<8)  /* URI value */
#define APP_OP_PHOTO_VALUE_URL      (1<<9)  /* URL value */
#define APP_OP_PHOTO_TYPE_JPEG      (1<<10) /* JPEG photo */
#define APP_OP_PHOTO_TYPE_GIF       (1<<11) /* GIF photo */

/* Sound Parameter */
#define APP_OP_SOUND_VALUE_URI      (1<<8)  /* URI value */
#define APP_OP_SOUND_VALUE_URL      (1<<9)  /* URL value */
#define APP_OP_SOUND_TYPE_BASIC     (1<<10) /* BASIC sound */
#define APP_OP_SOUND_TYPE_WAVE      (1<<11) /* WAVE sound */

/* vCard filter mask */
#define APP_OP_FILTER_VERSION  (1<<0)  /* vCard Version */
#define APP_OP_FILTER_FN       (1<<1)  /* Formatted Name */
#define APP_OP_FILTER_N        (1<<2)  /* Structured Presentation of Name */
#define APP_OP_FILTER_PHOTO    (1<<3)  /* Associated Image or Photo */
#define APP_OP_FILTER_BDAY     (1<<4)  /* Birthday */
#define APP_OP_FILTER_ADR      (1<<5)  /* Delivery Address */
#define APP_OP_FILTER_LABEL    (1<<6)  /* Delivery */
#define APP_OP_FILTER_TEL      (1<<7)  /* Telephone Number */
#define APP_OP_FILTER_EMAIL    (1<<8)  /* Electronic Mail Address */
#define APP_OP_FILTER_MAILER   (1<<9)  /* Electronic Mail */
#define APP_OP_FILTER_TZ       (1<<10)  /* Time Zone */
#define APP_OP_FILTER_GEO      (1<<11) /* Geographic Position */
#define APP_OP_FILTER_TITLE    (1<<12) /* Job */
#define APP_OP_FILTER_ROLE     (1<<13) /* Role within the Organization */
#define APP_OP_FILTER_LOGO     (1<<14) /* Organization Logo */
#define APP_OP_FILTER_AGENT    (1<<15) /* vCard of Person Representing */
#define APP_OP_FILTER_ORG      (1<<16) /* Name of Organization */
#define APP_OP_FILTER_NOTE     (1<<17) /* Comments */
#define APP_OP_FILTER_REV      (1<<18) /* Revision */
#define APP_OP_FILTER_SOUND    (1<<19) /* Pronunciation of Name */
#define APP_OP_FILTER_URL      (1<<20) /* Uniform Resource Locator */
#define APP_OP_FILTER_UID      (1<<21) /* Unique ID */
#define APP_OP_FILTER_KEY      (1<<22) /* Public Encryption Key */
#define APP_OP_FILTER_NICKNAME (1<<23) /* Nickname */
#define APP_OP_FILTER_CATEGORIES (1<<24) /* Categories */
#define APP_OP_FILTER_PROID    (1<<25) /* Product ID */
#define APP_OP_FILTER_CLASS    (1<<26) /* Class Information */
#define APP_OP_FILTER_SORT_STRING (1<<27) /* String used for sorting operations */
#define APP_OP_FILTER_TIME_STAMP (1<<28) /* Time Stamp */
#define APP_OP_FILTER_X_BT_SPEEDDIALKEY     (1<<29) /* Speed-dial shortcut */
#define APP_OP_FILTER_X_BT_UCI              (1<<30) /* Uniform Caller Identifier field */
#define APP_OP_FILTER_X_BT_UID              (1<<31) /* Bluetooth Contact Unique Identifier */
#define APP_OP_FILTER_ALL      (0)

/* This structure describes an object property, or individual item, inside an object. */
typedef struct
{
    UINT8       *p_data;            /* Pointer to property data. */
    UINT32      parameters;         /* Property parameters. */
    UINT16      name;               /* Property name. */
    UINT16      len;                /* Length of data. */
    UINT8       *p_param;           /* Pointer to the Parameters */
    UINT16      param_len;          /* Param Len */
} tAPP_OP_PROP;


/* Access response types */
#define APP_OP_ACCESS_ALLOW     0   /* Allow the requested operation */
#define APP_OP_ACCESS_FORBID    1   /* Disallow the requested operation */
#define APP_OP_ACCESS_NONSUP    2   /* Requested operation is not supported */

typedef UINT8 tAPP_OP_ACCESS;

/* Access event operation types */
#define APP_OP_OPER_PUSH        1
#define APP_OP_OPER_PULL        2

typedef UINT8 tAPP_OP_OPER;


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         app_op_build_card
**
** Description      Build a vCard object. The input to this function is
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
extern tAPP_OP_STATUS app_op_build_card(UINT8 *p_card, UINT16 *p_len,
                                              tAPP_OP_FMT fmt,
                                              tAPP_OP_PROP *p_prop,
                                              UINT8 num_prop);

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
extern tAPP_OP_STATUS app_op_build_note(UINT8 *p_note, UINT16 *p_len,
                                              tAPP_OP_PROP *p_prop,
                                              UINT8 num_prop);

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
extern tAPP_OP_STATUS app_op_build_cal(UINT8 *p_cal, UINT16 *p_len,
                                             tAPP_OP_PROP *p_prop,
                                             UINT8 num_prop,
                                             tAPP_OP_VCAL vcal_type);

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
extern tAPP_OP_STATUS app_op_parse_card(tAPP_OP_PROP *p_prop,
                                              UINT8 *p_num_prop, UINT8 *p_card,
                                              UINT32 len);

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
extern tAPP_OP_STATUS app_op_check_card(tAPP_OP_PROP *p_prop, UINT8 num_prop);

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
extern tAPP_OP_STATUS app_op_get_card_property(UINT8 *p_value, UINT16 *p_len, tAPP_OP_PROP *p_prop,
                                     UINT8 num_prop, UINT8 *p_name);

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
extern tAPP_OP_STATUS app_op_parse_note(tAPP_OP_PROP *p_prop,
                                              UINT8 *p_num_prop,
                                              UINT8 *p_note, UINT32 len);

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
extern tAPP_OP_STATUS app_op_parse_cal(tAPP_OP_PROP *p_prop,
                                             UINT8 *p_num_prop, UINT8 *p_cal,
                                             UINT32 len, tAPP_OP_VCAL *p_vcal_type);

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
extern void app_op_set_card_selector_operator(UINT64 selector, UINT8 selector_op);

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
extern void app_op_set_card_prop_filter_mask(UINT64 mask);

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
extern void app_op_get_pb_size(UINT8 *p_start, UINT8 *p_end, UINT16 *pb_size);

#ifdef __cplusplus
}
#endif

#endif /* APP_OP_API_H */


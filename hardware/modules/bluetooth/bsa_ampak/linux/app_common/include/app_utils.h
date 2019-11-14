/*****************************************************************************
**
**  Name:           app_utils.h
**
**  Description:    Bluetooth sample applications utility functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

/* idempotency */
#ifndef APP_UTILS_H
#define APP_UTILS_H

/* self sufficiency */
/* for printf */
#include <stdio.h>
/* for BD_ADDR and DEV_CLASS */
#include <bsa_rokid/bt_types.h>
/* for scru_dump_hex */
#include <bsa_rokid/bsa_trace.h>

/* Macro to retrieve the number of elements in a statically allocated array */
#define APP_NUM_ELEMENTS(__a) ((int)(sizeof(__a)/sizeof(__a[0])))

/* Macro to test if bits in __b are all set in __v */
#define APP_BITS_SET(__v, __b) (((__v) & (__b)) == (__b))

/* Macro to print an error message */
#define APP_ERROR0(format)                                                      \
do {                                                                            \
    app_print_error("%s: " format "\n", __func__);                              \
} while (0)

#define APP_ERROR1(format, ...)                                                 \
do {                                                                            \
    app_print_error("%s: " format "\n", __func__, __VA_ARGS__);                 \
} while (0)

#ifdef APP_TRACE_NODEBUG

#define APP_DEBUG0(format) do {} while(0)
#define APP_DEBUG1(format, ...) do {} while(0)
#define APP_DUMP(prefix,pointer,length) do {} while(0)

#else /* APP_TRACE_NODEBUG */

/* Macro to print a debug message */
#define APP_DEBUG0(format)                                                      \
do {                                                                            \
    app_print_debug("%s: " format "\n", __func__);                              \
} while (0)

#define APP_DEBUG1(format, ...)                                                 \
do {                                                                            \
    app_print_debug("%s: " format "\n", __func__, __VA_ARGS__);                 \
} while (0)

#define APP_DUMP(prefix,pointer,length)                                         \
do                                                                              \
{                                                                               \
    scru_dump_hex(pointer, prefix, length, TRACE_LAYER_NONE, TRACE_TYPE_DEBUG); \
} while(0)                                                                      \

#endif /* !APP_TRACE_NODEBUG */

/* Macro to print an information message */
#define APP_INFO0(format)                                                       \
do {                                                                            \
    app_print_info(format "\n");                                                \
} while (0)

#define APP_INFO1(format, ...)                                                  \
do {                                                                            \
    app_print_info(format "\n", __VA_ARGS__);                                   \
} while (0)

/*Rokid define*/
#define BT_MAX_DEV_NUM			10

typedef struct bt_device {
	unsigned char		bd_addr[6];
	unsigned char		bd_name[256];
	unsigned char 		bd_class[3];
	unsigned char		in_use;
}bd_property;

typedef struct bt_device_group {
	bd_property			bd_paired[BT_MAX_DEV_NUM];	
	bd_property			bd_discoveryed[BT_MAX_DEV_NUM];
}bd_g;


/*******************************************************************************
 **
 ** Function         app_get_cod_string
 **
 ** Description      This function is used to get readable string from Class of device
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
char *app_get_cod_string(const DEV_CLASS class_of_device);

/*******************************************************************************
 **
 ** Function         app_get_choice
 **
 ** Description      Wait for a choice from user
 **
 ** Parameters       The string to print before waiting for input
 **
 ** Returns          The number typed by the user, or -1 if the value type was
 **                  not parsable
 **
 *******************************************************************************/
int app_get_choice(const char *querystring);

/*******************************************************************************
 **
 ** Function        app_get_string
 **
 ** Description     Ask the user to enter a string value
 **
 ** Parameters      querystring: to print before waiting for input
 **                 str: the char buffer to fill with user input
 **                 len: the length of the char buffer
 **
 ** Returns         The length of the string entered not including last NULL char
 **                 negative value in case of error
 **
 *******************************************************************************/
int app_get_string(const char *querystring, char *str, int len);

/*******************************************************************************
 **
 ** Function        app_get_hex_data
 **
 ** Description     Ask the user to enter hexa data
 **
 ** Parameters      querystring: to print before waiting for input
 **                 b_buf: the buffer to fill with user input
 **                 len: the length of the buffer
 **
 ** Returns         The length of the data entered or -1 if not correctly formated
 **
 *******************************************************************************/
int app_get_hex_data(const char *querystring, UINT8 *b_buf, int buf_len);

/*******************************************************************************
 **
 ** Function        app_print_info
 **
 ** Description     This function is used to print an application information message
 **
 ** Parameters:     format: Format string
 **                 optional parameters
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_print_info(char *format, ...);

/*******************************************************************************
 **
 ** Function        app_print_debug
 **
 ** Description     This function is used to print an application debug message
 **
 ** Parameters:     format: Format string
 **                 optional parameters
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_print_debug(char *format, ...);

/*******************************************************************************
 **
 ** Function        app_print_error
 **
 ** Description     This function is used to print an application error message
 **
 ** Parameters:     format: Format string
 **                 optional parameters
 **
 ** Returns         void
 **
 *******************************************************************************/
void app_print_error(char *format, ...);

/*******************************************************************************
 **
 ** Function        app_file_size
 **
 ** Description     Retrieve the size of a file identified by descriptor
 **
 ** Parameters:     fd: File descriptor
 **
 ** Returns         File size if successful or negative error number
 **
 *******************************************************************************/
int app_file_size(int fd);

/*******************************************************************************
 **
 ** Function        app_hex_char
 **
 ** Description     Convert a hex char into its value
 **
 ** Parameters:     c : Hex char
 **
 ** Returns         Value of the hex char
 **
 *******************************************************************************/
UINT8 app_hex_char(UINT8 c);

/*******************************************************************************
 **
 ** Function        app_hex_read
 **
 ** Description     Read the next record from an Intel Hex file
 **
 ** Parameters:     p_file : Pointer to the FILE to read from
 **                 p_type : returned type of the record
 **                 p_offset : returned offset of the record
 **                 p_data: returned data of the record (used internally to store file line)
 **                 p_len: in: size of p_data buffer, out: length of the record data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hex_read(FILE *p_file, UINT8 *p_type, UINT16 *p_offset, UINT8 *p_data, UINT16 *p_len);

#endif


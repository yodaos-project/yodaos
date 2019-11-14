/*****************************************************************************
 **
 **  Name:           app_utils.c
 **
 **  Description:    Bluetooth utils functions
 **
 **  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>

#include <bsa_rokid/bsa_api.h>

#include "app_xml_param.h"
#include "app_utils.h"

/*
 * Definitions
 */
/* Terminal Attribute definitions */
#define RESET       0
#define BRIGHT      1
#define DIM         2
#define UNDERLINE   3
#define BLINK       4
#define REVERSE     7
#define HIDDEN      8

/* Terminal Color definitions */
#define BLACK       0
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define MAGENTA     5
#define CYAN        6
#define WHITE       7

/* Length of the string containing a TimeStamp */
#define APP_TIMESTAMP_LEN 80
/*
 * Local functions
 */

#ifdef APP_TRACE_COLOR
static void app_set_trace_color(int attr, int fg, int bg);
#endif
#ifdef APP_TRACE_TIMESTAMP
static char *app_get_time_stamp(char *p_buffer, int buffer_size);
#endif

/*******************************************************************************
 **
 ** Function        app_get_cod_string
 **
 ** Description     This function is used to get readable string from Class of device
 **
 ** Parameters      class_of_device: The Class of device to decode
 **
 ** Returns         Pointer on string containing device type
 **
 *******************************************************************************/
char *app_get_cod_string(const DEV_CLASS class_of_device)
{
    UINT8 major;

    /* Extract Major Device Class value */
    BTM_COD_MAJOR_CLASS(major, class_of_device);

    switch (major)
    {
    case BTM_COD_MAJOR_MISCELLANEOUS:
        return "Misc device";
        break;
    case BTM_COD_MAJOR_COMPUTER:
        return "Computer";
        break;
    case BTM_COD_MAJOR_PHONE:
        return "Phone";
        break;
    case BTM_COD_MAJOR_LAN_ACCESS_PT:
        return "Access Point";
        break;
    case BTM_COD_MAJOR_AUDIO:
        return "Audio/Video";
        break;
    case BTM_COD_MAJOR_PERIPHERAL:
        return "Peripheral";
        break;
    case BTM_COD_MAJOR_IMAGING:
        return "Imaging";
        break;
    case BTM_COD_MAJOR_WEARABLE:
        return "Wearable";
        break;
    case BTM_COD_MAJOR_TOY:
        return "Toy";
        break;
    case BTM_COD_MAJOR_HEALTH:
        return "Health";
        break;
    default:
        return "Unknown device type";
        break;
    }
    return NULL;
}


/*******************************************************************************
 **
 ** Function        app_get_choice
 **
 ** Description     Wait for a choice from user
 **
 ** Parameters      The string to print before waiting for input
 **
 ** Returns         The number typed by the user, or -1 if the value type was
 **                 not parsable
 **
 *******************************************************************************/
int app_get_choice(const char *querystring)
{
    int neg, value, c, base;
    int count = 0;

    base = 10;
    neg = 1;
    printf("%s => ", querystring);
    value = 0;
    do
    {
        c = getchar();

        if ((count == 0) && (c == '\n'))
        {
            return -1;
        }
        count ++;

        if ((c >= '0') && (c <= '9'))
        {
            value = (value * base) + (c - '0');
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            value = (value * base) + (c - 'a' + 10);
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            value = (value * base) + (c - 'A' + 10);
        }
        else if (c == '-')
        {
            neg *= -1;
        }
        else if (c == 'x')
        {
            base = 16;
        }

    } while ((c != EOF) && (c != '\n'));

    return value * neg;
}

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
int app_get_string(const char *querystring, char *str, int len)
{
    int c, index;

    if (querystring)
    {
        printf("%s => ", querystring);
    }

    index = 0;
    do
    {
        c = getchar();
        if (c == EOF)
        {
            return -1;
        }
        if ((c != '\n') && (index < (len - 1)))
        {
            str[index] = (char)c;
            index++;
        }
    } while (c != '\n');

    str[index] = '\0';
    return index;
}

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
int app_get_hex_data(const char *querystring, UINT8 *b_buf, int buf_len)
{
    int index;
    int strlen;
    char str[200];
    char *p;
    UINT8 value;

    strlen = app_get_string(querystring, str, sizeof(str));

    index = 0;
    p = str;
    while((strlen >= 2) && (index < buf_len))
    {
        /* If Character is Hexadecimal */
        if (isxdigit(*p))
        {
            /* MSB nibble */
            value = app_hex_char(*p++) << 4;
            strlen--;
            /* LSB nibble */
            if (isxdigit(*p))
            {
                value |= app_hex_char(*p++);
                strlen--;
            }
            else
            {
                APP_ERROR1("bad Hex char 0x%02X\n", *p);
                return -1;
            }
            b_buf[index++] = value;
        }
        else
        {
            APP_ERROR1("bad Hex char 0x%02X\n", *p);
            return -1;
        }

        /* If more data if string */
        if (strlen >= 1)
        {
            /* If space or other 'separation' byte */
            if ((*p == ' ') ||
                (*p == '.') ||
                (*p == '-') ||
                (*p == ':') ||
                (*p == '\r') ||
                (*p == '\n'))
            {
                p++;            /* Skip it */
                strlen--;
            }
        }
    }

    /* If entered hexa data does not fit in user buffer */
    if (strlen > 0)
    {
        APP_ERROR1("strlen>0 %d %x\n", strlen, *p);
        return -1;
    }

    return index;
}
#ifdef APP_TRACE_TIMESTAMP
/*******************************************************************************
 **
 ** Function        app_get_time_stamp
 **
 ** Description     This function is used to get a timestamp
 **
 ** Parameters:     p_buffer: buffer to write the timestamp
 **                 buffer_size: buffer size
 **
 ** Returns         pointer on p_buffer
 **
 *******************************************************************************/
static char *app_get_time_stamp(char *p_buffer, int buffer_size)
{
    char time_string[80];

    /* use GKI_get_time_stamp to have the same clock than the BSA client traces */
    GKI_get_time_stamp((INT8 *)time_string);

    snprintf(p_buffer, buffer_size, "%s", time_string);

    return p_buffer;
}
#endif

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
void app_print_info(char *format, ...)
{
    va_list ap;
#ifdef APP_TRACE_TIMESTAMP
    char time_stamp[APP_TIMESTAMP_LEN];
#endif

#ifdef APP_TRACE_COLOR
    app_set_trace_color(RESET, BLACK, WHITE);
#endif

#ifdef APP_TRACE_TIMESTAMP
    app_get_time_stamp(time_stamp, sizeof(time_stamp));
    printf("INFO@%s: ", time_stamp);
#endif

    va_start(ap, format);
    vprintf(format,ap);
    va_end(ap);

#ifdef APP_TRACE_COLOR
    app_set_trace_color(RESET, BLACK, WHITE);
#endif
}

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
void app_print_debug(char *format, ...)
{
    va_list ap;
#ifdef APP_TRACE_TIMESTAMP
    char time_stamp[APP_TIMESTAMP_LEN];
#endif

#ifdef APP_TRACE_COLOR
    app_set_trace_color(RESET, GREEN, WHITE);
#endif

#ifdef APP_TRACE_TIMESTAMP
    app_get_time_stamp(time_stamp, sizeof(time_stamp));
    printf("DEBUG@%s: ", time_stamp);
#else
    printf("DEBUG: ");
#endif

    va_start(ap, format);
    vprintf(format,ap);
    va_end(ap);

#ifdef APP_TRACE_COLOR
    app_set_trace_color(RESET, BLACK, WHITE);
#endif
}

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
void app_print_error(char *format, ...)
{
    va_list ap;
#ifdef APP_TRACE_TIMESTAMP
    char time_stamp[APP_TIMESTAMP_LEN];
#endif

#ifdef APP_TRACE_COLOR
    app_set_trace_color(RESET, RED, WHITE);
#endif

#ifdef APP_TRACE_TIMESTAMP
    app_get_time_stamp(time_stamp, sizeof(time_stamp));
    printf("ERROR@%s: ", time_stamp);
#else
    printf("ERROR: ");
#endif

    va_start(ap, format);
    vprintf(format,ap);
    va_end(ap);

#ifdef APP_TRACE_COLOR
    app_set_trace_color(RESET, BLACK, WHITE);
#endif
}

#ifdef APP_TRACE_COLOR
/*******************************************************************************
 **
 ** Function        app_set_trace_color
 **
 ** Description     This function changes the text color
 **
 ** Parameters:     attribute: Text attribute (reset/Blink/etc)
 **                 foreground: foreground color
 **                 background: background color
 **
 ** Returns         void
 **
 *******************************************************************************/
static void app_set_trace_color(int attribute, int foreground, int background)
{
    char command[13];

    /* Command is the control command to the terminal */
    snprintf(command, sizeof(command), "%c[%d;%d;%dm", 0x1B, attribute, foreground + 30, background + 40);
    printf("%s", command);
}
#endif

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
int app_file_size(int fd)
{
    struct stat file_stat;
    int rc = -1;

    rc = fstat(fd, &file_stat);

    if (rc >= 0)
    {
        /* Retrieve the size of the file */
        rc = file_stat.st_size;
    }
    else
    {
        APP_ERROR1("could not fstat(fd=%d)", fd);
    }
    return rc;
}

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
UINT8 app_hex_char(UINT8 c)
{
    if (c >= 'a')
    {
        return (c - 'a' + 10);
    }
    else if (c >= 'A')
    {
        return (c - 'A' + 10);
    }
    else
    {
        return (c - '0');
    }

}

/*******************************************************************************
 **
 ** Function        app_hex_convert
 **
 ** Description     Convert the record from an Intel Hex file
 **
 ** Parameters:     p_type : returned type of the record
 **                 p_offset : returned offset of the record
 **                 p_data: in: ascii line, out: record data
 **                 p_len: in: size of p_data buffer, out: length of record data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_hex_convert(UINT8 *p_type, UINT16 *p_offset, UINT8 *p_data, UINT16 *p_len)
{
    int r_index, w_index, size;
    UINT8 checksum;
    unsigned int min_len;

    min_len = (strlen((char *)p_data) < *p_len)?strlen((char *)p_data):*p_len;

    if (min_len < 11)
    {
        APP_ERROR0("Record too short");
        return -1;
    }
    if (p_data[0] != ':')
    {
        APP_ERROR0("Record does not start with colon character");
        return -1;
    }

    /* Start converting after the colon character */
    r_index = 1;
    w_index = 0;
    while (p_data[r_index] != 0)
    {
        if ((r_index % 2) == 0)
        {
            p_data[w_index]  = app_hex_char(p_data[r_index-1]) * 16;
            p_data[w_index] += app_hex_char(p_data[r_index]);
            w_index++;
        }
        r_index++;
    }

    /* Size of whole binary is: record + 1b length + 2b offset + 1b type + 1b checksum */
    size = p_data[0] + 5;
    /* Check that the size matches exactly the ASCII line size (to prevent segfaulting) */
    if (size > w_index)
    {
        APP_ERROR1("Line length (%d) is smaller than record data size (%d)", w_index, size);
        return -1;
    }
    /* Save the record information: data length, the offset and the type */
    *p_len = p_data[0];
    *p_offset = (p_data[1] * 256) + p_data[2];
    *p_type = p_data[3];

    for (checksum = 0, w_index = 0, r_index = 0; r_index < size; r_index++)
    {
        checksum += p_data[r_index];
        if (r_index > 3)
        {
            p_data[w_index++] = p_data[r_index];
        }
    }

    if (checksum != 0)
    {
        APP_ERROR0("record CRC failed");
        return -1;
    }
    return 0;
}

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
int app_hex_read(FILE *p_file, UINT8 *p_type, UINT16 *p_offset, UINT8 *p_data, UINT16 *p_len)
{
    if (fgets((char *)p_data, *p_len, p_file) == NULL)
    {
        /* End of file */
        return -1;
    }

    return app_hex_convert(p_type, p_offset, p_data, p_len);
}

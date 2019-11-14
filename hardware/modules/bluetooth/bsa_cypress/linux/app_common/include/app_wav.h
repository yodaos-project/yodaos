/*****************************************************************************
**
**  Name:           app_wav.h
**
**  Description:    WAV file related functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

/* Idempotence guarantee */
#ifndef APP_WAV_H
#define APP_WAV_H

/* Self sufficiency */
#include <bsa_rokid/bsa_av_api.h>

typedef struct
{
    tBSA_AV_CODEC_ID codec;
    unsigned char   nb_channels;
    unsigned char   stereo_mode;
    unsigned long   sample_rate;
    unsigned short  bits_per_sample;
} tAPP_WAV_FILE_FORMAT;

/*******************************************************************************
 **
 ** Function        app_wav_format
 **
 ** Description     Read the WAV file header from an unopened file
 **
 ** Parameters      p_fname: name the file to parse
 **                 p_format: format to fill with parsed values
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_wav_format(const char *p_fname, tAPP_WAV_FILE_FORMAT *p_format);

/*******************************************************************************
 **
 ** Function        app_wav_read_data
 **
 ** Description     Read data from the WAV file
 **
 ** Parameters      fd: file descriptor of the file to read from
 **                 p_format: WAV format returned by app_wav_read_format
 **                 p_data: buffer to fill with the data
 **                 len: length to read
 **
 ** Returns         Number of bytes read, -1 in case of error, 0 if end of file
 **
 *******************************************************************************/
int app_wav_read_data(int fd, const tAPP_WAV_FILE_FORMAT *p_format, char *p_data, int len);

/*******************************************************************************
 **
 ** Function        app_wav_open_file
 **
 ** Description     Open a WAV file and parse the header
 **
 ** Parameters      p_fname: name the file to parse
 **                 p_format: format to fill with parsed values
 **
 ** Returns         file descriptor if successful, -1 in case of error
 **
 *******************************************************************************/
int app_wav_open_file(const char *p_fname, tAPP_WAV_FILE_FORMAT *p_format);

/*******************************************************************************
 **
 ** Function        app_wav_create_file
 **
 ** Description     Create a wave file with proper header
 **
 ** Parameters      p_fname: name of the file to create
 **                 flags: extra flags to add to O_RDWR and O_CREAT (O_EXCL for
 **                        example if you want to prevent overwriting a file)
 **
 ** Returns         The file descriptor of the file created (-1 in case of error)
 **
 *******************************************************************************/
int app_wav_create_file(const char *p_fname, int flags);

/*******************************************************************************
 **
 ** Function        app_wav_close_file
 **
 ** Description     Update WAV file header and close file
 **
 ** Parameters      fd: file descriptor
 **                 p_format: structure containing the format of the samples
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_wav_close_file(int fd, const tAPP_WAV_FILE_FORMAT *p_format);

#endif


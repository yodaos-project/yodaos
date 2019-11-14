/*****************************************************************************
**
**  Name:           app_wav.c
**
**  Description:    WAV file related functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "app_utils.h"
#include "app_wav.h"

typedef struct
{
    BOOLEAN is_be;
} tAPP_WAV_CB;

tAPP_WAV_CB app_wav_cb =
{
    FALSE
};

#define APP_WAVE_HDR_SIZE 44

const unsigned char app_wav_hdr[APP_WAVE_HDR_SIZE] =
{
    'R', 'I', 'F', 'F',         /* Chunk ID : "RIFF" */
    '\0', '\0', '\0', '\0',     /* Chunk size = file size - 8 */
    'W', 'A', 'V', 'E',         /* Chunk format : "WAVE" */
    'f', 'm', 't', ' ',         /*   Subchunk ID : "fmt " */
    0x10, 0x00, 0x00, 0x00,     /*   Subchunk size : 16 for PCM format */
    0x01, 0x00,                 /*     Audio format : 1 means PCM linear */
    '\0', '\0',                 /*     Number of channels */
    '\0', '\0', '\0', '\0',     /*     Sample rate */
    '\0', '\0', '\0', '\0',     /*     Byte rate = SampleRate * NumChannels * BitsPerSample/8 */
    '\0', '\0',                 /*     Blockalign = NumChannels * BitsPerSample/8 */
    '\0', '\0',                 /*     Bitpersample */
    'd', 'a', 't', 'a',         /*   Subchunk ID : "data" */
    '\0', '\0', '\0', '\0'      /*   Subchunk size = NumSamples * NumChannels * BitsPerSample/8 */
};

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
int app_wav_format(const char *p_fname, tAPP_WAV_FILE_FORMAT *p_format)
{
    int fd;

    fd = app_wav_open_file(p_fname, p_format);

    if (fd >= 0)
    {
        close(fd);
        return 0;
    }

    return -1;
}

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
int app_wav_read_data(int fd, const tAPP_WAV_FILE_FORMAT *p_format, char *p_data, int len)
{
    int nbytes;
    int index;
    int bytes_per_sample = (p_format->bits_per_sample + 7)/8;
    unsigned short tmp16;
    unsigned short *tmp16_ptr;
    unsigned long tmp32;
    unsigned long *tmp32_ptr;

    nbytes = read(fd, p_data, len);

    /* Check that the buffer is aligned */
    if (((bytes_per_sample == 2) && ((size_t)p_data & 1)) ||
        ((bytes_per_sample == 4) && ((size_t)p_data & 3)))
    {
        APP_DEBUG1("Audio buffer start is not aligned on PCM sample (%d bytes) word boundary, this could degrade system perf (%p)", 
                bytes_per_sample, p_data);
    }

    if (nbytes > 0 && p_format->codec == BSA_AV_CODEC_PCM)
    {
        switch (bytes_per_sample)
        {
        case 1:
            break;
        case 2:
            if (nbytes & 1)
            {
                APP_DEBUG1("Number of PCM samples read not multiple of sample size(%d)", bytes_per_sample);
            }
            if (app_wav_cb.is_be)
            {
                tmp16_ptr = (unsigned short *)p_data;
                for (index = 0; index < nbytes; index += 2)
                {
                    tmp16 = *tmp16_ptr;
                    *tmp16_ptr = (tmp16 << 8 | tmp16 >> 8);
                    tmp16_ptr++;
                }
            }
            break;
        case 4:
            if (nbytes & 3)
            {
                APP_DEBUG1("Number of PCM samples read not multiple of sample size(%d)", bytes_per_sample);
            }
            if (app_wav_cb.is_be)
            {
                tmp32_ptr = (unsigned long *)p_data;
                for (index = 0; index < nbytes; index += 4)
                {
                    tmp32 = *tmp32_ptr;
                    *tmp32_ptr = (tmp32 >> 24) |
                        ((tmp32 >> 8) & 0xff00) |
                        ((tmp32 << 8) & 0xff0000) |
                        ((tmp32 << 24) & 0xff000000);
                    tmp32_ptr++;
                }
            }
            break;
        default:
            APP_ERROR1("Sample size is not supported (%d)", bytes_per_sample);
            break;
        }
    }
    return nbytes;
}

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
int app_wav_open_file(const char *p_fname, tAPP_WAV_FILE_FORMAT *p_format)
{
    int             fd;
    ssize_t         totalsize;
    ssize_t         size;
    unsigned char   riff[12];
    unsigned char   fmt_params[40];
    unsigned char   data[8];
    off_t           file_size;
    struct stat     stat_info;
    unsigned long   tmpU32;
    unsigned short  tmpU16;

    /* Check dynamically if computer is little or big endian */
    tmpU32 = 1;
    if (*((char *)(&tmpU32)) == 0) app_wav_cb.is_be = TRUE;

    if (p_format == NULL)
    {
        APP_ERROR0("format is NULL");
        return -1;
    }
    if (p_fname == NULL)
    {
        APP_ERROR0("no filename");
        return -1;
    }
    if ((fd = open(p_fname, O_RDONLY)) < 0)
    {
        APP_ERROR1("open(%s) failed: %d", p_fname, errno);
        return -1;
    }
    if (fstat(fd, &stat_info) < 0)
    {
        APP_ERROR1("stat(%s) failed: %d", p_fname, errno);
        goto app_wav_open_file_error;
    }
    file_size = stat_info.st_size;


    /* Read the RIFF header */
    size = read(fd, riff, sizeof(riff));
    if (size < 0)
    {
        APP_ERROR1("read(%s) failed: %d", p_fname, errno);
        goto app_wav_open_file_error;
    }

    if (size != sizeof(riff))
    {
        APP_ERROR1("Length read does not match RIFF header (%d != %d)", (int)size, sizeof(riff));
        goto app_wav_open_file_error;
    }

    /* Check ChunkId field */
    if (memcmp(&riff[0], "RIFF", 4))
    {
        APP_ERROR0("RIFF not found");
        goto app_wav_open_file_error;
    }

    /* Read ChunkSize field */
    tmpU32 = riff[4];
    tmpU32 |= (riff[5] << 8);
    tmpU32 |= (riff[6] << 16);
    tmpU32 |= (unsigned long) (riff[7] << 24);

    /* Check ChunkSize field */
    if ((tmpU32 + 8) != (UINT32)file_size)
    {
        APP_DEBUG1("WARNING: RIFF chunk size (%lu + 8) does not match file size(%lu)", tmpU32, file_size);
    }

    /* Check WAVE ID field */
    if (memcmp(&riff[8], "WAVE", 4))
    {
        APP_ERROR0("WAVE not found");
        goto app_wav_open_file_error;
    }

    /* RIFF header */
    totalsize = 12;

    /* Invalid value to indicate format chunk was not parsed */
    p_format->sample_rate = 0;

    /* Read all the chunks until data is found */
    do
    {
        /* Read next chunk header */
        size = read(fd, data, sizeof(data));
        if (size < 0)
        {
            APP_ERROR1("read(%s) failed: %d", p_fname, errno);
            goto app_wav_open_file_error;
        }
        if (size != sizeof(data))
        {
            APP_ERROR1("Length read does not match header (%d != %d)", (int)size, sizeof(data));
            goto app_wav_open_file_error;
        }

        /* ChunkSize */
        tmpU32 = data[4];
        tmpU32 |= (data[5] << 8);
        tmpU32 |= (data[6] << 16);
        tmpU32 |= (unsigned long) (data[7] << 24);

        totalsize += 8 + tmpU32;

        /* Check ChunkId field */
        if (!memcmp(&data[0], "fmt ", 4))
        {
            if ((tmpU32 != 16) && (tmpU32 != 18) && (tmpU32 != 40))
            {
                APP_DEBUG1("WARNING: format chunk size is not supported (%lu != (16, 18, 40))", tmpU32);
                goto app_wav_open_file_error;
            }

            /* Read the FMT params */
            size = read(fd, fmt_params, tmpU32);
            if (size < 0)
            {
                APP_ERROR1("read(%s) failed: %d", p_fname, errno);
                goto app_wav_open_file_error;
            }
            if ((UINT32)size != tmpU32)
            {
                APP_ERROR1("Length read does not match FMT params (%d != %d)", (int)size, (int)tmpU32);
                goto app_wav_open_file_error;
            }

            /* Read AudioFormat field */
            tmpU16 = fmt_params[0];
            tmpU16 |= (fmt_params[1] << 8);

            /* Check AudioFormat field => 0x01 = PCM LINEAR, 0x25 = apt-X */
            if (tmpU16 == 0x01)
            {
                p_format->codec = BSA_AV_CODEC_PCM;
            }
            else if (tmpU16 == 0x25)
            {
                p_format->codec = BSA_AV_CODEC_APTX;
            }
            else
            {
                APP_ERROR1("WAV audio format is not supported (%u)", tmpU16);
                goto app_wav_open_file_error;
            }

            /* Read NumChannels field */
            tmpU16 = fmt_params[2];
            tmpU16 |= (fmt_params[3] << 8);

            /* Check NumChannels field => 1:Mono 2:Stereo */
            if (p_format->codec == BSA_AV_CODEC_APTX)
            {
                if (tmpU16 > 3)
                {
                    APP_ERROR1("number of channels not supported in apt-X (%u)", tmpU16);
                    goto app_wav_open_file_error;
                }
                if (tmpU16 > 1)
                {
                    p_format->stereo_mode = tmpU16;
                    tmpU16 = 2;
                }
            }
            else if (tmpU16 > 2)
            {
                APP_ERROR1("bad number of channels not supported (%u)", tmpU16);
                goto app_wav_open_file_error;
            }
            p_format->nb_channels = tmpU16;

            /* Read SampleRate field */
            tmpU32 = fmt_params[4];
            tmpU32 |= (fmt_params[5] << 8);
            tmpU32 |= (fmt_params[6] << 16);
            tmpU32 |= (unsigned long) (fmt_params[7] << 24);
            p_format->sample_rate = tmpU32;

            /* Read BitsPerSample field (read before ByteRate and BlockAlign) */
            tmpU16 = fmt_params[14];
            tmpU16 |= (fmt_params[15] << 8);
            p_format->bits_per_sample = tmpU16;

            /* Read ByteRate field */
            tmpU32 = fmt_params[8];
            tmpU32 |= (fmt_params[9] << 8);
            tmpU32 |= (fmt_params[10] << 16);
            tmpU32 |= (unsigned long)(fmt_params[11] << 24);
            /* Check ByteRate field: should be = SampleRate * NumChannels * BitsPerSample/8 */
            if (tmpU32 != (p_format->sample_rate * p_format->nb_channels * p_format->bits_per_sample / 8))
            {
                APP_DEBUG1("WARNING: byte rate does not match PCM rate calculation (%lu != (%lu * %u * %u / 8))",
                        tmpU32, p_format->sample_rate, p_format->nb_channels, p_format->bits_per_sample);
            }

            /* Read BlockAlign field */
            tmpU16 = fmt_params[12];
            tmpU16 |= (fmt_params[13] << 8);
            /* Check BlockAlign field: should be NumChannels * BitsPerSample/8 */
            if (tmpU16 != (p_format->nb_channels * p_format->bits_per_sample / 8))
            {
                APP_ERROR1("Block alignment does not match calculation (%u != (%u * %u / 8))",
                        tmpU16, p_format->nb_channels, p_format->bits_per_sample);
                goto app_wav_open_file_error;
            }
        }
        else if (!memcmp(&data[0], "fact", 4))
        {
            if (tmpU32 > 8)
            {
                APP_ERROR1("FACT chunk size is not supported (%d)", (int)tmpU32);
                goto app_wav_open_file_error;
            }

            /* Bypass */
            size = read(fd, data, tmpU32);
            if (size < 0)
            {
                APP_ERROR1("read(%s) failed: %d", p_fname, errno);
                goto app_wav_open_file_error;
            }
            if ((unsigned long)size != tmpU32)
            {
                APP_ERROR1("Length read does not match FACT chunk (%d != %d)", (int)size, sizeof(data));
                goto app_wav_open_file_error;
            }
        }
        else if (!memcmp(&data[0], "LIST", 4) ||
                  !memcmp(&data[0], "cue ", 4))
        {
            /* Bypass */
            if (lseek(fd, tmpU32, SEEK_CUR) == -1)
            {
                APP_ERROR1("Failed bypassing the LIST chunk (%u)", tmpU32);
                goto app_wav_open_file_error;
            }
            if (size != sizeof(data))
            {
                APP_ERROR1("Length read does not match LIST chunk (%d != %d)", (int)size, sizeof(data));
                goto app_wav_open_file_error;
            }
        }
        else if (!memcmp(&data[0], "data", 4))
        {
            /* data chunk specific: added byte not included in chunk size*/
            if (tmpU32 & 1)
            {
                totalsize++;
            }
            /* Check size matches file size */
            if (totalsize != file_size)
            {
                APP_DEBUG1("WARNING: RIFF size does not match file size (%lu != %lu))",
                        file_size, totalsize);
            }
        }
        else
        {
            APP_ERROR1("unsupported chunk (%c%c%c%c)", data[0], data[1], data[2], data[3]);
            goto app_wav_open_file_error;
        }
    } while (memcmp(&data[0], "data", 4));

    if (!p_format->sample_rate)
    {
        APP_ERROR0("Format chunk was not found in WAV file");
        goto app_wav_open_file_error;
    }

    return fd;

    app_wav_open_file_error:
    close(fd);

    return -1;
}

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
int app_wav_create_file(const char *p_fname, int flags)
{
    int fd, dummy;

    /* Create file in read/write mode, reset the length field */
    flags |= O_RDWR | O_CREAT | O_TRUNC;

    fd = open(p_fname, flags, 0666);
    if (fd < 0)
    {
        if (!(flags & O_EXCL))
        {
            APP_ERROR1("open(%s) failed: %d", p_fname, errno);
        }
    }
    else
    {
        APP_DEBUG1("created WAV file %s", p_fname);
        dummy = write(fd, app_wav_hdr, sizeof(app_wav_hdr));
        (void)dummy;
    }
    return fd;
}

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
void app_wav_close_file(int fd, const tAPP_WAV_FILE_FORMAT *p_format)
{
    int data_size, dummy;
    unsigned char header[APP_WAVE_HDR_SIZE];
    int chunk_size;
    int byte_rate;
    short block_align;

    if (fd < 0)
    {
        APP_ERROR0("Bad file descriptor");
        return;
    }

    /* Copy the standard header */
    memcpy(header, app_wav_hdr, sizeof(header));

    /* Retrieve the size of the file */
    data_size = lseek(fd, 0, SEEK_CUR);
    if (data_size < 0)
    {
        APP_ERROR0("Read current pointer failed");
        data_size = APP_WAVE_HDR_SIZE;
    }

    /* Remove the standard header from the size of the file */
    data_size -= APP_WAVE_HDR_SIZE;

    chunk_size = data_size + 36;
    if (p_format->bits_per_sample == 8)
    {
        byte_rate = p_format->nb_channels * sizeof(char) * p_format->sample_rate;
        block_align = p_format->nb_channels * sizeof(char);
    }
    else
    {
        byte_rate = p_format->nb_channels * sizeof(short) * p_format->sample_rate;
        block_align = p_format->nb_channels * sizeof(short);
    }

    header[4] = (unsigned char)chunk_size;
    header[5] = (unsigned char)(chunk_size >> 8);
    header[6] = (unsigned char)(chunk_size >> 16);
    header[7] = (unsigned char)(chunk_size >> 24);

    header[22] = (unsigned char)p_format->nb_channels;
    header[23] = 0; /* p_format->nb_channels is coded on 1 byte only */

    header[24] = (unsigned char)p_format->sample_rate;
    header[25] = (unsigned char)(p_format->sample_rate >> 8);
    header[26] = (unsigned char)(p_format->sample_rate >> 16);
    header[27] = (unsigned char)(p_format->sample_rate >> 24);

    header[28] = (unsigned char)byte_rate;
    header[29] = (unsigned char)(byte_rate >> 8);
    header[30] = (unsigned char)(byte_rate >> 16);
    header[31] = (unsigned char)(byte_rate >> 24);

    header[32] = (unsigned char)block_align;
    header[33] = (unsigned char)(block_align >> 8);

    header[34] = (unsigned char)p_format->bits_per_sample;
    header[35] = (unsigned char)(p_format->bits_per_sample >> 8);

    header[40] = (unsigned char)data_size;
    header[41] = (unsigned char)(data_size >> 8);
    header[42] = (unsigned char)(data_size >> 16);
    header[43] = (unsigned char)(data_size >> 24);

    lseek(fd, 0, SEEK_SET);
    dummy = write(fd, header, sizeof(header));
    (void)dummy;
    close(fd);
}





/*****************************************************************************
 **
 **  Name:           app_alsa.c
 **
 **  Description:    Linux ALSA utility functions (playback/capture)
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#include "app_utils.h"

#include <bsa_rokid/bsa_api.h>
#include "app_alsa.h"

/*
 * Definitions
 */

#ifndef BSA_ALSA_DEV_NAME
#define BSA_ALSA_DEV_NAME  "default"
#endif

static char *alsa_device = BSA_ALSA_DEV_NAME; /* ALSA default device */
#ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED
static char *alsa_device_loopback = "loop"; /* ALSA loopback device */
#endif

typedef struct
{
    snd_pcm_t *p_playback_handle;
    tAPP_ALSA_PLAYBACK_OPEN playback_param;
    snd_pcm_t *p_capture_handle;
    tAPP_ALSA_CAPTURE_OPEN capture_param;
} tAPP_ALSA_CB;

/*
 * Global variables
 */
tAPP_ALSA_CB app_alsa_cb;

/*
 * Local functions
 */
static snd_pcm_format_t app_alsa_get_pcm_format(tAPP_ALSA_PCM_FORMAT app_format);

/*******************************************************************************
 **
 ** Function        app_alsa_init
 **
 ** Description     ALSA initialization
 **
 ** Parameters      None
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_init(void)
{
    APP_DEBUG0("");
    memset (&app_alsa_cb, 0, sizeof(app_alsa_cb));
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_alsa_playback_open_init
 **
 ** Description     Init ALSA playback parameters (to Speaker)
 **
 ** Parameters      p_open: Open parameters
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_playback_open_init(tAPP_ALSA_PLAYBACK_OPEN *p_open)
{
    p_open->access = APP_ALSA_PCM_ACCESS_RW_INTERLEAVED;
    p_open->blocking = FALSE; /* Non Blocking */
    p_open->format = APP_ALSA_PCM_FORMAT_S16_LE; /* Signed 16 bits Little Endian*/
    p_open->sample_rate = 44100; /* 44.1KHz */
    p_open->stereo = TRUE; /* Stereo */
    p_open->latency = 200000; /* 200ms */

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_alsa_playback_open
 **
 ** Description     Open ALSA playback channel (to Speaker)
 **
 ** Parameters      p_open: Open parameters
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_playback_open(tAPP_ALSA_PLAYBACK_OPEN *p_open)
{
    int mode = 0; /* Default if Blocking */
    unsigned int nb_channels;
    int rv;
    snd_pcm_format_t format;
    snd_pcm_access_t access;

    APP_DEBUG0("Opening ALSA/Asound audio driver Playback");

    /* check if already opened */
    if (app_alsa_cb.p_playback_handle != NULL)
    {
        APP_ERROR0("Playback was already opened");
        return -1;
    }

    /* check PCM Format parameter */
    format = app_alsa_get_pcm_format(p_open->format);
    if (format == SND_PCM_FORMAT_UNKNOWN)
    {
        return -1;
    }

    /* Check PCM access parameter */
    if (p_open->access == APP_ALSA_PCM_ACCESS_RW_INTERLEAVED)
    {
        access = SND_PCM_ACCESS_RW_INTERLEAVED;
    }
    else
    {
        APP_ERROR1("Unsupported PCM access:%d", p_open->access);
        return -1;
    }
    /* Check Blocking parameter */
    if (p_open->blocking == FALSE)
    {
        mode = SND_PCM_NONBLOCK;
    }

    /* check Stereo parameter */
    if (p_open->stereo)
    {
        nb_channels = 2;
    }
    else
    {
        nb_channels = 1;
    }

    /* Save the Playback open parameters */
    memcpy(&app_alsa_cb.playback_param, p_open, sizeof(app_alsa_cb.playback_param));

    /* Open ALSA driver */
    rv = snd_pcm_open(&app_alsa_cb.p_playback_handle, alsa_device,
            SND_PCM_STREAM_PLAYBACK, mode);
    if (rv < 0)
    {
        APP_ERROR1("unable to open ALSA device in playback mode:%s", snd_strerror(rv));
        return rv;
    }

    /* Configure ALSA driver with PCM parameters */
    rv = snd_pcm_set_params(app_alsa_cb.p_playback_handle,
            format,
            access,
            nb_channels,
            p_open->sample_rate,
            1, /* SW resample */
            p_open->latency);
    if (rv < 0)
    {
        APP_ERROR1("Unable to config ALSA device:%s", snd_strerror(rv));
        snd_pcm_close(app_alsa_cb.p_playback_handle);
        app_alsa_cb.p_playback_handle = NULL;
        return rv;
    }

    APP_DEBUG0("ALSA driver opened in playback mode");

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_alsa_playback_close
 **
 ** Description     close ALSA playback channel (to Speaker)
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_playback_close(void)
{
    int status;

    if (app_alsa_cb.p_playback_handle == NULL)
    {
        APP_ERROR0("Playback channel was not opened");
        return -1;
    }
    APP_DEBUG0("ALSA driver: Closing Playback");

    status = snd_pcm_close(app_alsa_cb.p_playback_handle);
    if (status < 0)
    {
        APP_ERROR1("snd_pcm_close error status:%d", status);
    }
    else
    {
        app_alsa_cb.p_playback_handle = NULL;
    }
    return status;
}

/*******************************************************************************
 **
 ** Function        app_alsa_playback_write
 **
 ** Description     Write PCM sample to Playback Channel
 **
 ** Parameters      p_buffer: pointer on buffer containing
 **                 buffer_size: Buffer size
 **
 ** Returns         number of bytes written (-1 if error)
 **
 *******************************************************************************/
int app_alsa_playback_write(void *p_buffer, int buffer_size)
{
    snd_pcm_sframes_t alsa_frames;
    snd_pcm_sframes_t alsa_frames_expected;
    int bytes_per_frame;

    if (p_buffer == NULL)
    {
        return 0;
    }

    if (app_alsa_cb.p_playback_handle == NULL)
    {
        APP_ERROR0("Playback channel not opened");
        return -1;
    }

    /* Compute number of bytes per PCM samples fct(stereo, format) */
    if (app_alsa_cb.playback_param.stereo)
    {
        bytes_per_frame = 2;
    }
    else
    {
        bytes_per_frame = 1;
    }

    if ((app_alsa_cb.playback_param.format == APP_ALSA_PCM_FORMAT_S16_LE) ||
        (app_alsa_cb.playback_param.format == APP_ALSA_PCM_FORMAT_S16_BE))
    {
        bytes_per_frame *= 2;
    }
    /* Compute the number of frames */
    alsa_frames_expected = buffer_size / bytes_per_frame;

    alsa_frames = snd_pcm_writei(app_alsa_cb.p_playback_handle, p_buffer, alsa_frames_expected);
    if (alsa_frames < 0)
    {
        APP_DEBUG1("snd_pcm_recover %d", (int)alsa_frames);
        alsa_frames = snd_pcm_recover(app_alsa_cb.p_playback_handle, alsa_frames, 0);
    }
    if (alsa_frames < 0)
    {
        APP_ERROR1("snd_pcm_writei failed:%s", snd_strerror(alsa_frames));
    }
    if (alsa_frames > 0 && alsa_frames < alsa_frames_expected)
    {
        APP_DEBUG1("Short write (expected %d, wrote %d)",
                (int) alsa_frames_expected, (int)alsa_frames);
    }
    return alsa_frames * bytes_per_frame;
}

/*******************************************************************************
 **
 ** Function        app_alsa_capture_open_init
 **
 ** Description     Init ALSA Capture parameters (from microphone)
 **
 ** Parameters      p_open: Capture parameters
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_capture_open_init(tAPP_ALSA_CAPTURE_OPEN *p_open)
{
    memset(p_open, 0, sizeof(*p_open));

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_alsa_capture_open
 **
 ** Description     Open ALSA Capture channel (from microphone)
 **
 ** Parameters      p_open: Capture parameters
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_capture_open(tAPP_ALSA_CAPTURE_OPEN *p_open)
{
    int mode = 0; /* Default is blocking */
    unsigned int nb_channels;
    int rv;
    snd_pcm_format_t format;
    snd_pcm_access_t access;

    APP_DEBUG0("Opening ALSA/Asound audio driver Capture");

    /* Sanity check if already opened */
    if (app_alsa_cb.p_capture_handle != NULL)
    {
        APP_DEBUG0("Capture was already opened");
    }

    /* check PCM Format parameter */
    format = app_alsa_get_pcm_format(p_open->format);
    if (format == SND_PCM_FORMAT_UNKNOWN)
    {
        return -1;
    }

    /* Check PCM access parameter */
    if (p_open->access == APP_ALSA_PCM_ACCESS_RW_INTERLEAVED)
    {
        access = SND_PCM_ACCESS_RW_INTERLEAVED;
    }
    else
    {
        APP_ERROR1("Unsupported PCM access:%d", p_open->access);
        return -1;
    }
    /* Check Blocking parameter */
    if (p_open->blocking == FALSE)
    {
        mode = SND_PCM_NONBLOCK;
    }

    /* check Stereo parameter */
    if (p_open->stereo)
    {
        nb_channels = 2;
    }
    else
    {
        nb_channels = 1;
    }

    /* Save the Capture open parameters */
    memcpy(&app_alsa_cb.capture_param, p_open, sizeof(app_alsa_cb.capture_param));

    /* Open ALSA driver */
    rv = snd_pcm_open(&app_alsa_cb.p_capture_handle, alsa_device,
            SND_PCM_STREAM_CAPTURE, mode);
    if (rv < 0)
    {
        APP_ERROR1("unable to open ALSA device in Capture mode:%s", snd_strerror(rv));
        return rv;
    }
    APP_DEBUG0("ALSA driver opened in Capture mode");

    /* Configure ALSA driver with PCM parameters */
    rv = snd_pcm_set_params(app_alsa_cb.p_capture_handle,
            format,
            access,
            nb_channels,
            p_open->sample_rate,
            1, /* SW resample */
            p_open->latency);
    if (rv)
    {
        APP_ERROR1("Unable to config ALSA device:%s", snd_strerror(rv));
        snd_pcm_close(app_alsa_cb.p_capture_handle);
        app_alsa_cb.p_capture_handle = NULL;
        return rv;
    }
    return 0;
}

#ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED
/*******************************************************************************
 **
 ** Function        app_alsa_capture_loopback_open
 **
 ** Description     Open ALSA Capture channel (from loopback driver)
 **
 ** Parameters      p_open: Capture parameters
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_capture_loopback_open(tAPP_ALSA_CAPTURE_OPEN *p_open)
{
    int mode = 0; /* Default is blocking */
    unsigned int nb_channels;
    int rv;
    snd_pcm_format_t format;
    snd_pcm_access_t access;

    APP_DEBUG0("Opening ALSA/Asound audio driver Capture");

    /* Sanity check if already opened */
    if (app_alsa_cb.p_capture_handle != NULL)
    {
        APP_DEBUG0("Capture was already opened");
    }

    /* check PCM Format parameter */
    format = app_alsa_get_pcm_format(p_open->format);
    if (format == SND_PCM_FORMAT_UNKNOWN)
    {
        return -1;
    }

    /* Check PCM access parameter */
    if (p_open->access == APP_ALSA_PCM_ACCESS_RW_INTERLEAVED)
    {
        access = SND_PCM_ACCESS_RW_INTERLEAVED;
    }
    else
    {
        APP_ERROR1("Unsupported PCM access:%d", p_open->access);
        return -1;
    }
    /* Check Blocking parameter */
    if (p_open->blocking == FALSE)
    {
        mode = SND_PCM_NONBLOCK;
    }

    /* check Stereo parameter */
    if (p_open->stereo)
    {
        nb_channels = 2;
    }
    else
    {
        nb_channels = 1;
    }

    /* Save the Capture open parameters */
    memcpy(&app_alsa_cb.capture_param, p_open, sizeof(app_alsa_cb.capture_param));

    /* Open ALSA driver */
    rv = snd_pcm_open(&app_alsa_cb.p_capture_handle, alsa_device_loopback,
            SND_PCM_STREAM_CAPTURE, mode);
    if (rv < 0)
    {
        APP_ERROR1("unable to open ALSA loopback device in Capture mode:%s", snd_strerror(rv));
        return rv;
    }
    APP_DEBUG0("ALSA loopback driver opened in Capture mode");

    /* Configure ALSA driver with PCM parameters */
    rv = snd_pcm_set_params(app_alsa_cb.p_capture_handle,
            format,
            access,
            nb_channels,
            p_open->sample_rate,
            1, /* SW resample */
            p_open->latency);
    if (rv)
    {
        APP_ERROR1("Unable to config ALSA device:%s", snd_strerror(rv));
        snd_pcm_close(app_alsa_cb.p_capture_handle);
        app_alsa_cb.p_capture_handle = NULL;
        return rv;
    }
    return 0;
}
#endif /* #ifdef APP_AV_BCST_PLAY_LOOPDRV_INCLUDED */

/*******************************************************************************
 **
 ** Function        app_alsa_capture_close
 **
 ** Description     close ALSA Capture channel (from Microphone)
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
int app_alsa_capture_close(void)
{
    int status;

    if (app_alsa_cb.p_capture_handle == NULL)
    {
        APP_ERROR0("Capture channel was not opened");
        return -1;
    }
    APP_DEBUG0("ALSA driver: Closing Capture mode");

    status = snd_pcm_close(app_alsa_cb.p_capture_handle);
    if (status < 0)
    {
        APP_ERROR1("snd_pcm_close error status:%d", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function        app_alsa_capture_read
 **
 ** Description     Read PCM samples from Capture Channel
 **
 ** Parameters      p_buffer: pointer on buffer containing
 **                 buffer_size: size of the buffer
 **
 ** Returns         number of bytes read (< 0 if error)
 **
 *******************************************************************************/
int app_alsa_capture_read(void *p_buffer, int buffer_size)
{
    snd_pcm_sframes_t alsa_frames;
    snd_pcm_sframes_t alsa_frames_expected;
    int bytes_per_frame;

    if (p_buffer == NULL)
    {
        return 0;
    }

    if (app_alsa_cb.p_capture_handle == NULL)
    {
        APP_ERROR0("Capture channel not opened");
        return -1;
    }

    /* Compute number of bytes per PCM samples fct(stereo, format) */
    if (app_alsa_cb.capture_param.stereo)
    {
        bytes_per_frame = 2;
    }
    else
    {
        bytes_per_frame = 1;
    }

    if ((app_alsa_cb.capture_param.format == APP_ALSA_PCM_FORMAT_S16_LE) ||
        (app_alsa_cb.capture_param.format == APP_ALSA_PCM_FORMAT_S16_BE))
    {
        bytes_per_frame *= 2;
    }

    /* Divide by the number of channel and by */
    alsa_frames_expected = buffer_size / bytes_per_frame;

    alsa_frames = snd_pcm_readi(app_alsa_cb.p_capture_handle, p_buffer, alsa_frames_expected);
    if (alsa_frames < 0)
    {
        if (alsa_frames == -EAGAIN)
        {
            APP_ERROR1("snd_pcm_readi returns:%d (EAGAIN)", (int)alsa_frames);
            /* This is not really an error */
            alsa_frames = 0;
        }
        else
        {
            APP_ERROR1("snd_pcm_readi returns:%d", (int)alsa_frames);
        }
    }
    else if ((alsa_frames >= 0) &&
             (alsa_frames < alsa_frames_expected))
    {
        APP_DEBUG1("Short read (expected %i, read %i)",
            (int) alsa_frames_expected, (int)alsa_frames);
    }

    return alsa_frames * bytes_per_frame;
}

/*******************************************************************************
 **
 ** Function        app_alsa_get_pcm_format
 **
 ** Description     Check and Convert the PCM format asked by the application to
 **                 ALSA PCM format
 **
 ** Parameters      app_format: PCM format asked by the application
 **
 ** Returns         Alsa PCM format
 **
 *******************************************************************************/
static snd_pcm_format_t app_alsa_get_pcm_format(tAPP_ALSA_PCM_FORMAT app_format)
{
    snd_pcm_format_t format;

    switch (app_format)
    {
    case APP_ALSA_PCM_FORMAT_S16_LE:
        format = SND_PCM_FORMAT_S16_LE;
        break;
    case APP_ALSA_PCM_FORMAT_S16_BE:
        format = SND_PCM_FORMAT_S16_BE;
        break;
    default:
        format = SND_PCM_FORMAT_UNKNOWN;
        APP_ERROR1("Unsupported PCM format:%d", app_format);
        break;
    }
    return format;
}


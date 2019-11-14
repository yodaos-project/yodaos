/*****************************************************************************
 **
 **  Name:           app_alsa.h
 **
 **  Description:    Linux ALSA utility functions (playback/recorde)
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_ALSA_H
#define APP_ALSA_H

#define APP_ALSA_PCM_FORMAT_S16_LE  0   /* Signed 16 Bits Little Endian */
#define APP_ALSA_PCM_FORMAT_S16_BE  1   /* Signed 16 Bits Big Endian */
typedef UINT8 tAPP_ALSA_PCM_FORMAT;

#define APP_ALSA_PCM_ACCESS_RW_INTERLEAVED  0   /* RW Interleaved access */
typedef UINT8 tAPP_ALSA_PCM_ACCESS;

typedef struct
{
    BOOLEAN blocking;   /* Blocking mode */
    tAPP_ALSA_PCM_FORMAT format; /* PCM Format */
    tAPP_ALSA_PCM_ACCESS access; /* PCM access */
    BOOLEAN stereo; /* Nono or Stereo */
    UINT32 sample_rate; /* Sampling rate (e.g. 8000 for 8KHz) */
    UINT32 latency; /* Latency (ms) */
} tAPP_ALSA_PLAYBACK_OPEN;

typedef tAPP_ALSA_PLAYBACK_OPEN tAPP_ALSA_CAPTURE_OPEN;

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
int app_alsa_init(void);

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
int app_alsa_playback_open_init(tAPP_ALSA_PLAYBACK_OPEN *p_open);

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
int app_alsa_playback_open(tAPP_ALSA_PLAYBACK_OPEN *p_open);

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
int app_alsa_playback_close(void);

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
int app_alsa_playback_write(void *p_buffer, int buffer_size);

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
int app_alsa_capture_open_init(tAPP_ALSA_CAPTURE_OPEN *p_open);

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
int app_alsa_capture_open(tAPP_ALSA_CAPTURE_OPEN *p_open);

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
int app_alsa_capture_loopback_open(tAPP_ALSA_CAPTURE_OPEN *p_open);
#endif

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
int app_alsa_capture_close(void);

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
int app_alsa_capture_read(void *p_buffer, int buffer_size);

#endif

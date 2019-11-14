/*****************************************************************************
**
**  Name:           app_hs.c
**
**  Description:    Bluetooth Manager application
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "hfp_hs.h"
#include <bsa_rokid/gki.h>
#include <bsa_rokid/uipc.h>

#include "app_manager.h"
#include "app_utils.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"

#include "app_disc.h"

#include "app_dm.h"
#include <bsa_rokid/btm_api.h>
#include <bsa_rokid/bta_api.h>
#include <alsa/asoundlib.h>
#include <utils/Timers.h>
#include <cutils/properties.h>

#ifdef KAMINO_K18 //bt pcm hw connect codec directly not use software to play BT RX pcm
	#define PCM_DEVICE_BTPCM "hw:0,0"
#else
	#define USE_PULSEAUDIO
	#define PCM_DEVICE_BTPCM "hw:0,0"
#endif

#define HFP_DUMP_DATA  //dump /data/hs_data.pcm
//#define USE_MIC_TEST   // TODO !!not work OK

#ifdef USE_PULSEAUDIO
/*pulseauido*/
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/def.h>
#include <pulse/sample.h>
#else
#include <tinyalsa/asoundlib.h>
#define PLAY_DEV 1
#endif

#ifndef BSA_SCO_ROUTE_DEFAULT
#define BSA_SCO_ROUTE_DEFAULT BSA_SCO_ROUTE_HCI//BSA_SCO_ROUTE_HCI BSA_SCO_ROUTE_PCM
#endif


#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_PCM)
    #define APP_HS_SAMPLE_RATE      16000
#else
    #define APP_HS_SAMPLE_RATE      8000      /* AG Voice sample rate is always 8KHz */
#endif
#define APP_HS_BITS_PER_SAMPLE  16        /* AG Voice sample size is 16 bits */
#define APP_HS_CHANNEL_NB       1         /* AG Voice sample in mono */

#define APP_HS_FEATURES  ( BSA_HS_FEAT_ECNR | BSA_HS_FEAT_3WAY | BSA_HS_FEAT_CLIP | \
                           BSA_HS_FEAT_VREC | BSA_HS_FEAT_RVOL | BSA_HS_FEAT_ECS | \
                           BSA_HS_FEAT_ECC | BSA_HS_FEAT_CODEC | BSA_HS_FEAT_UNAT )

#define APP_HS_MIC_VOL  14
#define APP_HS_SPK_VOL  14

#define APP_HS_HSP_SERVICE_NAME "BSA Headset"
#define APP_HS_HFP_SERVICE_NAME "BSA Handsfree"

#define APP_HS_MAX_AUDIO_BUF 240

/*
 * Types
 */

/* control block (not needed to be stored in NVRAM) */
typedef struct
{
    tBSA_HS_CONN_CB    conn_cb[BSA_HS_MAX_NUM_CONN];
    //BOOLEAN            registered;
    //BOOLEAN            is_muted;
    //BOOLEAN            mute_inband_ring;
    //UINT32             ring_handle;
    //UINT32             cw_handle;
    //UINT32             call_op_handle;
    UINT8              sco_route;
    //int                data_size;
#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_HCI)
    short              audio_buf[APP_HS_MAX_AUDIO_BUF];
#endif
    BOOLEAN            open_pending;
    //BD_ADDR            open_pending_bda;
} tAPP_HS_CB;

struct HFP_HS_t
{
#ifdef HFP_DUMP_DATA
    FILE *fp;
#endif
    void *caller;
    //int mic_vol;
    //int spk_vol;

    int is_enabled;

    //BD_ADDR bd_addr;
    //BD_ADDR opened_addr;
#ifdef USE_PULSEAUDIO
    pa_simple *pas; //paplay
#endif
    int is_playing;
    int is_capture;
    pthread_mutex_t mutex;
    hfp_callbacks_t listener;
    void *listener_handle;
};
/*
 * Globales Variables
 */

static HFP_HS *_HS;
static tAPP_HS_CB  app_hs_cb;

const char *app_hs_service_ind_name[] =
{
    "NO SERVICE",
    "SERVICE AVAILABLE"
};

const char *app_hs_call_ind_name[] =
{
    "NO CALL",
    "ACTIVE CALL"
};

const char *app_hs_callsetup_ind_name[] =
{
    "CALLSETUP DONE",
    "INCOMING CALL",
    "OUTGOING CALL",
    "ALERTING REMOTE"
};

const char *app_hs_callheld_ind_name[] =
{
    "NONE ON-HOLD",
    "ACTIVE+HOLD",
    "ALL ON-HOLD"
};

const char *app_hs_roam_ind_name[] =
{
    "HOME NETWORK",
    "ROAMING"
};


/*
* Local Function
*/
#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_PCM)
static pthread_t sco_rx_thread;
static pthread_t sco_tx_thread;
static int sco_enabled = 0;

//static char pcm_device_mic[]	= "hw:0,3";
//static char pcm_device_btpcm[]	= "hw:0,0";
//static char pcm_device_spk[]		= "hw:0,1";
static void *sco_rx_cb(void *arg);
static void *sco_tx_cb(void *arg);

int set_sco_enable(HFP_HS *hs, int enable)
{
    BT_LOGD("%s sco stream", enable == 0 ? "Disable" : "Enable");
    if (sco_enabled == enable) {
        BT_LOGD("sco stream %s already", enable == 0 ? "disabled" : "enabled");
        return 0;
    }

    sco_enabled = enable;
    if (enable) {
        pthread_attr_t     attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&sco_rx_thread, &attr, sco_rx_cb, hs)) {
            BT_LOGE("rx thread create failed: %s", strerror(errno));
            sco_enabled = 0;
            return -1;
        }

        if (pthread_create(&sco_tx_thread, &attr, sco_tx_cb, hs)) {
            BT_LOGE("tx thread create failed: %s", strerror(errno));
            sco_enabled = 0;
            return -1;
        }
        pthread_attr_destroy(&attr);
    } else {
        if (sco_rx_thread) {
            sco_rx_thread  = 0;
        }
        if (sco_tx_thread) {
            sco_tx_thread  = 0;
        }
    }

    return 0;
}

static void *sco_rx_cb(void *arg)
{
#ifdef KAMINO_K18
	BT_LOGW("sco_rx_cb cpu do nothing, bt receive data and send to codec\n");
	return NULL;
#else
	HFP_HS *hs = arg;
	snd_pcm_t *pcm_handle_capture = NULL;
	snd_pcm_t *pcm_handle_playback = NULL;
	snd_pcm_hw_params_t *snd_params = NULL;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	const int channels = APP_HS_CHANNEL_NB, expected_frames = 240;
	int dir, status, buf_size;
	unsigned int rate = APP_HS_SAMPLE_RATE;
	char *buf = NULL;
	snd_pcm_uframes_t frames = 120;

	BT_LOGD("setting bt pcm");
	/*********BT PCM ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_capture, PCM_DEVICE_BTPCM,
						  SND_PCM_STREAM_CAPTURE,	/*bt pcm as input device*/
						  0);		 				/*SYNC MODE*/

	if (status < 0) {
		BT_LOGE("bt pcm open fail:%s", strerror(errno));
		return NULL;
	}
	snd_pcm_hw_params_alloca(&snd_params);
	snd_pcm_hw_params_any(pcm_handle_capture, snd_params);
	snd_pcm_hw_params_set_access(pcm_handle_capture, snd_params,
								 SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle_capture, snd_params, format);
	snd_pcm_hw_params_set_channels(pcm_handle_capture, snd_params, channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle_capture, snd_params, &rate, 0);
	snd_pcm_hw_params_set_period_size_near(pcm_handle_capture, snd_params, &frames, &dir);

	status = snd_pcm_hw_params(pcm_handle_capture, snd_params);

	if (status < 0) {
		BT_LOGE("bt pcm set params fail:%s", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		return NULL;
	}

	BT_LOGD("setting speaker");
#ifdef USE_PULSEAUDIO
#else
	/*********SPEAKER ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_playback, pcm_device_spk,
						  SND_PCM_STREAM_PLAYBACK,	/*speaker as output device*/
						  0);						/* SYNC MODE */
	if (status < 0) {
		BT_LOGE("bt pcm open fail:%s", strerror(errno));
		return NULL;
	}

	status = snd_pcm_set_params(pcm_handle_playback, format,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								APP_HS_CHANNEL_NB,
								APP_HS_SAMPLE_RATE,
								1,
								100000);
	if (status < 0) {
		BT_LOGE("bt pcm set params fail:%s", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		snd_pcm_close(pcm_handle_playback);
		return NULL;
	}
#endif
	BT_LOGD("start streaming");
	/*********STREAM HANDLING BEGIN***************************/
	buf_size = expected_frames * APP_HS_CHANNEL_NB * 2;	/*bytes = frames * ch * 16Bit/8 */
	buf = malloc(buf_size);
	frames = 0;
	while (sco_enabled) {
		memset(buf, 0, buf_size);
		frames = snd_pcm_readi(pcm_handle_capture, buf, expected_frames);
		if (frames == -EPIPE) {
			BT_LOGE("pcm read underrun");
			snd_pcm_prepare(pcm_handle_capture);
			frames = 0;
			continue;
		} else if (frames == -EBADFD) {
			BT_LOGE("pcm read EBADFD, retring");
			frames = 0;
			continue;
		}
#ifdef USE_PULSEAUDIO
    int error;
    pthread_mutex_lock(&hs->mutex);
    if (hs->pas != NULL && hs->is_playing && frames > 0) {
        if((pa_simple_write(hs->pas, buf, frames * APP_HS_CHANNEL_NB * 2 , &error)) < 0) {
            BT_LOGE("pa_simple_write failed %s\n", pa_strerror(error));
        }
    }
    else {
        BT_LOGD("playback stream NULL, play %d\n", hs->is_playing);
    }
    pthread_mutex_unlock(&hs->mutex);
#else
    frames = snd_pcm_writei(pcm_handle_playback, buf, frames);
    /*if write failed somehow, just ignore, we don't want to wast too much time*/
    if (frames == -EPIPE) {
        BT_LOGE("speaker write underrun");
        snd_pcm_prepare(pcm_handle_playback);
    } else if (frames == -EBADFD) {
        BT_LOGE("speaker write  EBADFD");
    }
#endif
	}
	BT_LOGI("stopping");
	/********STOP**********************************************/
	if (pcm_handle_capture)
		snd_pcm_close(pcm_handle_capture);
	if (pcm_handle_playback)
		snd_pcm_close(pcm_handle_playback);
	if (buf)
		free(buf);

	return NULL;
#endif
}

static void *sco_tx_cb(void *arg)
{
	HFP_HS *hs = arg;
	//snd_pcm_t *pcm_handle_capture = NULL;
	snd_pcm_t *pcm_handle_playback = NULL;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	const int mic_channels = APP_HS_CHANNEL_NB, expected_frames = 240;
	int status, buf_size;
	char *buf = NULL;
	snd_pcm_uframes_t frames = 120;
	BT_HFP_MSG msg = {0};

	BT_LOGD("setting bt pcm");
	/*********BT PCM ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_playback, PCM_DEVICE_BTPCM,
						  SND_PCM_STREAM_PLAYBACK,	/*bt pcm as output device*/
						  0);						/*SYNC MODE*/
	if (status < 0) {
		BT_LOGE("bt pcm open fail:%s", strerror(errno));
		goto exit;
	}

	status = snd_pcm_set_params(pcm_handle_playback, format,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								APP_HS_CHANNEL_NB,
								APP_HS_SAMPLE_RATE,
								1,				/*allow alsa resample*/
								100000);		/*expected max latence = 100ms*/

	if (status < 0) {
		BT_LOGE("bt pcm set params fail:%s", strerror(errno));
		goto exit;
	}

	BT_LOGD("start streaming");
	/*********STREAM HANDLING BEGIN***************************/
	buf_size  = expected_frames * mic_channels * 2;	/*bytes = frames * ch * 16Bit/8 , about 15ms*/
	buf  = malloc(buf_size);

	//bak_time = systemTime(CLOCK_MONOTONIC) /1000;
	while (sco_enabled && hs->is_capture) {
            memset(buf, 0, buf_size);

            if (hs->listener) {
                msg.feed_mic.data = buf;
                msg.feed_mic.need_len = buf_size;
                msg.feed_mic.get_len = 0;
                msg.feed_mic.bits = APP_HS_BITS_PER_SAMPLE;
                msg.feed_mic.channel= mic_channels;
                msg.feed_mic.rate= APP_HS_SAMPLE_RATE;
                hs->listener(hs->listener_handle, BT_HS_FEED_MIC_EVT, &msg);
                if (msg.feed_mic.get_len <= 0) {
                    BT_LOGD("feed_mic len error: %d", msg.feed_mic.get_len);
                    usleep(15*1000);
                    continue;
                }
                frames = msg.feed_mic.get_len / 2 / mic_channels;
            } else {
                BT_LOGE("not set hs listener!!!");
                goto exit;
            }
        #ifdef HFP_DUMP_DATA
        {
            char value[PROP_VALUE_MAX] = {0};
            property_get("rokid.dump.bt", value, "0");
            if (hs->fp && (atoi(value) > 0))
                fwrite(buf, buf_size, 1, hs->fp);
        }
        #endif

            frames = snd_pcm_writei(pcm_handle_playback, buf, frames);
            /*if write failed somehow, just ignore, we don't want to wast too much time*/
            if (frames == -EPIPE) {
                BT_LOGE("bt pcm write underrun");
                snd_pcm_prepare(pcm_handle_playback);
            } else if (frames == -EBADFD) {
                BT_LOGE("bt pcm write  EBADFD");
            }
        }
exit:
	BT_LOGI("stopping");
	/********STOP**********************************************/
	//if (pcm_handle_capture)
	//	snd_pcm_close(pcm_handle_capture);
	if (pcm_handle_playback)
		snd_pcm_close(pcm_handle_playback);
	if (buf)
		free(buf);

	return NULL;
}
#endif

static int app_hs_open_pcm(HFP_HS *hs)
{
    if (!hs) return -1;
#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_PCM)
    set_sco_enable(hs, 1);
#endif
    hs->is_playing = 1;
    hs->is_capture = 1;
#ifdef HFP_DUMP_DATA
    if (!hs->fp)
        hs->fp = fopen("/data/hs_data.pcm", "wb");
#endif

    return 0;
}

static int app_hs_close_pcm(HFP_HS *hs)
{
    if (!hs) return -1;

    hs->is_playing = 0;
    hs->is_capture = 0;
#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_PCM)
    set_sco_enable(hs, 0);
#endif

#ifdef HFP_DUMP_DATA
    if (hs->fp) {
        fclose(hs->fp);
        hs->fp = NULL;
    }
#endif
    return 0;
}

/*******************************************************************************
**
** Function         app_hs_get_default_conn
**
** Description      Find the first active connection control block
**
** Returns          Pointer to the found connection, NULL if not found
*******************************************************************************/
tBSA_HS_CONN_CB *app_hs_get_default_conn()
{
    UINT16 cb_index;

    //APPL_TRACE_EVENT0("app_hs_get_default_conn");

    for(cb_index = 0; cb_index < BSA_HS_MAX_NUM_CONN; cb_index++)
    {
        if(app_hs_cb.conn_cb[cb_index].connection_active)
            return &app_hs_cb.conn_cb[cb_index];
    }
    return NULL;
}

/*******************************************************************************
**
** Function         app_hs_get_conn_by_handle
**
** Description      Find a connection control block by its handle
**
** Returns          Pointer to the found connection, NULL if not found
*******************************************************************************/
static tBSA_HS_CONN_CB *app_hs_get_conn_by_handle(UINT16 handle)
{
    //APPL_TRACE_EVENT1("app_hs_get_conn_by_handle: %d", handle);

    /* check that the handle does not go beyond limits */
    if (handle <= BSA_HS_MAX_NUM_CONN)
    {
        return &app_hs_cb.conn_cb[handle-1];
    }

    return NULL;
}

/*******************************************************************************
**
** Function         app_hs_find_indicator_id
**
** Description      parses the indicator string and finds the position of a field
**
** Returns          index in the string
*******************************************************************************/
static UINT8 app_hs_find_indicator_id(char * ind, char * field)
{
    UINT16 string_len = strlen(ind);
    UINT8 i, id = 0;
    BOOLEAN skip = FALSE;

    for(i=0; i< string_len ; i++)
    {
        if(ind[i] == '"')
        {
            if(!skip)
            {
                id++;
                if(!strncmp(&ind[i+1], field, strlen(field)) && (ind[i+1+strlen(field)] == '"'))
                {

                    return id;
                }
                else
                {
                    /* skip the next " */
                    skip = TRUE;
                }
            }
            else
            {
                skip = FALSE;
            }
        }
    }
    return 0;
}

/*******************************************************************************
**
** Function         app_hs_decode_indicator_string
**
** Description      process the indicator string and sets the indicator ids
**
** Returns          void
*******************************************************************************/
static void app_hs_decode_indicator_string(tBSA_HS_CONN_CB *p_conn, char * ind)
{
    p_conn->call_ind_id = app_hs_find_indicator_id(ind, "call");
    p_conn->call_setup_ind_id = app_hs_find_indicator_id(ind, "callsetup");
    if(!p_conn->call_setup_ind_id)
    {
        p_conn->call_setup_ind_id = app_hs_find_indicator_id(ind, "call_setup");
    }
    p_conn->service_ind_id = app_hs_find_indicator_id(ind, "service");
    p_conn->battery_ind_id = app_hs_find_indicator_id(ind, "battchg");
    p_conn->callheld_ind_id = app_hs_find_indicator_id(ind, "callheld");
    p_conn->signal_strength_ind_id = app_hs_find_indicator_id(ind, "signal");
    p_conn->roam_ind_id = app_hs_find_indicator_id(ind, "roam");
}

/*******************************************************************************
**
** Function         app_hs_set_indicator_status
**
** Description      sets the current indicator
**
** Returns          void
*******************************************************************************/
static void app_hs_set_initial_indicator_status(tBSA_HS_CONN_CB *p_conn, char * ind)
{
    UINT8 i, pos;

    /* Clear all indicators. Not all indicators will be initialized */
    p_conn->curr_call_ind = 0;
    p_conn->curr_call_setup_ind = 0;
    p_conn->curr_service_ind = 0;
    p_conn->curr_callheld_ind = 0;
    p_conn->curr_signal_strength_ind = 0;
    p_conn->curr_roam_ind = 0;
    p_conn->curr_battery_ind = 0;

    /* skip any spaces in the front */
    while ( *ind == ' ' ) ind++;

    /* get "call" indicator*/
    pos = p_conn->call_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_call_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "callsetup" indicator*/
    pos = p_conn->call_setup_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_call_setup_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "service" indicator*/
    pos = p_conn->service_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_service_ind = ind[i] - '0';
            /* if there is no service play the designated tone */
            if(!p_conn->curr_service_ind)
            {
                /*
                if(HS_CFG_BEEP_NO_NETWORK)
                {
                    UTL_BeepPlay(HS_CFG_BEEP_NO_NETWORK);
                }*/
            }
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "callheld" indicator*/
    pos = p_conn->callheld_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_callheld_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "signal" indicator*/
    pos = p_conn->signal_strength_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_signal_strength_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "roam" indicator*/
    pos = p_conn->roam_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_roam_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    /* get "battchg" indicator*/
    pos = p_conn->battery_ind_id -1;
    for(i=0; i< strlen(ind) ; i++)
    {
        if(!pos)
        {
            p_conn->curr_battery_ind = ind[i] - '0';
            break;
        }
        else if(ind[i] == ',')
            pos--;
    }

    if(p_conn->curr_callheld_ind != 0)
    {
        BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_3WAY_HELD);
    }
    else if(p_conn->curr_call_ind == BSA_HS_CALL_ACTIVE)
    {
        if(p_conn->curr_call_setup_ind == BSA_HS_CALLSETUP_INCOMING)
        {
            BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_WAITCALL);
        }
        else
        {
            BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_CALLACTIVE);
        }
    }
    else if(p_conn->curr_call_setup_ind == BSA_HS_CALLSETUP_INCOMING)
    {
        BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_RINGACT);
    }
    else if((p_conn->curr_call_setup_ind == BSA_HS_CALLSETUP_OUTGOING) ||
            (p_conn->curr_call_setup_ind == BSA_HS_CALLSETUP_ALERTING)
           )
    {
        BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_OUTGOINGCALL);
    }

    /* Dump indicators */
    if(p_conn->curr_service_ind < 2)
    BT_LOGD("Service: %s,%d", app_hs_service_ind_name[p_conn->curr_service_ind],p_conn->curr_service_ind);

    if(p_conn->curr_call_ind < 2)
    BT_LOGD("Call: %s,%d", app_hs_call_ind_name[p_conn->curr_call_ind],p_conn->curr_call_ind);

    if(p_conn->curr_call_setup_ind < 4)
    BT_LOGD("Callsetup: Ind %s,%d", app_hs_callsetup_ind_name[p_conn->curr_call_setup_ind],p_conn->curr_call_setup_ind);

    if(p_conn->curr_callheld_ind < 3)
    BT_LOGD("Hold: %s,%d", app_hs_callheld_ind_name[p_conn->curr_callheld_ind],p_conn->curr_callheld_ind);

    if(p_conn->curr_roam_ind < 2)
    BT_LOGD("Roam: %s,%d", app_hs_roam_ind_name[p_conn->curr_roam_ind],p_conn->curr_roam_ind);
}

#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_HCI)
/*******************************************************************************
 **
 ** Function         app_hs_sco_uipc_cback
 **
 ** Description     uipc audio call back function.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void app_hs_sco_uipc_cback(BT_HDR *p_buf)
{
    UINT8 *pp = (UINT8 *)(p_buf + 1);
    UINT8 pkt_len;
    HFP_HS *hs = _HS;

    if (p_buf == NULL) {
        return;
    }
    if (!hs || (p_buf->len == 0)) goto exit;

    pkt_len = p_buf->len;

#ifdef  USE_PULSEAUDIO
    int error;
    //BT_LOGV("len %d", pkt_len);
    pthread_mutex_lock(&hs->mutex);
    if (hs->pas != NULL && hs->is_playing) {
        if((pa_simple_write(hs->pas, pp, pkt_len, &error)) < 0) {
            BT_LOGE("pa_simple_write failed %s\n", pa_strerror(error));
        }
    }
    else {
        BT_LOGE("playback stream NULL, play %d\n", hs->is_playing);
    }
    pthread_mutex_unlock(&hs->mutex);
#endif

    /* for now we just handle one instance */
    /* for multiple instance user should be prompted */
    tBSA_HS_CONN_CB * p_conn = &(app_hs_cb.conn_cb[0]);
    BT_HFP_MSG msg = {0};
    if (hs->listener &&  hs->is_capture) {
        msg.feed_mic.data = (char *)app_hs_cb.audio_buf;
        msg.feed_mic.need_len = pkt_len;
        msg.feed_mic.get_len = 0;
        msg.feed_mic.bits = APP_HS_BITS_PER_SAMPLE;
        msg.feed_mic.channel= APP_HS_CHANNEL_NB;
        msg.feed_mic.rate= APP_HS_SAMPLE_RATE;
        hs->listener(hs->listener_handle, BT_HS_FEED_MIC_EVT, &msg);
        if (msg.feed_mic.get_len <= 0) {
            BT_LOGD("feed_mic len error: %d", msg.feed_mic.get_len);
        } else {
            if (TRUE != UIPC_Send(p_conn->uipc_channel, 0, (UINT8 *) app_hs_cb.audio_buf, msg.feed_mic.get_len)) {
                BT_LOGE("UIPC_Send failed");
            }
        }
    }
exit:
    GKI_freebuf(p_buf);
}
#endif

/* public function */

/*******************************************************************************
**
** Function         app_hs_open
**
** Description      Establishes mono headset connections
**
** Parameter        BD address to connect to. If its NULL, the app will prompt user for device.
**
** Returns          0 if success -1 if failure
*******************************************************************************/


int rokid_hs_open(HFP_HS *hs, BD_ADDR bd_addr)
{
    tBSA_STATUS status = 0;
    tBSA_HS_OPEN param;

    if (!hs || !bd_addr) return -1;
    BSA_HsOpenInit(&param);

    bdcpy(param.bd_addr, bd_addr);
    /* we manage only one connection for now */
    param.hndl = app_hs_cb.conn_cb[0].handle;
    status = BSA_HsOpen(&param);
    app_hs_cb.open_pending = TRUE;

    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_HsOpen failed (%d)", status);
        app_hs_cb.open_pending = FALSE;
    }
    return status;
}

/*******************************************************************************
**
** Function         app_hs_audio_open
**
** Description      Open the SCO connection alone
**
** Parameter        None
**
** Returns          0 if success -1 if failure
*******************************************************************************/

int app_hs_audio_open(HFP_HS *hs)
{
    BT_LOGI("app_hs_audio_open\n");

    tBSA_STATUS status;
    tBSA_HS_AUDIO_OPEN audio_open;
    tBSA_HS_CONN_CB * p_conn = &(app_hs_cb.conn_cb[0]);

    if(app_hs_cb.sco_route == BSA_SCO_ROUTE_HCI &&
        p_conn->uipc_channel == UIPC_CH_ID_BAD)
    {
        BT_LOGE("Bad UIPC channel in app_hs_audio_open");
        return -1;
    }

    BSA_HsAudioOpenInit(&audio_open);
    audio_open.sco_route = app_hs_cb.sco_route;
    audio_open.hndl = app_hs_cb.conn_cb[0].handle;
    status = BSA_HsAudioOpen(&audio_open);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("failed with status : %d", status);
        return -1;
    }
    return status;
}

/*******************************************************************************
**
** Function         app_hs_audio_close
**
** Description      Close the SCO connection alone
**
** Parameter        None
**
** Returns          0 if success -1 if failure
*******************************************************************************/

int app_hs_audio_close(HFP_HS *hs)
{
    BT_LOGI("app_hs_audio_close\n");
    tBSA_STATUS status;
    tBSA_HS_AUDIO_CLOSE audio_close;
    tBSA_HS_CONN_CB *p_conn;

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL)
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsAudioCloseInit(&audio_close);
    audio_close.hndl = app_hs_cb.conn_cb[0].handle;
    status = BSA_HsAudioClose(&audio_close);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("failed with status : %d", status);
        return -1;
    }
    return status;
}

/*******************************************************************************
**
** Function         app_hs_close
**
** Description      release mono headset connections
**
** Returns          0 if success -1 if failure
*******************************************************************************/
int app_hs_close(HFP_HS *hs)
{
    tBSA_HS_CLOSE param;
    tBSA_STATUS status;
    tBSA_HS_CONN_CB *p_conn;

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL) {
        BT_LOGW("### %s no connection available\n", __func__);
        return 0;
    }
    /* Prepare parameters */
    BSA_HsCloseInit(&param);

    /* todo : promt user to check what to close here */
    /* for now we just handle one AG at a time */
    param.hndl = app_hs_cb.conn_cb[0].handle;

    status = BSA_HsClose(&param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("failed with status : %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         app_hs_cancel
**
** Description      cancel connections
**
** Returns          0 if success -1 if failure
*******************************************************************************/
int app_hs_cancel(HFP_HS *hs)
{
    tBSA_HS_CANCEL param;
    tBSA_STATUS status;

    /* Prepare parameters */
    BSA_HsCancelInit(&param);

    status = BSA_HsCancel(&param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("failed with status : %d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
**
** Function         app_is_open_pending
**
**
** Returns          TRUE if open if pending
*******************************************************************************/
BOOLEAN app_hs_is_open_pending(HFP_HS *hs, BD_ADDR bda_open_pending)
{
    return app_hs_cb.open_pending;
}

/*******************************************************************************
**
** Function         app_hs_answer_call
**
** Description      example of function to answer the call
**
** Returns          0 if success -1 if failure
*******************************************************************************/
int app_hs_answer_call(HFP_HS *hs)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn;

    BT_LOGI("app_hs_answer_call\n");

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL)
    {
        BT_LOGW("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_A_CMD;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_hangup
**
** Description      example of function to hang up
**
** Returns          0 if success -1 if failure
*******************************************************************************/
int app_hs_hangup(HFP_HS *hs)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn;

    BT_LOGI("app_hs_hangup\n");

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL)
    {
        BT_LOGW("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_CHUP_CMD;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_stop
**
** Description      This function is used to stop hs services and close all
**                  UIPC channel.
**
** Parameters       void
**
** Returns          void
**
*******************************************************************************/
int app_hs_stop(HFP_HS *hs)
{
    tBSA_HS_DISABLE      disable_param;
    tBSA_HS_DEREGISTER   deregister_param;
    tBSA_HS_CONN_CB * p_conn;
    UINT8 index;

    BT_CHECK_HANDLE(hs);
    if (!hs->is_enabled)
    {
        BT_LOGW("### no enabled yet\n");
        return 0;
    }
    app_hs_close_pcm(hs);
#ifdef USE_PULSEAUDIO
    pthread_mutex_lock(&hs->mutex);
    if (hs->pas) {
        pa_simple_free(hs->pas);
        hs->pas = NULL;
    }
    pthread_mutex_unlock(&hs->mutex);
#endif

    /* If connection exist, shold close first */
    if ((p_conn = app_hs_get_default_conn()) != NULL)
    {
        int times = 10;
        app_hs_close(hs);
        while (times --&& app_hs_cb.conn_cb[0].connection_active) {
            usleep(200*1000);
        }
    }

    BSA_HsDeregisterInit(&deregister_param);
    for (index=0; index < BSA_HS_MAX_NUM_CONN; index++) {
        p_conn = &(app_hs_cb.conn_cb[index]);
        deregister_param.hndl = p_conn->handle;
        BT_LOGD("handle %d", deregister_param.hndl);
        BSA_HsDeregister(&deregister_param);

        if(p_conn->uipc_channel != UIPC_CH_ID_BAD)
        {
            if(p_conn->uipc_connected)
            {
                BT_LOGD("Closing UIPC Channel");
                UIPC_Close(p_conn->uipc_channel);
                p_conn->uipc_connected = FALSE;
            }
            p_conn->uipc_channel = UIPC_CH_ID_BAD;
        }
    }

    BSA_HsDisableInit(&disable_param);
    BSA_HsDisable(&disable_param);
    hs->is_enabled = 0;
    return 0;
}



static BT_HFP_CIEV_STATUS app_hs_find_decodec_ciev_id(tBSA_HS_CONN_CB *p_conn, char * ciev)
{
    UINT16 string_len = strlen(ciev);
    int i;
    UINT8 buf1[2] = {0};
    UINT8 buf2[2] = {0};
    int id = 0, val = 0;
    BT_HFP_CIEV_STATUS status = BT_HS_STATUS_UNKNOWN;

    for(i=1; i< string_len -1 ; i++) {
        if(ciev[i] == ',') {
            buf1[0] = ciev[i -1];
            buf2[0] = ciev[i + 1];
            id = atoi((const char *)buf1);
            val = atoi((const char *)buf2);
            BT_LOGI("%s id:%d val:%d\n", __func__, id, val);
            if (id == p_conn->call_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_CALL_OFF;
                        break;
                    case 1:
                        status = BT_HS_CALL_ON;
                        break;
                    default:
                        break;
                }
            } else if (id == p_conn->call_setup_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_CALLSETUP_DONE;
                        break;
                    case 1:
                        status = BT_HS_CALLSETUP_INCOMING;
                        break;
                    case 2:
                        status = BT_HS_CALLSETUP_OUTGOING;
                        break;
                    case 3:
                        status = BT_HS_CALLSETUP_REMOTE_ALERTED;
                        break;
                    default:
                        break;
                }
            } else if (id == p_conn->service_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_SERVICE_OFF;
                        break;
                    case 1:
                        status = BT_HS_SERVICE_ON;
                        break;
                    default:
                        break;
                }
            } else if (id == p_conn->battery_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_BATTCHG0;
                        break;
                    case 1:
                        status = BT_HS_BATTCHG1;
                        break;
                    case 2:
                        status = BT_HS_BATTCHG2;
                        break;
                    case 3:
                        status = BT_HS_BATTCHG3;
                        break;
                    case 4:
                        status = BT_HS_BATTCHG4;
                        break;
                    case 5:
                        status = BT_HS_BATTCHG5;
                        break;
                    default:
                        break;
                }
            } else if (id == p_conn->callheld_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_CALLHELD_NONE;
                        break;
                    case 1:
                        status = BT_HS_CALLHELD_HOLD_WITH_ACTIVE;
                        break;
                    case 2:
                        status = BT_HS_CALLHELD_HOLD;
                        break;
                    default:
                        break;
                }
            } else if (id == p_conn->signal_strength_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_SIGNAL0;
                        break;
                    case 1:
                        status = BT_HS_SIGNAL1;
                        break;
                    case 2:
                        status = BT_HS_SIGNAL2;
                        break;
                    case 3:
                        status = BT_HS_SIGNAL3;
                        break;
                    case 4:
                        status = BT_HS_SIGNAL4;
                        break;
                    case 5:
                        status = BT_HS_SIGNAL5;
                        break;
                    default:
                        break;
                }
            } else if (id == p_conn->roam_ind_id) {
                switch (val) {
                    case 0:
                        status = BT_HS_ROAM_OFF;
                        break;
                    case 1:
                        status = BT_HS_ROAM_ON;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    BT_LOGD("CIEV %d\n", status);
    return status;
}

/*******************************************************************************
**
** Function         app_hs_cback
**
** Description      Example of HS callback function
**
** Parameters       event code and data
**
** Returns          void
**
*******************************************************************************/
void app_hs_cback(tBSA_HS_EVT event, tBSA_HS_MSG *p_data)
{
    char buf[100];
    UINT16 handle = 0;
    tBSA_HS_CONN_CB *p_conn;
    HFP_HS *hs = _HS;
    BT_HFP_MSG msg = {0};

    if (!p_data || !hs) {
        BT_LOGI("app_hs_cback p_data=NULL for event:%d\n", event);
        return;
    }

    /* retrieve the handle of the connection for which this event */
    handle = p_data->hdr.handle;
    BT_LOGD("app_hs_cback event:%d for handle: %d", event, handle);

    /* retrieve the connection for this handle */
    p_conn = app_hs_get_conn_by_handle(handle);

    if (!p_conn)
    {
        BT_LOGE("app_hs_cback: handle %d not supported\n", handle);
        return;
    }

    BT_LOGD("\n event %d", event);
    switch (event)
    {
    case BSA_HS_CONN_EVT:       /* Service level connection */
        BT_LOGI("BSA_HS_CONN_EVT:\n");
        BD_ADDR bd_addr = {0};
        app_hs_cb.open_pending = FALSE;
        if (rokidbt_a2dp_sink_get_open_pending_addr(hs->caller, bd_addr) == 1) {
            if (A2DP_SINK_OPEN_PENDING_LINK_UP && (bdcmp(bd_addr, p_data->conn.bd_addr) != 0)) {
                tBSA_HS_CLOSE param;
                tBSA_STATUS status;

                BT_LOGW("a2dk sink already link up yet , we not want hfp/a2dp connect different devices!!");
                BSA_HsCloseInit(&param);

                param.hndl = p_data->conn.handle;

                status = BSA_HsClose(&param);
                if (status != BSA_SUCCESS)
                {
                    BT_LOGE("failed with status : %d", status);
                }
                break;
            }
        }
        BT_LOGI("    - Remote bdaddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                p_data->conn.bd_addr[0], p_data->conn.bd_addr[1],
                p_data->conn.bd_addr[2], p_data->conn.bd_addr[3],
                p_data->conn.bd_addr[4], p_data->conn.bd_addr[5]);
        BT_LOGI("    - Service: ");
        switch (p_data->conn.service)
        {
            case BSA_HSP_HS_SERVICE_ID:
                BT_LOGI("Headset\n");
                break;
            case BSA_HFP_HS_SERVICE_ID:
                BT_LOGI("Handsfree\n");
                break;
            default:
                BT_LOGI("Not supported 0x%08x\n", p_data->conn.service);
                return;
                break;
        }

        /* check if this conneciton is already opened */
        if (p_conn->connection_active)
        {
            BT_LOGI("BSA_HS_CONN_EVT: connection already opened for handle %d\n", handle);
            break;
        }
        BTDevice dev = {0};

        bdcpy(dev.bd_addr, p_data->conn.bd_addr);
        if (rokidbt_find_scaned_device(hs->caller, &dev) == 1) {
            snprintf(msg.conn.name, sizeof(msg.conn.name), "%s", dev.name);
        } else if (rokidbt_find_bonded_device(hs->caller, &dev) == 1) {
            snprintf(msg.conn.name, sizeof(msg.conn.name), "%s", dev.name);
        }
        bdcpy(p_conn->connected_bd_addr, p_data->conn.bd_addr);
        p_conn->handle = p_data->conn.handle;
        p_conn->connection_active = TRUE;
        p_conn->connected_hs_service_id = p_data->conn.service;
        p_conn->peer_feature = p_data->conn.peer_features;
        p_conn->status = BSA_HS_ST_CONNECT;
        if (hs->listener) {
            bdcpy(msg.conn.bd_addr, p_data->conn.bd_addr);
            msg.conn.idx = 0;
            msg.conn.status = 0;
            msg.conn.service = p_data->conn.service;
            msg.conn.peer_features = p_data->conn.peer_features;
            hs->listener(hs->listener_handle, BT_HS_CONN_EVT, &msg);
        }

        break;


    case BSA_HS_CLOSE_EVT:      /* Connection Closed (for info)*/
        /* Close event, reason BSA_HS_CLOSE_CLOSED or BSA_HS_CLOSE_CONN_LOSS */
        BT_LOGD("BSA_HS_CLOSE_EVT, reason %d", p_data->hdr.status);
        app_hs_cb.open_pending = FALSE;

        if (!p_conn->connection_active)
        {
            BT_LOGI("BSA_HS_CLOSE_EVT: connection not opened for handle %d\n", handle);
            break;
        }
        p_conn->connection_active = FALSE;
        p_conn->indicator_string_received = FALSE;


        BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_CONNECTABLE);

        if (hs->listener) {
            msg.close.idx = 0;
            msg.close.status = p_data->hdr.status;
            hs->listener(hs->listener_handle, BT_HS_CLOSE_EVT, &msg);
        }
        break;

    case BSA_HS_AUDIO_OPEN_EVT:     /* Audio Open Event */
        BT_LOGD("BSA_HS_AUDIO_OPEN_EVT\n");

#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_HCI)
        if(app_hs_cb.sco_route == BSA_SCO_ROUTE_HCI &&
           p_conn->uipc_channel != UIPC_CH_ID_BAD &&
           !p_conn->uipc_connected) {
            /* Open UIPC channel for TX channel ID */
            if(UIPC_Open(p_conn->uipc_channel, app_hs_sco_uipc_cback)!= TRUE)
            {
                BT_LOGE("app_hs_register failed to open UIPC channel(%d)",
                        p_conn->uipc_channel);
                break;
            }
            p_conn->uipc_connected = TRUE;
            UIPC_Ioctl(p_conn->uipc_channel, UIPC_REG_CBACK, app_hs_sco_uipc_cback);
        }
#endif
        if(BSA_HS_GETSTATUS(p_conn, BSA_HS_ST_SCOOPEN))
            app_hs_close_pcm(hs);
        app_hs_open_pcm(hs);


        p_conn->call_state = BSA_HS_CALL_CONN;
        BSA_HS_SETSTATUS(p_conn, BSA_HS_ST_SCOOPEN);
        if (hs->listener) {
            hs->listener(hs->listener_handle, BT_HS_AUDIO_OPEN_EVT, NULL);
        }
        break;

    case BSA_HS_AUDIO_CLOSE_EVT:         /* Audio Close event */
        BT_LOGD("BSA_HS_AUDIO_CLOSE_EVT\n");
        app_hs_close_pcm(hs);

        if (hs->listener) {
            hs->listener(hs->listener_handle, BT_HS_AUDIO_CLOSE_EVT, NULL);
        }
        if (!p_conn->connection_active)
        {
            BT_LOGI("BSA_HS_AUDIO_CLOSE_EVT: connection not opened for handle %d\n", handle);
            return;
        }
        p_conn->call_state = BSA_HS_CALL_NONE;
        BSA_HS_RESETSTATUS(p_conn, BSA_HS_ST_SCOOPEN);

        if(p_conn->uipc_channel != UIPC_CH_ID_BAD &&
           p_conn->uipc_connected) {
            BT_LOGD("Closing UIPC Channel");
            UIPC_Close(p_conn->uipc_channel);
            p_conn->uipc_connected = FALSE;
        }
        break;

    case BSA_HS_CIEV_EVT:                /* CIEV event */
        BT_LOGI("BSA_HS_CIEV_EVT\n");
        strncpy(buf, p_data->val.str, 4);
        buf[5] ='\0';
        BT_LOGI("Call Ind Status %s\n",buf);
        if (hs->listener) {
            msg.ciev.status= app_hs_find_decodec_ciev_id(p_conn, buf);
            hs->listener(hs->listener_handle, BT_HS_CIEV_EVT, &msg);
        }
        break;

    case BSA_HS_CIND_EVT:                /* CIND event */
        BT_LOGI("BSA_HS_CIND_EVT\n");
        BT_LOGI("Call Indicator %s\n",p_data->val.str);

        /* check if indicator configuration was received */
        if(p_conn->indicator_string_received)
        {
            app_hs_set_initial_indicator_status(p_conn, p_data->val.str);
            msg.cind.status.call_ind_id = p_conn->call_ind_id;
            msg.cind.status.call_setup_ind_id = p_conn->call_setup_ind_id;
            msg.cind.status.service_ind_id = p_conn->service_ind_id;
            msg.cind.status.battery_ind_id = p_conn->battery_ind_id;
            msg.cind.status.callheld_ind_id = p_conn->callheld_ind_id;
            msg.cind.status.signal_strength_ind_id = p_conn->signal_strength_ind_id;
            msg.cind.status.roam_ind_id = p_conn->roam_ind_id;

            msg.cind.status.curr_call_ind = p_conn->curr_call_ind;
            msg.cind.status.curr_call_setup_ind = p_conn->curr_call_setup_ind;
            msg.cind.status.curr_service_ind = p_conn->curr_service_ind;
            msg.cind.status.curr_battery_ind = p_conn->curr_battery_ind;
            msg.cind.status.curr_callheld_ind = p_conn->curr_callheld_ind;
            msg.cind.status.curr_signal_strength_ind = p_conn->curr_signal_strength_ind;
            msg.cind.status.curr_roam_ind = p_conn->curr_roam_ind;

            BT_LOGI("call_ind_id %d, call_setup_ind_id %d,service_ind_id %d, battery_ind_id %d, callheld_ind_id %d, signal_strength_ind_id %d, roam_ind_id %d\n",
            msg.cind.status.call_ind_id, msg.cind.status.call_setup_ind_id, msg.cind.status.service_ind_id, msg.cind.status.battery_ind_id,
            msg.cind.status.callheld_ind_id, msg.cind.status.signal_strength_ind_id, msg.cind.status.roam_ind_id);
            BT_LOGI("curr_call_ind %d, curr_call_setup_ind %d,curr_service_ind %d, curr_battery_ind %d, curr_callheld_ind %d, curr_signal_strength_ind %d, curr_roam_ind %d\n",
            msg.cind.status.curr_call_ind, msg.cind.status.curr_call_setup_ind, msg.cind.status.curr_service_ind, msg.cind.status.curr_battery_ind,
            msg.cind.status.curr_callheld_ind, msg.cind.status.curr_signal_strength_ind, msg.cind.status.curr_roam_ind);
            if (hs->listener) {
                hs->listener(hs->listener_handle, BT_HS_CIND_EVT, &msg);
            }
        }
        else
        {
            p_conn->indicator_string_received = TRUE;
            app_hs_decode_indicator_string(p_conn, p_data->val.str);
        }
        break;

    case BSA_HS_RING_EVT:
        BT_LOGD( "BSA_HS_RING_EVT\n");
        if (hs->listener) {
            hs->listener(hs->listener_handle, BT_HS_RING_EVT, NULL);
        }
        break;

    case BSA_HS_CLIP_EVT: {
        char incoming_num[255] ={0};
        int i = 0;
        int j = 0;
        int len = 0;
        BT_LOGI("BSA_HS_CLIP_EVT:%s\n",p_data->val.str);
        while (p_data->val.str[i] != '\0' && len < BSA_HS_AT_MAX_LEN) {
            len++;
            if (p_data->val.str[i++] == '"') {
                break;
            }
        }
        while (p_data->val.str[i] != '\0' && len < BSA_HS_AT_MAX_LEN) {
            len++;
            if (p_data->val.str[i] == '"') {
                break;
            }
            incoming_num[j++] = p_data->val.str[i++];
        }
        incoming_num[j] = '\0';
        BT_LOGI("BSA_HS_CLIP_EVT:incoming num:%s\n", incoming_num);
     }
        break;

    case BSA_HS_BSIR_EVT:
        BT_LOGD( "BSA_HS_BSIR_EVT\n");
        break;

    case BSA_HS_BVRA_EVT:
        BT_LOGD( "BSA_HS_BVRA_EVT\n");
        break;

    case BSA_HS_CCWA_EVT:
        BT_LOGD( "Call waiting : BSA_HS_CCWA_EVT:%s\n", p_data->val.str);
        break;

    case BSA_HS_CHLD_EVT:
        BT_LOGD( "BSA_HS_CHLD_EVT\n");
        if (hs->listener) {
            hs->listener(hs->listener_handle, BT_HS_CHLD_EVT, NULL);
        }
        break;

    case BSA_HS_VGM_EVT:
        BT_LOGD( "BSA_HS_VGM_EVT\n");
        break;

    case BSA_HS_VGS_EVT:
        BT_LOGD( "BSA_HS_VGS_EVT\n");
        break;

    case BSA_HS_BINP_EVT:
        BT_LOGD( "BSA_HS_BINP_EVT\n");
        break;

    case BSA_HS_BTRH_EVT:
        BT_LOGD( "BSA_HS_BTRH_EVT\n");
        break;

    case BSA_HS_CNUM_EVT:
        BT_LOGD( "BSA_HS_CNUM_EVT:%s\n",p_data->val.str);
        break;

    case BSA_HS_COPS_EVT:
        BT_LOGD( "BSA_HS_COPS_EVT:%s\n",p_data->val.str);
        break;

    case BSA_HS_CMEE_EVT:
        BT_LOGD( "BSA_HS_CMEE_EVT:%s\n", p_data->val.str);
        break;

    case BSA_HS_CLCC_EVT:
        BT_LOGD( "BSA_HS_CLCC_EVT:%s\n", p_data->val.str);
        break;

    case BSA_HS_UNAT_EVT:
        BT_LOGD( "BSA_HS_UNAT_EVT\n");
        break;

    case BSA_HS_OK_EVT:
        BT_LOGD( "BSA_HS_OK_EVT: command value %d, %s\n",p_data->val.num, p_data->val.str);
        if (hs->listener) {
            hs->listener(hs->listener_handle, BT_HS_OK_EVT, NULL);
        }
        break;

    case BSA_HS_ERROR_EVT:
        BT_LOGD( "BSA_HS_ERROR_EVT\n");
        if (hs->listener) {
            hs->listener(hs->listener_handle, BT_HS_ERROR_EVT, NULL);
        }
        break;

    case BSA_HS_BCS_EVT:
        BT_LOGD( "BSA_HS_BCS_EVT: codec %d (%s)\n",p_data->val.num,
            (p_data->val.num == BSA_SCO_CODEC_MSBC) ? "mSBC":"CVSD");
        break;

    case BSA_HS_OPEN_EVT:
        BT_LOGD( "BSA_HS_OPEN_EVT\n");
        app_hs_cb.open_pending = FALSE;
        break;

    default:
        BT_LOGI("app_hs_cback unknown event:%d\n", event);
        break;
    }
}

HFP_HS *hs_create(void *caller)
{
    HFP_HS *hs = calloc(1, sizeof(*hs));

    hs->caller = caller;
    pthread_mutex_init(&hs->mutex, NULL);
    _HS = hs;
    return hs;
}

int hs_destroy(HFP_HS *hs)
{
    if (hs) {
        //app_hs_stop(hs);
        pthread_mutex_destroy(&hs->mutex);
        free(hs);
    }
    _HS = NULL;
    return 0;
}

/*******************************************************************************
**
** Function         app_hs_register
**
** Description      Example of function to start register one Hs instance
**
** Parameters       void
**
** Returns          0 if ok -1 in case of error
**
*******************************************************************************/
int app_hs_register(HFP_HS *hs)
{
    int index, status = 0;
    tBSA_HS_REGISTER   param;
    tBSA_HS_CONN_CB * p_conn;

    BT_LOGD("start Register");

    app_hs_cb.sco_route = BSA_SCO_ROUTE_DEFAULT;

    /* prepare parameters */
    BSA_HsRegisterInit(&param);
    param.services = BSA_HSP_HS_SERVICE_MASK | BSA_HFP_HS_SERVICE_MASK;
    param.sec_mask = BSA_SEC_NONE;
    /* Use BSA_HS_FEAT_CODEC (for WBS) for SCO over PCM only, not for SCO over HCI*/
    param.features = APP_HS_FEATURES;
    if(app_hs_cb.sco_route == BSA_SCO_ROUTE_HCI)
        param.features &= ~BSA_HS_FEAT_CODEC;
    param.settings.ecnr_enabled = (param.features & BSA_HS_FEAT_ECNR) ? TRUE : FALSE;
    param.settings.mic_vol = APP_HS_MIC_VOL;
    param.settings.spk_vol = APP_HS_SPK_VOL;
    strncpy(param.service_name[0], APP_HS_HSP_SERVICE_NAME, BSA_HS_SERVICE_NAME_LEN_MAX);
    strncpy(param.service_name[1], APP_HS_HFP_SERVICE_NAME, BSA_HS_SERVICE_NAME_LEN_MAX);

    /* SCO routing options:  BSA_SCO_ROUTE_HCI or BSA_SCO_ROUTE_PCM */
    param.sco_route = app_hs_cb.sco_route;

    for(index=0; index<BSA_HS_MAX_NUM_CONN; index++)
    {
        status = BSA_HsRegister(&param);
        if (status != BSA_SUCCESS)
        {
            BT_LOGE("Unable to register HS with status %d", status);
            return -1;
        }

        p_conn = &(app_hs_cb.conn_cb[index]);
        p_conn->handle = param.hndl;
        p_conn->uipc_channel = param.uipc_channel;

        BT_LOGD("Register complete handle %d, status %d, uipc [%d->%d], sco_route %d",
                param.hndl, status, p_conn->uipc_channel, param.uipc_channel, param.sco_route);
    }

    BT_LOGD("Register complete");
    return 0;
}

int app_hs_set_listener(HFP_HS *hs, hfp_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(hs);
    hs->listener = listener;
    hs->listener_handle = listener_handle;
    return 0;
}

int app_hs_enable(HFP_HS *hs)
{
    UINT8 index;
    tBSA_STATUS status;
    tBSA_HS_ENABLE     enable_param;
    tBSA_HS_DISABLE      disable_param;
    tBSA_HS_DEREGISTER   deregister_param;
    int latency = 200; // 200 ms

    BT_CHECK_HANDLE(hs);
    if (hs->is_enabled) {
        BT_LOGW("#### app_hs_enable is already enabled\n");
        return 0;
    }
    memset(&app_hs_cb, 0, sizeof(app_hs_cb));

    for(index=0; index<BSA_HS_MAX_NUM_CONN; index++)
    {
        app_hs_cb.conn_cb[index].uipc_channel = UIPC_CH_ID_BAD;
    }

    /* prepare parameters */
    BSA_HsEnableInit(&enable_param);
    enable_param.p_cback = app_hs_cback;

    status = BSA_HsEnable(&enable_param);
    if (status != BSA_SUCCESS) {
        BT_LOGE("BSA_HsEnable failes with status %d", status);
        goto enable_failed;
    }

    status = app_hs_register(hs);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("app_hs_register failes with status %d", status);
        goto register_failed;
    }
#ifdef USE_PULSEAUDIO
    int error;
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = APP_HS_SAMPLE_RATE,
        .channels = APP_HS_CHANNEL_NB,
    };
    pa_buffer_attr attr = {0};

    attr.maxlength = (uint32_t)-1;
    attr.tlength = (uint32_t)-1;
    attr.minreq = (uint32_t) -1;
    attr.prebuf = APP_HS_SAMPLE_RATE  * APP_HS_CHANNEL_NB * 2 * latency /1000;
    attr.fragsize = attr.tlength;
    /* Create a new playback stream */
    pthread_mutex_lock(&hs->mutex);
    if (!hs->pas) {
        if (!(hs->pas = pa_simple_new(NULL, "BT-HS", PA_STREAM_PLAYBACK, NULL,
                            "playback", &ss, NULL, &attr, &error))) {
            BT_LOGI("pa_simple_new() failed: %s\n", pa_strerror(error));
            pthread_mutex_unlock(&hs->mutex);
            status = -1;
            goto create_pas_failed;
        }
    }
    pthread_mutex_unlock(&hs->mutex);
#endif

    hs->is_enabled = 1;

    return 0;

#ifdef USE_PULSEAUDIO
create_pas_failed:
    BSA_HsDeregisterInit(&deregister_param);
    for (index=0; index<BSA_HS_MAX_NUM_CONN; index++) {
        tBSA_HS_CONN_CB * p_conn;
        p_conn = &(app_hs_cb.conn_cb[index]);
        deregister_param.hndl = p_conn->handle;
        BT_LOGD("handle %d", deregister_param.hndl);
        BSA_HsDeregister(&deregister_param);

        if(p_conn->uipc_channel != UIPC_CH_ID_BAD)
        {
            if(p_conn->uipc_connected)
            {
                BT_LOGD("Closing UIPC Channel");
                UIPC_Close(p_conn->uipc_channel);
                p_conn->uipc_connected = FALSE;
            }
            p_conn->uipc_channel = UIPC_CH_ID_BAD;
        }
    }
#endif
register_failed:
    BSA_HsDisableInit(&disable_param);
    BSA_HsDisable(&disable_param);
enable_failed:
    return status;
}
/*******************************************************************************
**
** Function         app_hs_hold_call
**
** Description      Hold active call
**
** Parameters       void
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_hold_call(HFP_HS *hs, tBSA_BTHF_CHLD_TYPE_T type)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn;

    BT_LOGI("app_hs_hold_call\n");

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL)
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_CHLD_CMD;
    cmd_param.data.num = type;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_last_num_dial
**
** Description      Re-dial last dialed number
**
** Parameters       void
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_last_num_dial(HFP_HS *hs)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn;

    BT_LOGI("entry");

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL)
    {
        BT_LOGW("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_BLDN_CMD;

    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_dial_num
**
** Description      Dial a phone number
**
** Parameters       Phone number string
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_dial_num(HFP_HS *hs, const char *num)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn;

    BT_LOGI("num %s\n", num);

    if((num == NULL) || (strlen(num) == 0))
    {
        BT_LOGW("empty number string");
        return -1;
    }

    /* If no connection exist, error */
    if ((p_conn = app_hs_get_default_conn()) == NULL)
    {
        BT_LOGE("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_D_CMD;

    strcpy(cmd_param.data.str, num);
    strcat(cmd_param.data.str, ";");
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_send_unat
**
** Description      Send an unknown AT Command
**
** Parameters       char *cCmd
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_unat(HFP_HS *hs, char *cCmd)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("Command : %s\n", cCmd);

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())) || NULL==cCmd)
    {
        BT_LOGW("no connection available or invalid AT Command");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_UNAT_CMD;

    strncpy(cmd_param.data.str, cCmd, sizeof(cmd_param.data.str)-1);

    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_send_clcc
**
** Description      Send CLCC AT Command for obtaining list of current calls
**
** Parameters       None
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_clcc_cmd(HFP_HS *hs)
{
    BT_LOGI("app_hs_send_clcc_cmd\n");
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn = NULL;

    /* If no connection exist, error */
    if (NULL==(p_conn = app_hs_get_default_conn()))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_CLCC_CMD;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_send_cops_cmd
**
** Description      Send COPS AT Command to obtain network details and set network details
**
** Parameters       char *cCmd
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_cops_cmd(HFP_HS *hs, char *cCmd)
{

    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("app_hs_send_cops_cmd\n");
    /* If no connection exist, error */
    if (NULL==(p_conn = app_hs_get_default_conn()))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    strncpy(cmd_param.data.str, cCmd, sizeof(cmd_param.data.str)-1);
    cmd_param.command = BSA_HS_COPS_CMD;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_send_ind_cmd
**
** Description      Send indicator (CIND) AT Command
**
** Parameters       None
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_ind_cmd(HFP_HS *hs)
{
    BT_LOGI("app_hs_send_cind_cmd\n");
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    /* If no connection exist, error */
    if (NULL==(p_conn = app_hs_get_default_conn()))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_CIND_CMD;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_mute_unmute_microphone
**
** Description      Send Mute / unmute commands
**
** Returns          Returns SUCCESS or FAILURE on sending mute/unmute command
*******************************************************************************/
int app_hs_mute_unmute_microphone(HFP_HS *hs, BOOLEAN bMute)
{
    char str[20];
    memset(str,'\0',sizeof(str));
    strcpy(str, "+CMUT=");

    if(bMute)
        strcat(str, "1");
    else
        strcat(str, "0");
    return app_hs_send_unat(hs, str);
}

/*******************************************************************************
**
** Function         app_hs_send_dtmf
**
** Description      Send DTMF AT Command
**
** Parameters       char dtmf (0-9, #, A-D)
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_dtmf(HFP_HS *hs, char dtmf)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("Command : %x\n", dtmf);

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())) || '\0'==dtmf)
    {
        BT_LOGW("no connection available or invalid DTMF Command");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_VTS_CMD;

    cmd_param.data.str[0] = dtmf;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_send_cnum
**
** Description      Send CNUM AT Command
**
** Parameters       No parameter needed
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_cnum(HFP_HS *hs)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("app_hs_send_cnum:Command \n");

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())))
    {
        BT_LOGD("no connection available or invalid DTMF Command");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_CNUM_CMD;
    cmd_param.data.num=0;

    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_send_keypress_evt
**
** Description      Send Keypress event
**
** Parameters       char *cCmd - Keyboard sequence (0-9, * , #)
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_send_keypress_evt(HFP_HS *hs, char *cCmd)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("app_hs_send_keypress_evt:Command \n");

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;
    cmd_param.command = BSA_HS_CKPD_CMD;
    strncpy(cmd_param.data.str, cCmd, sizeof(cmd_param.data.str)-1);

    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_start_voice_recognition
**
** Description      Start voice recognition
**
** Parameters       None
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_start_voice_recognition(HFP_HS *hs)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("app_hs_start_voice_recognition:Command \n");

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    if(app_hs_cb.conn_cb[0].peer_feature & BSA_HS_PEER_FEAT_VREC)
    {
        BSA_HsCommandInit(&cmd_param);
        cmd_param.hndl = p_conn->handle;
        cmd_param.command = BSA_HS_BVRA_CMD;
        cmd_param.data.num = 1;

        BSA_HsCommand(&cmd_param);
        return 0;
    }

    BT_LOGD("Peer feature - VR feature not available");
    return -1;
}


/*******************************************************************************
**
** Function         app_hs_stop_voice_recognition
**
** Description      Stop voice recognition
**
** Parameters       None
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_stop_voice_recognition(HFP_HS *hs)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("app_hs_stop_voice_recognition:Command \n");

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    if(app_hs_cb.conn_cb[0].peer_feature & BSA_HS_PEER_FEAT_VREC)
    {
        BSA_HsCommandInit(&cmd_param);
        cmd_param.hndl = p_conn->handle;
        cmd_param.command = BSA_HS_BVRA_CMD;
        cmd_param.data.num = 0;
        BSA_HsCommand(&cmd_param);
        return 0;
   }

   BT_LOGD("Peer feature - VR feature not available");
   return -1;
}

/*******************************************************************************
**
** Function         app_hs_set_volume
**
** Description      Send volume AT Command
**
** Parameters       tBSA_BTHF_VOLUME_TYPE_T type, int volume
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_set_volume(HFP_HS *hs, tBSA_BTHF_VOLUME_TYPE_T type, int volume)
{
    tBSA_HS_COMMAND cmd_param;
    tBSA_HS_CONN_CB *p_conn=NULL;

    BT_LOGI("type : %d, volume:%d\n", type, volume);

    /* If no connection exist, error */
    if ((NULL==(p_conn = app_hs_get_default_conn())))
    {
        BT_LOGD("no connection available");
        return -1;
    }

    BSA_HsCommandInit(&cmd_param);
    cmd_param.hndl = p_conn->handle;

    if(BTHF_VOLUME_TYPE_SPK == type)
        cmd_param.command = BSA_HS_SPK_CMD;
    else
        cmd_param.command = BSA_HS_MIC_CMD;

    cmd_param.data.num = volume;
    return BSA_HsCommand(&cmd_param);
}

/*******************************************************************************
**
** Function         app_hs_getallIndicatorValues
**
** Description      Get all indicator values
**
** Parameters       tBSA_HS_IND_VALS *pIndVals
**
** Returns          0 if successful execution, error code else
**
*******************************************************************************/
int app_hs_getallIndicatorValues(HFP_HS *hs, tBSA_HS_IND_VALS *pIndVals)
{
    tBSA_HS_CONN_CB * p_conn = &(app_hs_cb.conn_cb[0]);
    if(NULL!=p_conn && NULL!=pIndVals)
    {
       pIndVals->curr_callwait_ind = 0;
       pIndVals->curr_signal_strength_ind = p_conn->curr_signal_strength_ind;
       pIndVals->curr_battery_ind = p_conn->curr_battery_ind;
       pIndVals->curr_call_ind = p_conn->curr_call_ind;
       pIndVals->curr_call_setup_ind = p_conn->curr_call_setup_ind;
       pIndVals->curr_roam_ind = p_conn->curr_roam_ind;
       pIndVals->curr_service_ind = p_conn->curr_service_ind;
       pIndVals->curr_callheld_ind = p_conn->curr_callheld_ind;
       if(pIndVals->curr_call_ind == BSA_HS_CALL_ACTIVE &&
          pIndVals->curr_call_setup_ind == BSA_HS_CALLSETUP_INCOMING)
       {
           pIndVals->curr_callwait_ind = 1;
       }
       return 0;
    }
    return -1;
}

int get_hs_enabled(HFP_HS *hs)
{
    return hs ? hs->is_enabled : 0;
}

int get_hs_capture(HFP_HS *hs)
{
    return hs ? hs->is_capture : 0;
}

int get_hs_playing(HFP_HS *hs)
{
    return hs ? hs->is_playing : 0;
}


int app_hs_get_type(void)
{
#if (BSA_SCO_ROUTE_DEFAULT == BSA_SCO_ROUTE_PCM)
    return BT_HS_USE_PCM;
#else
    return BT_HS_USE_UART;
#endif
}
/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2012, Luka Perkov
 * Copyright (C) 2012, John Crispin
 * Copyright (C) 2012, Andrej Vlašić
 * Copyright (C) 2012, Kaspar Schleiser for T-Labs
 *                     (Deutsche Telekom Innovation Laboratories)
 * Copyright (C) 2012, Mirko Vogt for T-Labs
 *                     (Deutsche Telekom Innovation Laboratories)
 * Copyright (c) 2015, Antonio Eugenio Burriel
 * Copyright (C) 2017, Stefan Koch
 *
 * Luka Perkov <openwrt@lukaperkov.net>
 * John Crispin <blogic@openwrt.org>
 * Andrej Vlašić <andrej.vlasic0@gmail.com>
 * Kaspar Schleiser <kaspar@schleiser.de>
 * Mirko Vogt <mirko@openwrt.org>
 * Antonio Eugenio Burriel <aeburriel@gmail.com>
 * Stefan Koch <stefan.koch10@gmail.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief Asterisk channel line driver for Lantiq based TAPI boards
 *
 * \author Luka Perkov <openwrt@lukaperkov.net>
 * \author John Crispin <blogic@openwrt.org>
 * \author Andrej Vlašić <andrej.vlasic0@gmail.com>
 * \author Kaspar Schleiser <kaspar@schleiser.de>
 * \author Mirko Vogt <mirko@openwrt.org>
 * \author Antonio Eugenio Burriel <aeburriel@gmail.com>
 * \author Stefan Koch <stefan.koch10@gmail.com>
 *
 * \ingroup channel_drivers
 */

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: xxx $")

#include <ctype.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#ifdef HAVE_LINUX_COMPILER_H
#include <linux/compiler.h>
#endif
#include <linux/telephony.h>

#include <asterisk/lock.h>
#include <asterisk/channel.h>
#include <asterisk/config.h>
#include <asterisk/module.h>
#include <asterisk/pbx.h>
#include <asterisk/utils.h>
#include <asterisk/callerid.h>
#include <asterisk/causes.h>
#include <asterisk/indications.h>
#include <asterisk/stringfields.h>
#include <asterisk/musiconhold.h>
#include <asterisk/sched.h>
#include <asterisk/cli.h>
#include <asterisk/devicestate.h>
#include <asterisk/format_cache.h>

/* Lantiq TAPI includes */
#include <drv_tapi/drv_tapi_io.h>
#include <drv_vmmc/vmmc_io.h>

#define TAPI_AUDIO_PORT_NUM_MAX                 2

/* Tapi predefined tones 0 to 31 */
#define TAPI_TONE_LOCALE_NONE			0
//#define TAPI_TONE_LOCALE_DIAL_CODE		25
//#define TAPI_TONE_LOCALE_RINGING_CODE		26
//#define TAPI_TONE_LOCALE_BUSY_CODE		27
//#define TAPI_TONE_LOCALE_CONGESTION_CODE	27

/* Tapi custom tones 32 to 256 */
#define TAPI_TONE_LOCALE_DIAL_CODE		32
#define TAPI_TONE_LOCALE_RINGING_CODE		33
#define TAPI_TONE_LOCALE_BUSY_CODE		34
#define TAPI_TONE_LOCALE_CONGESTION_CODE	35

#define LANTIQ_CONTEXT_PREFIX "lantiq"
#define DEFAULT_INTERDIGIT_TIMEOUT 4000
#define G723_HIGH_RATE	1
#define LED_NAME_LENGTH 32

static const char config[] = "lantiq.conf";

static char firmware_filename[PATH_MAX] = "/lib/firmware/ifx_firmware.bin";
static char bbd_filename[PATH_MAX] = "/lib/firmware/ifx_bbd_fxs.bin";
static char base_path[PATH_MAX] = "/dev/vmmc";
static int per_channel_context = 0;

/* tone generator types */
enum tone_generator_t {
	TONE_INTEGRATED, /* tapi tone generator */
	TONE_ASTERISK, /* asterisk tone generator where possible */
	TONE_MEDIA /* media tone where possible */
};

/* tone generator (default is integraded) */
static enum tone_generator_t tone_generator = TONE_INTEGRATED;

/* tone zones for dial, ring, busy and congestion */
struct ast_tone_zone_sound *ts_dial;
struct ast_tone_zone_sound *ts_ring;
struct ast_tone_zone_sound *ts_busy;
struct ast_tone_zone_sound *ts_congestion;

/*
 * The private structures of the Phone Jack channels are linked for selecting
 * outgoing channels.
 */
enum channel_state {
	ONHOOK,
	OFFHOOK,
	DIALING,
	INCALL,
	CALL_ENDED,
	RINGING,
	UNKNOWN
};

static struct lantiq_pvt {
	struct ast_channel *owner;       /* Channel we belong to, possibly NULL   */
	int port_id;                     /* Port number of this object, 0..n      */
	int channel_state;
	char context[AST_MAX_CONTEXT];   /* this port's dialplan context          */
	int dial_timer;                  /* timer handle for autodial timeout     */
	char dtmfbuf[AST_MAX_EXTENSION]; /* buffer holding dialed digits          */
	int dtmfbuf_len;                 /* lenght of dtmfbuf                     */
	int rtp_timestamp;               /* timestamp for RTP packets             */
	int ptime;			 /* Codec base ptime			  */
	uint16_t rtp_seqno;              /* Sequence nr for RTP packets           */
	uint32_t call_setup_start;       /* Start of dialling in ms               */
	uint32_t call_setup_delay;       /* time between ^ and 1st ring in ms     */
	uint32_t call_start;             /* time we started dialling / answered   */
	uint32_t call_answer;            /* time the callee answered our call     */
	uint16_t jb_size;                /* Jitter buffer size                    */
	uint32_t jb_underflow;           /* Jitter buffer injected samples        */
	uint32_t jb_overflow;            /* Jitter buffer dropped samples         */
	uint16_t jb_delay;               /* Jitter buffer: playout delay          */
	uint16_t jb_invalid;             /* Jitter buffer: Nr. of invalid packets */
} *iflist = NULL;

static struct lantiq_ctx {
		int dev_fd;
		int channels;
		int ch_fd[TAPI_AUDIO_PORT_NUM_MAX];
		char voip_led[LED_NAME_LENGTH]; /* VOIP LED name */
		char ch_led[TAPI_AUDIO_PORT_NUM_MAX][LED_NAME_LENGTH]; /* FXS LED names */
		int interdigit_timeout; /* Timeout in ms between dialed digits */
} dev_ctx;

static int ast_digit_begin(struct ast_channel *ast, char digit);
static int ast_digit_end(struct ast_channel *ast, char digit, unsigned int duration);
static int ast_lantiq_call(struct ast_channel *ast, const char *dest, int timeout);
static int ast_lantiq_hangup(struct ast_channel *ast);
static int ast_lantiq_answer(struct ast_channel *ast);
static struct ast_frame *ast_lantiq_read(struct ast_channel *ast);
static int ast_lantiq_write(struct ast_channel *ast, struct ast_frame *frame);
static struct ast_frame *ast_lantiq_exception(struct ast_channel *ast);
static int ast_lantiq_indicate(struct ast_channel *chan, int condition, const void *data, size_t datalen);
static int ast_lantiq_fixup(struct ast_channel *old, struct ast_channel *new);
static struct ast_channel *ast_lantiq_requester(const char *type, struct ast_format_cap *cap, const struct ast_assigned_ids *assigned_ids, const struct ast_channel *requestor, const char *data, int *cause);
static int ast_lantiq_devicestate(const char *data);
static int acf_channel_read(struct ast_channel *chan, const char *funcname, char *args, char *buf, size_t buflen);
static void lantiq_jb_get_stats(int c);
static struct ast_format *lantiq_map_rtptype_to_format(uint8_t rtptype);
static uint8_t lantiq_map_format_to_rtptype(const struct ast_format *format);
static int lantiq_conf_enc(int c, const struct ast_format *format);
static void lantiq_reset_dtmfbuf(struct lantiq_pvt *pvt);

static struct ast_channel_tech lantiq_tech = {
	.type = "TAPI",
	.description = "Lantiq TAPI Telephony API Driver",
	.send_digit_begin = ast_digit_begin,
	.send_digit_end = ast_digit_end,
	.call = ast_lantiq_call,
	.hangup = ast_lantiq_hangup,
	.answer = ast_lantiq_answer,
	.read = ast_lantiq_read,
	.write = ast_lantiq_write,
	.exception = ast_lantiq_exception,
	.indicate = ast_lantiq_indicate,
	.fixup = ast_lantiq_fixup,
	.requester = ast_lantiq_requester,
	.devicestate = ast_lantiq_devicestate,
	.func_channel_read = acf_channel_read
};

/* Protect the interface list (of lantiq_pvt's) */
AST_MUTEX_DEFINE_STATIC(iflock);

/*
 * Protect the monitoring thread, so only one process can kill or start it, and
 * not when it's doing something critical.
 */
AST_MUTEX_DEFINE_STATIC(monlock);

/* The scheduling context */
struct ast_sched_context *sched;

/*
 * This is the thread for the monitor which checks for input on the channels
 * which are not currently in use.
 */
static pthread_t monitor_thread = AST_PTHREADT_NULL;


#define WORDS_BIGENDIAN
/* struct taken from some GPLed code by  Mike Borella */
typedef struct rtp_header
{
#if defined(WORDS_BIGENDIAN)
  uint8_t version:2, padding:1, extension:1, csrc_count:4;
  uint8_t marker:1, payload_type:7;
#else
  uint8_t csrc_count:4, extension:1, padding:1, version:2;
  uint8_t payload_type:7, marker:1;
#endif
  uint16_t seqno;
  uint32_t timestamp;
  uint32_t ssrc;
} rtp_header_t;
#define RTP_HEADER_LEN 12
#define RTP_BUFFER_LEN 512
/* Internal RTP payload types - standard */
#define RTP_PCMU	0
#define RTP_G723_63	4
#define RTP_PCMA	8
#define RTP_G722	9
#define RTP_CN		13
#define RTP_G729	18
/* Internal RTP payload types - custom   */
#define RTP_G7221	100
#define RTP_G726	101
#define RTP_ILBC	102
#define RTP_SLIN8	103
#define RTP_SLIN16	104
#define RTP_SIREN7	105
#define RTP_G723_53	106


/* LED Control. Taken with modifications from SVD by Luca Olivetti <olivluca@gmail.com> */
#define LED_SLOW_BLINK	1000
#define LED_FAST_BLINK	100
static FILE *led_open(const char *led, char* sub)
{
	char fname[100];

	if (snprintf(fname, sizeof(fname), "/sys/class/leds/%s/%s", led, sub) >= sizeof(fname))
		return NULL;
	return fopen(fname, "r+");
}

static FILE *led_trigger(const char *led)
{
	return led_open(led, "trigger");
}

static void led_delay(const char *led, int onoff, int msec)
{
	FILE *fp = led_open(led, onoff ? "delay_on" : "delay_off");
	if (fp) {
		fprintf(fp,"%d\n",msec);
		fclose(fp);
	}
}

static void led_on(const char *led)
{
	FILE *fp;

	fp = led_trigger(led);
	if (fp) {
		fprintf(fp,"default-on\n");
		fclose(fp);
	}
}

static void led_off(const char *led)
{
	FILE *fp;

	fp = led_trigger(led);
	if (fp) {
		fprintf(fp,"none\n");
		fclose(fp);
	}
}

static void led_blink(const char *led, int period)
{
	FILE *fp;

	fp = led_trigger(led);
	if (fp) {
		fprintf(fp, "timer\n");
		fclose(fp);
		led_delay(led, 1, period/2);
		led_delay(led, 0, period/2);
	}
}

static uint32_t now(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	uint64_t tmp = ts.tv_sec*1000 + (ts.tv_nsec/1000000);
	return (uint32_t) tmp;
}

static uint32_t epoch(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec;
}

static int lantiq_dev_open(const char *dev_path, const int32_t ch_num)
{
	char dev_name[PATH_MAX];
	memset(dev_name, 0, sizeof(dev_name));
	snprintf(dev_name, PATH_MAX, "%s%u%u", dev_path, 1, ch_num);
	return open((const char*)dev_name, O_RDWR, 0644);
}

static void lantiq_ring(int c, int r, const char *cid, const char *name)
{
	uint8_t status;

	if (r) {
		led_blink(dev_ctx.ch_led[c], LED_FAST_BLINK);
		if (!cid) {
			status = (uint8_t) ioctl(dev_ctx.ch_fd[c], IFX_TAPI_RING_START, 0);
		} else {
			IFX_TAPI_CID_MSG_t msg;
			IFX_TAPI_CID_MSG_ELEMENT_t elements[3];
			int count = 0;
			time_t timestamp;
			struct tm *tm;

			elements[count].string.elementType = IFX_TAPI_CID_ST_CLI;
			elements[count].string.len = strlen(cid);
			if (elements[count].string.len > IFX_TAPI_CID_MSG_LEN_MAX) {
				elements[count].string.len = IFX_TAPI_CID_MSG_LEN_MAX;
			}
			strncpy((char *)elements[count].string.element, cid, IFX_TAPI_CID_MSG_LEN_MAX);
			elements[count].string.element[IFX_TAPI_CID_MSG_LEN_MAX-1] = '\0';
			count++;

			if (name) {
				elements[count].string.elementType = IFX_TAPI_CID_ST_NAME;
				elements[count].string.len = strlen(name);
				if (elements[count].string.len > IFX_TAPI_CID_MSG_LEN_MAX) {
					elements[count].string.len = IFX_TAPI_CID_MSG_LEN_MAX;
				}
				strncpy((char *)elements[count].string.element, name, IFX_TAPI_CID_MSG_LEN_MAX);
				elements[count].string.element[IFX_TAPI_CID_MSG_LEN_MAX-1] = '\0';
				count++;
			}

			if ((time(&timestamp) != -1) && ((tm=localtime(&timestamp)) != NULL)) {
				elements[count].date.elementType = IFX_TAPI_CID_ST_DATE;
				elements[count].date.day = tm->tm_mday;
				elements[count].date.month = tm->tm_mon + 1;
				elements[count].date.hour = tm->tm_hour;
				elements[count].date.mn = tm->tm_min;
				count++;
			}

			msg.txMode = IFX_TAPI_CID_HM_ONHOOK;
			msg.messageType = IFX_TAPI_CID_MT_CSUP;
			msg.message = elements;
			msg.nMsgElements = count;

			status = (uint8_t) ioctl(dev_ctx.ch_fd[c], IFX_TAPI_CID_TX_SEQ_START, (IFX_int32_t) &msg);
		}
	} else {
		status = (uint8_t) ioctl(dev_ctx.ch_fd[c], IFX_TAPI_RING_STOP, 0);
		led_off(dev_ctx.ch_led[c]);
	}

	if (status) {
		ast_log(LOG_ERROR, "%s ioctl failed\n",
			(r ? "IFX_TAPI_RING_START" : "IFX_TAPI_RING_STOP"));
	}
}

/* add a frequency to TAPE tone structure */
/* returns the TAPI frequency ID */
static int tapitone_add_freq (IFX_TAPI_TONE_t *tone, IFX_uint32_t freq) {
	const int n=4; /* TAPI tone structure supports up to 4 frequencies */
	int error=0;
	int ret;
	int i;

	/* pointer array for freq's A, B, C, D */
	IFX_uint32_t *freqarr[] = { &(tone->simple.freqA), &(tone->simple.freqB), &(tone->simple.freqC), &(tone->simple.freqD) };

	/* pointer array for level's A, B, C, D */
	IFX_int32_t *lvlarr[] = { &(tone->simple.levelA), &(tone->simple.levelB), &(tone->simple.levelC), &(tone->simple.levelD) };

	/* array for freq IDs */
	IFX_uint32_t retarr[] = { IFX_TAPI_TONE_FREQA, IFX_TAPI_TONE_FREQB, IFX_TAPI_TONE_FREQC, IFX_TAPI_TONE_FREQD };

	/* determine if freq already set */
	for (i = 0; i < n; i++) {
		if(*freqarr[i] == freq) /* freq found */
			break;
		else if (i == n-1) /* last iteration */
			error=1; /* not found */
	}

	/* write frequency if not already set */
	if(error) {
		error=0; /* reset error flag */
		/* since freq is not set, write it into first free place */
		for (i = 0; i < n; i++) {
			if(!*freqarr[i]) { /* free place */
				*freqarr[i] = freq; /* set freq */
				*lvlarr[i] = -150; /* set volume level */
				break;
			} else if (i == n-1) /* last iteration */
				error=1; /* no free place becaus maximum count of freq's is set */
		}
	}

	/* set freq ID return value */
	if (!freq || error)
		ret = IFX_TAPI_TONE_FREQNONE;
	else
		ret = retarr[i];

	return ret; /* freq ID */
}

/* convert asterisk playlist string to tapi tone structure */
/* based on ast_playtones_start() from indications.c of asterisk 13 */
static void playlist_to_tapitone (const char *playlst, IFX_uint32_t index, IFX_TAPI_TONE_t *tone)
{
	char *s, *data = ast_strdupa(playlst);
	char *stringp;
	char *separator;
	int i;

	/* initialize tapi tone structure */
	memset(tone, 0, sizeof(IFX_TAPI_TONE_t));
	tone->simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
	tone->simple.index = index;

	stringp = data;

	/* check if the data is separated with '|' or with ',' by default */
	if (strchr(stringp,'|')) {
		separator = "|";
	} else {
		separator = ",";
	}

	for ( i = 0; (s = strsep(&stringp, separator)) && !ast_strlen_zero(s) && i < IFX_TAPI_TONE_STEPS_MAX; i++) {
		struct ast_tone_zone_part tone_data = {
			.time = 0,
		};

		s = ast_strip(s);
		if (s[0]=='!') {
			s++;
		}

		if (ast_tone_zone_part_parse(s, &tone_data)) {
			ast_log(LOG_ERROR, "Failed to parse tone part '%s'\n", s);
			continue;
		}

		/* first tone must hava a cadence */
		if (i==0 && !tone_data.time)
			tone->simple.cadence[i] = 1000;
		else
			tone->simple.cadence[i] = tone_data.time;

		/* check for modulation */
		if (tone_data.modulate) {
			tone->simple.modulation[i] = IFX_TAPI_TONE_MODULATION_ON;
			tone->simple.modulation_factor = IFX_TAPI_TONE_MODULATION_FACTOR_90;
		}

		/* copy freq's to tapi tone structure */
		/* a freq will implicitly skipped if it is zero  */
		tone->simple.frequencies[i] |= tapitone_add_freq(tone, tone_data.freq1);
		tone->simple.frequencies[i] |= tapitone_add_freq(tone, tone_data.freq2);
	}
}

static int lantiq_play_tone(int c, int t)
{
	/* stop currently playing tone before starting new one */
	ioctl(dev_ctx.ch_fd[c], IFX_TAPI_TONE_LOCAL_PLAY, TAPI_TONE_LOCALE_NONE);

	/* do not handle stop tone twice */
	if (t != TAPI_TONE_LOCALE_NONE) {
		/* start new tone */
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_TONE_LOCAL_PLAY, t)) {
			ast_log(LOG_ERROR, "IFX_TAPI_TONE_LOCAL_PLAY ioctl failed\n");
			return -1;
		}
	}

	return 0;
}

static enum channel_state lantiq_get_hookstatus(int port)
{
	uint8_t status;

	if (ioctl(dev_ctx.ch_fd[port], IFX_TAPI_LINE_HOOK_STATUS_GET, &status)) {
		ast_log(LOG_ERROR, "IFX_TAPI_LINE_HOOK_STATUS_GET ioctl failed\n");
		return UNKNOWN;
	}

	if (status) {
		return OFFHOOK;
	} else {
		return ONHOOK;
	}
}

static int
lantiq_dev_binary_buffer_create(const char *path, uint8_t **ppBuf, uint32_t *pBufSz)
{
	FILE *fd;
	struct stat file_stat;
	int status = -1;

	fd = fopen(path, "rb");
	if (fd == NULL) {
		ast_log(LOG_ERROR, "binary file %s open failed\n", path);
		goto on_exit;
	}

	if (stat(path, &file_stat)) {
		ast_log(LOG_ERROR, "file %s statistics get failed\n", path);
		goto on_exit;
	}

	*ppBuf = malloc(file_stat.st_size);
	if (*ppBuf == NULL) {
		ast_log(LOG_ERROR, "binary file %s memory allocation failed\n", path);
		goto on_exit;
	}

	if (fread (*ppBuf, sizeof(uint8_t), file_stat.st_size, fd) != file_stat.st_size) {
		ast_log(LOG_ERROR, "file %s read failed\n", path);
		status = -1;
		goto on_exit;
	}

	*pBufSz = file_stat.st_size;
	status = 0;

on_exit:
	if (fd != NULL)
		fclose(fd);

	if (*ppBuf != NULL && status)
		free(*ppBuf);

	return status;
}

static int32_t lantiq_dev_firmware_download(int32_t fd, const char *path)
{
	uint8_t *firmware = NULL;
	uint32_t size = 0;
	VMMC_IO_INIT vmmc_io_init;

	ast_log(LOG_DEBUG, "loading firmware: \"%s\".\n", path);

	if (lantiq_dev_binary_buffer_create(path, &firmware, &size))
		return -1;

	memset(&vmmc_io_init, 0, sizeof(VMMC_IO_INIT));
	vmmc_io_init.pPRAMfw = firmware;
	vmmc_io_init.pram_size = size;

	if (ioctl(fd, FIO_FW_DOWNLOAD, &vmmc_io_init)) {
		ast_log(LOG_ERROR, "FIO_FW_DOWNLOAD ioctl failed\n");
		return -1;
	}

	if (firmware != NULL)
		free(firmware);

	return 0;
}

static const char *state_string(enum channel_state s)
{
	switch (s) {
		case ONHOOK: return "ONHOOK";
		case OFFHOOK: return "OFFHOOK";
		case DIALING: return "DIALING";
		case INCALL: return "INCALL";
		case CALL_ENDED: return "CALL_ENDED";
		case RINGING: return "RINGING";
		default: return "UNKNOWN";
	}
}

static const char *control_string(int c)
{
	switch (c) {
		case AST_CONTROL_HANGUP: return "Other end has hungup";
		case AST_CONTROL_RING: return "Local ring";
		case AST_CONTROL_RINGING: return "Remote end is ringing";
		case AST_CONTROL_ANSWER: return "Remote end has answered";
		case AST_CONTROL_BUSY: return "Remote end is busy";
		case AST_CONTROL_TAKEOFFHOOK: return "Make it go off hook";
		case AST_CONTROL_OFFHOOK: return "Line is off hook";
		case AST_CONTROL_CONGESTION: return "Congestion (circuits busy)";
		case AST_CONTROL_FLASH: return "Flash hook";
		case AST_CONTROL_WINK: return "Wink";
		case AST_CONTROL_OPTION: return "Set a low-level option";
		case AST_CONTROL_RADIO_KEY: return "Key Radio";
		case AST_CONTROL_RADIO_UNKEY: return "Un-Key Radio";
		case AST_CONTROL_PROGRESS: return "Remote end is making Progress";
		case AST_CONTROL_PROCEEDING: return "Remote end is proceeding";
		case AST_CONTROL_HOLD: return "Hold";
		case AST_CONTROL_UNHOLD: return "Unhold";
		case AST_CONTROL_SRCUPDATE: return "Media Source Update";
		case AST_CONTROL_CONNECTED_LINE: return "Connected Line";
		case AST_CONTROL_REDIRECTING: return "Redirecting";
		case AST_CONTROL_INCOMPLETE: return "Incomplete";
		case -1: return "Stop tone";
		default: return "Unknown";
	}
}

static int ast_lantiq_indicate(struct ast_channel *chan, int condition, const void *data, size_t datalen)
{
	struct lantiq_pvt *pvt = ast_channel_tech_pvt(chan);

	ast_verb(3, "phone indication \"%s\"\n", control_string(condition));

	switch (condition) {
		case -1:
			{
				lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_NONE);
				return 0;
			}
		case AST_CONTROL_CONGESTION:
			{
				if (tone_generator == TONE_INTEGRATED)
					lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_CONGESTION_CODE);
				else
					ast_playtones_start(chan, 0, ts_congestion->data, 1);

				return 0;
			}
		case AST_CONTROL_BUSY:
			{
				if (tone_generator == TONE_INTEGRATED)
					lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_BUSY_CODE);
				else
					ast_playtones_start(chan, 0, ts_busy->data, 1);

				return 0;
			}
		case AST_CONTROL_RINGING:
		case AST_CONTROL_PROGRESS:
			{
				pvt->call_setup_delay = now() - pvt->call_setup_start;

				if (tone_generator == TONE_INTEGRATED)
					lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_RINGING_CODE);
				else if (tone_generator == TONE_ASTERISK) /* do nothing if TONE_MEDIA is set */
					ast_playtones_start(chan, 0, ts_ring->data, 1);

				return 0;
			}
		default:
			{
				/* -1 lets asterisk generate the tone */
				return -1;
			}
	}
}

static int ast_lantiq_fixup(struct ast_channel *old, struct ast_channel *new)
{
	ast_log(LOG_DEBUG, "entering... no code here...\n");
	return 0;
}

static int ast_digit_begin(struct ast_channel *chan, char digit)
{
	/* TODO: Modify this callback to let Asterisk support controlling the length of DTMF */
	ast_log(LOG_DEBUG, "entering... no code here...\n");
	return 0;
}

static int ast_digit_end(struct ast_channel *ast, char digit, unsigned int duration)
{
	ast_log(LOG_DEBUG, "entering... no code here...\n");
	return 0;
}

static int ast_lantiq_call(struct ast_channel *ast, const char *dest, int timeout)
{
	int res = 0;
	struct lantiq_pvt *pvt;

	/* lock to prevent simultaneous access with do_monitor thread processing */
	ast_mutex_lock(&iflock);

	pvt = ast_channel_tech_pvt(ast);
	ast_log(LOG_DEBUG, "state: %s\n", state_string(pvt->channel_state));

	if (pvt->channel_state == ONHOOK) {
		struct ast_party_id connected_id = ast_channel_connected_effective_id(ast);
		const char *cid = connected_id.number.valid ? connected_id.number.str : NULL;
		const char *name = connected_id.name.valid ? connected_id.name.str : NULL;

		ast_log(LOG_DEBUG, "port %i is ringing\n", pvt->port_id);
		ast_log(LOG_DEBUG, "port %i CID: %s\n", pvt->port_id, cid ? cid : "none");
		ast_log(LOG_DEBUG, "port %i NAME: %s\n", pvt->port_id, name ? name : "none");

		lantiq_ring(pvt->port_id, 1, cid, name);
		pvt->channel_state = RINGING;

		ast_setstate(ast, AST_STATE_RINGING);
		ast_queue_control(ast, AST_CONTROL_RINGING);
	} else {
		ast_log(LOG_DEBUG, "port %i is busy\n", pvt->port_id);
		ast_setstate(ast, AST_STATE_BUSY);
		ast_queue_control(ast, AST_CONTROL_BUSY);
		res = -1;
	}

	ast_mutex_unlock(&iflock);

	return res;
}

static int ast_lantiq_hangup(struct ast_channel *ast)
{
	struct lantiq_pvt *pvt;

	/* lock to prevent simultaneous access with do_monitor thread processing */
	ast_mutex_lock(&iflock);

	pvt = ast_channel_tech_pvt(ast);
	ast_log(LOG_DEBUG, "state: %s\n", state_string(pvt->channel_state));

	if (ast_channel_state(ast) == AST_STATE_RINGING) {
		ast_debug(1, "channel state is RINGING\n");
	}

	switch (pvt->channel_state) {
		case RINGING:
		case ONHOOK:
			lantiq_ring(pvt->port_id, 0, NULL, NULL);
			pvt->channel_state = ONHOOK;
			break;
		default:
			ast_log(LOG_DEBUG, "we were hung up, play busy tone\n");
			pvt->channel_state = CALL_ENDED;
			lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_BUSY_CODE);
	}

	lantiq_jb_get_stats(pvt->port_id);

	ast_setstate(ast, AST_STATE_DOWN);
	ast_module_unref(ast_module_info->self);
	ast_channel_tech_pvt_set(ast, NULL);
	pvt->owner = NULL;

	ast_mutex_unlock(&iflock);

	return 0;
}

static int ast_lantiq_answer(struct ast_channel *ast)
{
	struct lantiq_pvt *pvt = ast_channel_tech_pvt(ast);

	ast_log(LOG_DEBUG, "Remote end has answered call.\n");

	if (lantiq_conf_enc(pvt->port_id, ast_channel_writeformat(ast)))
		return -1;

	pvt->call_answer = epoch();

	return 0;
}

static struct ast_frame * ast_lantiq_read(struct ast_channel *ast)
{
	ast_log(LOG_DEBUG, "entering... no code here...\n");
	return NULL;
}

/* create asterisk format from rtp payload type */
static struct ast_format *lantiq_map_rtptype_to_format(uint8_t rtptype)
{
	struct ast_format *format = NULL;

	switch (rtptype) {
		case RTP_PCMU: format = ast_format_ulaw; break;
		case RTP_PCMA: format = ast_format_alaw; break;
		case RTP_G722: format = ast_format_g722; break;
		case RTP_G726: format = ast_format_g726; break;
		case RTP_SLIN8: format = ast_format_slin; break;
		case RTP_SLIN16: format = ast_format_slin16; break;
		case RTP_ILBC: format = ast_format_ilbc; break;
		case RTP_SIREN7: format = ast_format_siren7; break;
		case RTP_G723_63: format = ast_format_g723; break;
		case RTP_G723_53: format = ast_format_g723; break;
		case RTP_G729: format = ast_format_g729; break;
		default:
		{
			ast_log(LOG_ERROR, "unsupported rtptype received is 0x%x, forcing ulaw\n", (unsigned) rtptype);
			format = ast_format_ulaw;
		}
	}

	return format;
}

/* create rtp payload type from asterisk format */
static uint8_t lantiq_map_format_to_rtptype(const struct ast_format *format)
{
	uint8_t rtptype = 0;

	if (ast_format_cmp(format, ast_format_ulaw) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_PCMU;
	else if (ast_format_cmp(format, ast_format_alaw) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_PCMA;
	else if (ast_format_cmp(format, ast_format_g722) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_G722;
	else if (ast_format_cmp(format, ast_format_g726) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_G726;
	else if (ast_format_cmp(format, ast_format_slin) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_SLIN8;
	else if (ast_format_cmp(format, ast_format_slin16) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_SLIN16;
	else if (ast_format_cmp(format, ast_format_ilbc) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_ILBC;
	else if (ast_format_cmp(format, ast_format_siren7) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_SIREN7;
	else if (ast_format_cmp(format, ast_format_g723) == AST_FORMAT_CMP_EQUAL)
#if defined G723_HIGH_RATE
		rtptype = RTP_G723_63;
#else
		rtptype = RTP_G723_53;
#endif
	else if (ast_format_cmp(format, ast_format_g729) == AST_FORMAT_CMP_EQUAL)
		rtptype = RTP_G729;
	else {
		ast_log(LOG_ERROR, "unsupported format %s, forcing ulaw\n", ast_format_get_name(format));
		rtptype = RTP_PCMU;
	}

	return rtptype;
}

static int lantiq_conf_enc(int c, const struct ast_format *format)
{
	/* Configure encoder before starting RTP session */
	IFX_TAPI_ENC_CFG_t enc_cfg;

	memset(&enc_cfg, 0, sizeof(IFX_TAPI_ENC_CFG_t));

	if (ast_format_cmp(format, ast_format_ulaw) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_MLAW;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 10;
	} else if (ast_format_cmp(format, ast_format_alaw) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_ALAW;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 10;
	} else if (ast_format_cmp(format, ast_format_g722) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_G722_64;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 20;
	} else if (ast_format_cmp(format, ast_format_g726) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_G726_32;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 10;
	} else if (ast_format_cmp(format, ast_format_slin) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_LIN16_8;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 10;
	} else if (ast_format_cmp(format, ast_format_slin16) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_LIN16_16;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_10;
		iflist[c].ptime = 10;
	} else if (ast_format_cmp(format, ast_format_ilbc) == AST_FORMAT_CMP_EQUAL) {
		/* iLBC 15.2kbps is currently unsupported by Asterisk */
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_ILBC_133;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_30;
		iflist[c].ptime = 30;
	} else if (ast_format_cmp(format, ast_format_siren7) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_G7221_32;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 20;
	} else if (ast_format_cmp(format, ast_format_g723) == AST_FORMAT_CMP_EQUAL) {
#if defined G723_HIGH_RATE
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_G723_63;
#else
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_G723_53;
#endif
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_30;
		iflist[c].ptime = 30;
	} else if (ast_format_cmp(format, ast_format_g729) == AST_FORMAT_CMP_EQUAL) {
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_G729;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 10;
	} else {
		ast_log(LOG_ERROR, "unsupported format %s, forcing ulaw\n", ast_format_get_name(format));
		enc_cfg.nEncType = IFX_TAPI_COD_TYPE_MLAW;
		enc_cfg.nFrameLen = IFX_TAPI_COD_LENGTH_20;
		iflist[c].ptime = 10;
	}

	ast_log(LOG_DEBUG, "Configuring encoder to use TAPI codec type %d (%s) on channel %i\n", enc_cfg.nEncType, ast_format_get_name(format), c);

	if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_ENC_CFG_SET, &enc_cfg)) {
		ast_log(LOG_ERROR, "IFX_TAPI_ENC_CFG_SET %d failed\n", c);
	}

	if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_ENC_START, 0)) {
		ast_log(LOG_ERROR, "IFX_TAPI_ENC_START ioctl failed\n");
	}

	if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_DEC_START, 0)) {
		ast_log(LOG_ERROR, "IFX_TAPI_DEC_START ioctl failed\n");
	}

	return 0;
}

static int ast_lantiq_write(struct ast_channel *ast, struct ast_frame *frame)
{
	char buf[RTP_BUFFER_LEN];
	rtp_header_t *rtp_header = (rtp_header_t *) buf;
	struct lantiq_pvt *pvt = ast_channel_tech_pvt(ast);
	int ret;
	uint8_t rtptype;
	int subframes, subframes_rtp, length, samples;
	char *head, *tail;

	if(frame->frametype != AST_FRAME_VOICE) {
		ast_log(LOG_DEBUG, "unhandled frame type\n");
		return 0;
	}

	if (frame->datalen == 0) {
		ast_log(LOG_DEBUG, "we've been prodded\n");
		return 0;
	}

	/* get rtp payload type */
	rtptype = lantiq_map_format_to_rtptype(frame->subclass.format);

	rtp_header->version      = 2;
	rtp_header->padding      = 0;
	rtp_header->extension    = 0;
	rtp_header->csrc_count   = 0;
	rtp_header->marker       = 0;
	rtp_header->ssrc         = 0;
	rtp_header->payload_type = rtptype;

	subframes = (iflist[pvt->port_id].ptime + frame->len - 1) / iflist[pvt->port_id].ptime; /* number of subframes in AST frame */
	if (subframes == 0)
		subframes = 1;

	subframes_rtp = (RTP_BUFFER_LEN - RTP_HEADER_LEN) * subframes / frame->datalen; /* how many frames fit in a single RTP packet */

	/* By default stick to the maximum multiple of native frame length */
	length = subframes_rtp * frame->datalen / subframes;
	samples = length * frame->samples / frame->datalen;

	head = frame->data.ptr;
	tail = frame->data.ptr + frame->datalen;
	while (head < tail) {
		rtp_header->seqno        = pvt->rtp_seqno++;
		rtp_header->timestamp    = pvt->rtp_timestamp;

		if ((tail - head) < (RTP_BUFFER_LEN - RTP_HEADER_LEN)) {
			length = tail - head;
			samples = length * frame->samples / frame->datalen;
		}

		if ( frame->datalen <= (sizeof(buf) - RTP_HEADER_LEN) )
			memcpy(buf + RTP_HEADER_LEN, head, length);
		else {
			ast_log(LOG_WARNING, "buffer is too small\n");
			return -1;
		}

		head += length;
		pvt->rtp_timestamp += (rtp_header->payload_type == RTP_G722) ? samples / 2 : samples; /* per RFC3551 */

		ret = write(dev_ctx.ch_fd[pvt->port_id], buf, RTP_HEADER_LEN + length);
		if (ret < 0) {
			ast_debug(1, "TAPI: ast_lantiq_write(): error writing.\n");
			return -1;
		}
		if (ret != (RTP_HEADER_LEN + length)) {
			ast_log(LOG_WARNING, "Short TAPI write of %d bytes, expected %d bytes\n", ret, RTP_HEADER_LEN + length);
			continue;
		}
	}

	return 0;
}

static int acf_channel_read(struct ast_channel *chan, const char *funcname, char *args, char *buf, size_t buflen)
{
	struct lantiq_pvt *pvt;
	int res = 0;

	if (!chan || ast_channel_tech(chan) != &lantiq_tech) {
		ast_log(LOG_ERROR, "This function requires a valid Lantiq TAPI channel\n");
		return -1;
	}

	ast_mutex_lock(&iflock);

	pvt = (struct lantiq_pvt*) ast_channel_tech_pvt(chan);

	if (!strcasecmp(args, "csd")) {
		snprintf(buf, buflen, "%lu", (unsigned long int) pvt->call_setup_delay);
	} else if (!strcasecmp(args, "jitter_stats")){
		lantiq_jb_get_stats(pvt->port_id);
		snprintf(buf, buflen, "jbBufSize=%u,jbUnderflow=%u,jbOverflow=%u,jbDelay=%u,jbInvalid=%u",
				(uint32_t) pvt->jb_size,
				(uint32_t) pvt->jb_underflow,
				(uint32_t) pvt->jb_overflow,
				(uint32_t) pvt->jb_delay,
				(uint32_t) pvt->jb_invalid);
	} else if (!strcasecmp(args, "jbBufSize")) {
		snprintf(buf, buflen, "%u", (uint32_t) pvt->jb_size);
	} else if (!strcasecmp(args, "jbUnderflow")) {
		snprintf(buf, buflen, "%u", (uint32_t) pvt->jb_underflow);
	} else if (!strcasecmp(args, "jbOverflow")) {
		snprintf(buf, buflen, "%u", (uint32_t) pvt->jb_overflow);
	} else if (!strcasecmp(args, "jbDelay")) {
		snprintf(buf, buflen, "%u", (uint32_t) pvt->jb_delay);
	} else if (!strcasecmp(args, "jbInvalid")) {
		snprintf(buf, buflen, "%u", (uint32_t) pvt->jb_invalid);
	} else if (!strcasecmp(args, "start")) {
		struct tm *tm = gmtime((const time_t*)&pvt->call_start);
		strftime(buf, buflen, "%F %T", tm);
	} else if (!strcasecmp(args, "answer")) {
		struct tm *tm = gmtime((const time_t*)&pvt->call_answer);
		strftime(buf, buflen, "%F %T", tm);
	} else {
		res = -1;
	}

	ast_mutex_unlock(&iflock);

	return res;
}

static struct ast_frame * ast_lantiq_exception(struct ast_channel *ast)
{
	ast_log(LOG_DEBUG, "entering... no code here...\n");
	return NULL;
}

static void lantiq_jb_get_stats(int c) {
	struct lantiq_pvt *pvt = &iflist[c];

	IFX_TAPI_JB_STATISTICS_t param;
	memset (&param, 0, sizeof (param));
	if (ioctl (dev_ctx.ch_fd[c], IFX_TAPI_JB_STATISTICS_GET, (IFX_int32_t) &param) != IFX_SUCCESS) {
		ast_debug(1, "Error getting jitter buffer  stats.\n");
	} else {
#if !defined (TAPI_VERSION3) && defined (TAPI_VERSION4)
		ast_debug(1, "Jitter buffer stats:  dev=%u, ch=%u, nType=%u, nBufSize=%u, nIsUnderflow=%u, nDsOverflow=%u, nPODelay=%u, nInvalid=%u\n",
				(uint32_t) param.dev,
				(uint32_t) param.ch,
#else
		ast_debug(1, "Jitter buffer stats:  nType=%u, nBufSize=%u, nIsUnderflow=%u, nDsOverflow=%u, nPODelay=%u, nInvalid=%u\n",
#endif
				(uint32_t) param.nType,
				(uint32_t) param.nBufSize,
				(uint32_t) param.nIsUnderflow,
				(uint32_t) param.nDsOverflow,
				(uint32_t) param.nPODelay,
				(uint32_t) param.nInvalid);

		pvt->jb_size = param.nBufSize;
		pvt->jb_underflow = param.nIsUnderflow;
		pvt->jb_overflow = param.nDsOverflow;
		pvt->jb_invalid = param.nInvalid;
		pvt->jb_delay = param.nPODelay;
	}
}


static int lantiq_standby(int c)
{
	ast_debug(1, "Stopping line feed for channel %i\n", c);
	if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_STANDBY)) {
		ast_log(LOG_ERROR, "IFX_TAPI_LINE_FEED_SET ioctl failed\n");
		return -1;
	}

	if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_ENC_STOP, 0)) {
		ast_log(LOG_ERROR, "IFX_TAPI_ENC_STOP ioctl failed\n");
		return -1;
	}

	if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_DEC_STOP, 0)) {
		ast_log(LOG_ERROR, "IFX_TAPI_DEC_STOP ioctl failed\n");
		return -1;
	}

	return lantiq_play_tone(c, TAPI_TONE_LOCALE_NONE);
}

static int lantiq_end_dialing(int c)
{
	struct lantiq_pvt *pvt = &iflist[c];

	ast_log(LOG_DEBUG, "end of dialing\n");

	if (pvt->dial_timer != -1) {
		AST_SCHED_DEL(sched, pvt->dial_timer);
		pvt->dial_timer = -1;
	}

	if(pvt->owner) {
		ast_hangup(pvt->owner);
	}
	lantiq_reset_dtmfbuf(pvt);

	return 0;
}

static int lantiq_end_call(int c)
{
	struct lantiq_pvt *pvt = &iflist[c];

	ast_log(LOG_DEBUG, "end of call\n");

	if(pvt->owner) {
		lantiq_jb_get_stats(c);
		ast_queue_hangup(pvt->owner);
	}

	return 0;
}

static struct ast_channel *lantiq_channel(int state, int c, char *ext, char *ctx, struct ast_format_cap *cap, const struct ast_assigned_ids *assigned_ids, const struct ast_channel *requestor)
{
	struct ast_channel *chan = NULL;
	struct lantiq_pvt *pvt = &iflist[c];
	struct ast_format *format = NULL;
	struct ast_format_cap *newcap = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT);
	struct ast_format_cap *formatcap = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT);

	if (!newcap || !formatcap) {
		ast_log(LOG_DEBUG, "Cannot allocate format capabilities!\n");
		return NULL;
	}

	chan = ast_channel_alloc(1, state, NULL, NULL, "", ext, ctx, assigned_ids, requestor, c, "TAPI/%d", (c + 1));
	if (!chan) {
		ast_log(LOG_DEBUG, "Cannot allocate channel!\n");
		ao2_ref(newcap, -1);
		ao2_ref(formatcap, -1);
		return NULL;
	}

	ast_channel_tech_set(chan, &lantiq_tech);
	ast_channel_tech_pvt_set(chan, pvt);
	pvt->owner = chan;

	if (cap && ast_format_cap_iscompatible(cap, lantiq_tech.capabilities)) { /* compatible format capabilities given */
		ast_format_cap_get_compatible(lantiq_tech.capabilities, cap, newcap);
		format = ast_format_cap_get_format(newcap, 0); /* choose format */
	} else { /* no or unsupported format capabilities given */
		format = ast_format_cap_get_format(lantiq_tech.capabilities, 0); /* choose format from capabilities */
	}

	/* set choosed format */
	ast_format_cap_append(formatcap, format, 0);
	ast_channel_nativeformats_set(chan, formatcap);
	ast_channel_set_readformat(chan, format);
	ast_channel_set_writeformat(chan, format);
	ast_channel_set_rawreadformat(chan, format);
	ast_channel_set_rawwriteformat(chan, format);

	ao2_ref(newcap, -1);
	ao2_ref(formatcap, -1);

	/* configuring encoder */
	if (format != 0)
		if (lantiq_conf_enc(c, format) < 0)
			return NULL;

	return chan;
}

static struct ast_channel *ast_lantiq_requester(const char *type, struct ast_format_cap *cap, const struct ast_assigned_ids *assigned_ids, const struct ast_channel *requestor, const char *data, int *cause)
{
	struct ast_str *buf = ast_str_alloca(64);
	struct ast_channel *chan = NULL;
	int port_id = -1;

	ast_mutex_lock(&iflock);

	ast_debug(1, "Asked to create a TAPI channel with formats: %s\n", ast_format_cap_get_names(cap, &buf));

	/* check for correct data argument */
	if (ast_strlen_zero(data)) {
		ast_log(LOG_ERROR, "Unable to create channel with empty destination.\n");
		*cause = AST_CAUSE_CHANNEL_UNACCEPTABLE;
		goto bailout;
	}

	/* get our port number */
	port_id = atoi((char*) data);
	if (port_id < 1 || port_id > dev_ctx.channels) {
		ast_log(LOG_ERROR, "Unknown channel ID: \"%s\"\n", data);
		*cause = AST_CAUSE_CHANNEL_UNACCEPTABLE;
		goto bailout;
	}

	/* on asterisk user's side, we're using port 1-2.
	 * Here in non normal human's world, we begin
	 * counting at 0.
	 */
	port_id -= 1;


	/* Bail out if channel is already in use */
	struct lantiq_pvt *pvt = &iflist[port_id];
	if (! pvt->channel_state == ONHOOK) {
		ast_debug(1, "TAPI channel %i alread in use.\n", port_id+1);
	} else {
		chan = lantiq_channel(AST_STATE_DOWN, port_id, NULL, NULL, cap, assigned_ids, requestor);
		ast_channel_unlock(chan);  /* it's essential to unlock channel */
	}

bailout:
	ast_mutex_unlock(&iflock);
	return chan;
}

static int ast_lantiq_devicestate(const char *data)
{
	int port = atoi(data) - 1;
	if ((port < 1) || (port > dev_ctx.channels)) {
		return AST_DEVICE_INVALID;
	}

	switch (iflist[port].channel_state) {
		case ONHOOK:
			return AST_DEVICE_NOT_INUSE;
		case OFFHOOK:
		case DIALING:
		case INCALL:
		case CALL_ENDED:
			return AST_DEVICE_INUSE;
		case RINGING:
			return AST_DEVICE_RINGING;
		case UNKNOWN:
		default:
			return AST_DEVICE_UNKNOWN;
	}
}

static int lantiq_dev_data_handler(int c)
{
	char buf[BUFSIZ];
	struct ast_frame frame = {0};
	struct ast_format *format = NULL;

	int res = read(dev_ctx.ch_fd[c], buf, sizeof(buf));
	if (res <= 0) {
		ast_log(LOG_ERROR, "we got read error %i\n", res);
		return -1;
	} else if (res < RTP_HEADER_LEN) {
		ast_log(LOG_ERROR, "we got data smaller than header size\n");
		return -1;
	}

	rtp_header_t *rtp = (rtp_header_t*) buf;
	struct lantiq_pvt *pvt = (struct lantiq_pvt *) &iflist[c];
	if ((!pvt->owner) || (ast_channel_state(pvt->owner) != AST_STATE_UP)) {
		return 0;
	}

	if (rtp->payload_type == RTP_CN) {
		/* TODO: Handle Comfort Noise frames */
		ast_debug(1, "Dropping Comfort Noise frame\n");
	}

	format = lantiq_map_rtptype_to_format(rtp->payload_type);
	frame.src = "TAPI";
	frame.frametype = AST_FRAME_VOICE;
	frame.subclass.format = format;
	frame.datalen = res - RTP_HEADER_LEN;
	frame.data.ptr = buf + RTP_HEADER_LEN;
	frame.samples = ast_codec_samples_count(&frame);

	if(!ast_channel_trylock(pvt->owner)) {
		ast_queue_frame(pvt->owner, &frame);
		ast_channel_unlock(pvt->owner);
	}

	return 0;
}

static int accept_call(int c)
{
	struct lantiq_pvt *pvt = &iflist[c];

	ast_log(LOG_DEBUG, "accept call\n");

	if (pvt->owner) {
		struct ast_channel *chan = pvt->owner;

		switch (ast_channel_state(chan)) {
			case AST_STATE_RINGING:
				lantiq_play_tone(c, TAPI_TONE_LOCALE_NONE);
				ast_queue_control(pvt->owner, AST_CONTROL_ANSWER);
				pvt->channel_state = INCALL;
				pvt->call_start = epoch();
				pvt->call_answer = pvt->call_start;
				break;
			default:
				ast_log(LOG_WARNING, "entered unhandled state %s\n", ast_state2str(ast_channel_state(chan)));
		}
	}

	return 0;
}

static int lantiq_dev_event_hook(int c, int state)
{
	ast_mutex_lock(&iflock);

	ast_log(LOG_DEBUG, "on port %i detected event %s hook\n", c, state ? "on" : "off");

	struct lantiq_pvt *pvt = &iflist[c];

	int ret = -1;
	if (state) { /* going onhook */
		switch (iflist[c].channel_state) {
			case DIALING:
				ret = lantiq_end_dialing(c);
				break;
			case INCALL:
				ret = lantiq_end_call(c);
				break;
		}

		iflist[c].channel_state = ONHOOK;

		/* stop DSP data feed */
		lantiq_standby(c);
		led_off(dev_ctx.ch_led[c]);

	} else { /* going offhook */
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_ACTIVE)) {
			ast_log(LOG_ERROR, "IFX_TAPI_LINE_FEED_SET ioctl failed\n");
			goto out;
		}

		switch (iflist[c].channel_state) {
			case RINGING:
				ret = accept_call(c);
				led_blink(dev_ctx.ch_led[c], LED_SLOW_BLINK);
				break;
			default:
				iflist[c].channel_state = OFFHOOK;
				lantiq_play_tone(c, TAPI_TONE_LOCALE_DIAL_CODE);
				lantiq_reset_dtmfbuf(pvt);
				ret = 0;
				led_on(dev_ctx.ch_led[c]);
				break;
		}

	}

out:
	ast_mutex_unlock(&iflock);

	return ret;
}

static void lantiq_reset_dtmfbuf(struct lantiq_pvt *pvt)
{
	pvt->dtmfbuf[0] = '\0';
	pvt->dtmfbuf_len = 0;
}

static void lantiq_dial(struct lantiq_pvt *pvt)
{
	struct ast_channel *chan = NULL;

	ast_mutex_lock(&iflock);
	ast_log(LOG_DEBUG, "user want's to dial %s.\n", pvt->dtmfbuf);

	if (ast_exists_extension(NULL, pvt->context, pvt->dtmfbuf, 1, NULL)) {
		ast_debug(1, "found extension %s, dialing\n", pvt->dtmfbuf);

		ast_verbose(VERBOSE_PREFIX_3 " extension exists, starting PBX %s\n", pvt->dtmfbuf);

		chan = lantiq_channel(AST_STATE_UP, pvt->port_id, pvt->dtmfbuf, pvt->context, NULL, 0, NULL);
		if (!chan) {
			ast_log(LOG_ERROR, "couldn't create channel\n");
			goto bailout;
		}
		ast_channel_tech_pvt_set(chan, pvt);
		pvt->owner = chan;

		ast_setstate(chan, AST_STATE_RING);
		pvt->channel_state = INCALL;

		pvt->call_setup_start = now();
		pvt->call_start = epoch();

		if (ast_pbx_start(chan)) {
			ast_log(LOG_WARNING, " unable to start PBX on %s\n", ast_channel_name(chan));
			ast_hangup(chan);
		}

		ast_channel_unlock(chan); /* it's essential to unlock channel */
	} else {
		ast_log(LOG_DEBUG, "no extension found\n");
		lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_CONGESTION_CODE);
		pvt->channel_state = CALL_ENDED;
	}

	lantiq_reset_dtmfbuf(pvt);
bailout:
	ast_mutex_unlock(&iflock);
}

static int lantiq_event_dial_timeout(const void* data)
{
	ast_debug(1, "TAPI: lantiq_event_dial_timeout()\n");

	struct lantiq_pvt *pvt = (struct lantiq_pvt *) data;
	pvt->dial_timer = -1;

	if (! pvt->channel_state == ONHOOK) {
		lantiq_dial(pvt);
	} else {
		ast_debug(1, "TAPI: lantiq_event_dial_timeout(): dial timeout in state ONHOOK.\n");
	}

	return 0;
}

static int lantiq_send_digit(int c, char digit)
{
	struct lantiq_pvt *pvt = &iflist[c];

	struct ast_frame f = { .frametype = AST_FRAME_DTMF, .subclass.integer = digit };

	if (pvt->owner) {
		ast_log(LOG_DEBUG, "Port %i transmitting digit \"%c\"\n", c, digit);
		return ast_queue_frame(pvt->owner, &f);
	} else {
		ast_debug(1, "Warning: lantiq_send_digit() without owner!\n");
		return -1;
	}
}

static void lantiq_dev_event_digit(int c, char digit)
{
	ast_mutex_lock(&iflock);

	ast_log(LOG_DEBUG, "on port %i detected digit \"%c\"\n", c, digit);

	struct lantiq_pvt *pvt = &iflist[c];

	switch (pvt->channel_state) {
		case INCALL:
			lantiq_send_digit(c, digit);
			break;
		case OFFHOOK:
			pvt->channel_state = DIALING;

			lantiq_play_tone(c, TAPI_TONE_LOCALE_NONE);
			led_blink(dev_ctx.ch_led[c], LED_SLOW_BLINK);

			/* fall through */
		case DIALING:
			if (pvt->dtmfbuf_len < AST_MAX_EXTENSION - 1) {
				pvt->dtmfbuf[pvt->dtmfbuf_len] = digit;
				pvt->dtmfbuf[++pvt->dtmfbuf_len] = '\0';
			} else {
				/* No more room for another digit */
				lantiq_end_dialing(c);
				lantiq_play_tone(pvt->port_id, TAPI_TONE_LOCALE_BUSY_CODE);
				pvt->channel_state = CALL_ENDED;
				break;
			}

			/* setup autodial timer */
			if (pvt->dial_timer == -1) {
				ast_log(LOG_DEBUG, "setting new timer\n");
				pvt->dial_timer = ast_sched_add(sched, dev_ctx.interdigit_timeout, lantiq_event_dial_timeout, (const void*) pvt);
			} else {
				ast_log(LOG_DEBUG, "replacing timer\n");
				AST_SCHED_REPLACE(pvt->dial_timer, sched, dev_ctx.interdigit_timeout, lantiq_event_dial_timeout, (const void*) pvt);
			}
			break;
		default:
			ast_log(LOG_ERROR, "don't know what to do in unhandled state\n");
			break;
	}

	ast_mutex_unlock(&iflock);
	return;
}

static void lantiq_dev_event_handler(void)
{
	IFX_TAPI_EVENT_t event;
	unsigned int i;

	for (i = 0; i < dev_ctx.channels ; i++) {
		ast_mutex_lock(&iflock);

		memset (&event, 0, sizeof(event));
		event.ch = i;
		if (ioctl(dev_ctx.dev_fd, IFX_TAPI_EVENT_GET, &event)) {
			ast_mutex_unlock(&iflock);
			continue;
		}
		if (event.id == IFX_TAPI_EVENT_NONE) {
			ast_mutex_unlock(&iflock);
			continue;
		}

		ast_mutex_unlock(&iflock);

		switch(event.id) {
			case IFX_TAPI_EVENT_FXS_ONHOOK:
				lantiq_dev_event_hook(i, 1);
				break;
			case IFX_TAPI_EVENT_FXS_OFFHOOK:
				lantiq_dev_event_hook(i, 0);
				break;
			case IFX_TAPI_EVENT_DTMF_DIGIT:
				lantiq_dev_event_digit(i, (char)event.data.dtmf.ascii);
				break;
			case IFX_TAPI_EVENT_PULSE_DIGIT:
				if (event.data.pulse.digit == 0xB) {
					lantiq_dev_event_digit(i, '0');
				} else {
					lantiq_dev_event_digit(i, '0' + (char)event.data.pulse.digit);
				}
				break;
			case IFX_TAPI_EVENT_COD_DEC_CHG:
			case IFX_TAPI_EVENT_TONE_GEN_END:
			case IFX_TAPI_EVENT_CID_TX_SEQ_END:
				break;
			default:
				ast_log(LOG_ERROR, "Unknown TAPI event %08X. Restarting Asterisk...\n", event.id);
				sleep(1);
				ast_cli_command(-1, "core restart now");
				break;
		}
	}
}

static void * lantiq_events_monitor(void *data)
{
	ast_verbose("TAPI thread started\n");

	struct pollfd fds[TAPI_AUDIO_PORT_NUM_MAX + 1];
	int c;

	fds[0].fd = dev_ctx.dev_fd;
	fds[0].events = POLLIN;
	for (c = 0; c < dev_ctx.channels; c++) {
		fds[c + 1].fd = dev_ctx.ch_fd[c];
		fds[c + 1].events = POLLIN;
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	for (;;) {
		if (poll(fds, dev_ctx.channels + 1, 2000) <= 0) {
			continue;
		}

		ast_mutex_lock(&monlock);
		if (fds[0].revents & POLLIN) {
			lantiq_dev_event_handler();
		}

		for (c = 0; c < dev_ctx.channels; c++) {
			if ((fds[c + 1].revents & POLLIN) && (lantiq_dev_data_handler(c))) {
				ast_log(LOG_ERROR, "data handler %d failed\n", c);
				break;
			}
		}
		ast_mutex_unlock(&monlock);
	}

	return NULL;
}

static int restart_monitor(void)
{
	/* If we're supposed to be stopped -- stay stopped */
	if (monitor_thread == AST_PTHREADT_STOP)
		return 0;

	ast_mutex_lock(&monlock);

	if (monitor_thread == pthread_self()) {
		ast_mutex_unlock(&monlock);
		ast_log(LOG_WARNING, "Cannot kill myself\n");
		return -1;
	}

	if (monitor_thread != AST_PTHREADT_NULL) {
		/* Wake up the thread */
		pthread_kill(monitor_thread, SIGURG);
	} else {
		/* Start a new monitor */
		if (ast_pthread_create_background(&monitor_thread, NULL, lantiq_events_monitor, NULL) < 0) {
			ast_mutex_unlock(&monlock);
			ast_log(LOG_ERROR, "Unable to start monitor thread.\n");
			return -1;
		}
	}
	ast_mutex_unlock(&monlock);

	return 0;
}

static void lantiq_cleanup(void)
{
	int c;

	if (dev_ctx.dev_fd < 0) {
		return;
	}

	for (c = 0; c < dev_ctx.channels ; c++) {
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_STANDBY)) {
			ast_log(LOG_WARNING, "IFX_TAPI_LINE_FEED_SET ioctl failed\n");
		}

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_ENC_STOP, 0)) {
			ast_log(LOG_WARNING, "IFX_TAPI_ENC_STOP ioctl failed\n");
		}

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_DEC_STOP, 0)) {
			ast_log(LOG_WARNING, "IFX_TAPI_DEC_STOP ioctl failed\n");
		}
		led_off(dev_ctx.ch_led[c]);
	}

	if (ioctl(dev_ctx.dev_fd, IFX_TAPI_DEV_STOP, 0)) {
		ast_log(LOG_WARNING, "IFX_TAPI_DEV_STOP ioctl failed\n");
	}

	close(dev_ctx.dev_fd);
	dev_ctx.dev_fd = -1;
	led_off(dev_ctx.voip_led);
}

static int unload_module(void)
{
	int c;

	ast_channel_unregister(&lantiq_tech);

	if (ast_mutex_lock(&iflock)) {
		ast_log(LOG_WARNING, "Unable to lock the interface list\n");
		return -1;
	}
	for (c = 0; c < dev_ctx.channels ; c++) {
		if (iflist[c].owner)
			ast_softhangup(iflist[c].owner, AST_SOFTHANGUP_APPUNLOAD);
	}
	ast_mutex_unlock(&iflock);

	if (ast_mutex_lock(&monlock)) {
		ast_log(LOG_WARNING, "Unable to lock the monitor\n");
		return -1;
	}
	if (monitor_thread && (monitor_thread != AST_PTHREADT_STOP) && (monitor_thread != AST_PTHREADT_NULL)) {
		pthread_t th = monitor_thread;
		monitor_thread = AST_PTHREADT_STOP;
		pthread_cancel(th);
		pthread_kill(th, SIGURG);
		ast_mutex_unlock(&monlock);
		pthread_join(th, NULL);
	} else {
		monitor_thread = AST_PTHREADT_STOP;
		ast_mutex_unlock(&monlock);
	}

	ast_sched_context_destroy(sched);
	ast_mutex_destroy(&iflock);
	ast_mutex_destroy(&monlock);

	lantiq_cleanup();
	ast_free(iflist);

	return 0;
}

static struct lantiq_pvt *lantiq_init_pvt(struct lantiq_pvt *pvt)
{
	if (pvt) {
		pvt->owner = NULL;
		pvt->port_id = -1;
		pvt->channel_state = UNKNOWN;
		pvt->context[0] = '\0';
		pvt->dial_timer = -1;
		pvt->dtmfbuf[0] = '\0';
		pvt->dtmfbuf_len = 0;
		pvt->call_setup_start = 0;
		pvt->call_setup_delay = 0;
		pvt->call_answer = 0;
		pvt->jb_size = 0;
		pvt->jb_underflow = 0;
		pvt->jb_overflow = 0;
		pvt->jb_delay = 0;
		pvt->jb_invalid = 0;
	} else {
		ast_log(LOG_ERROR, "unable to clear pvt structure\n");
	}

	return pvt;
}

static int lantiq_create_pvts(void)
{
	int i;

	iflist = ast_calloc(1, sizeof(struct lantiq_pvt) * dev_ctx.channels);

	if (!iflist) {
		ast_log(LOG_ERROR, "unable to allocate memory\n");
		return -1;
	}

	for (i = 0; i < dev_ctx.channels; i++) {
		lantiq_init_pvt(&iflist[i]);
		iflist[i].port_id = i;
		if (per_channel_context) {
			snprintf(iflist[i].context, AST_MAX_CONTEXT, "%s%i", LANTIQ_CONTEXT_PREFIX, i + 1);
			ast_debug(1, "Context for channel %i: %s\n", i, iflist[i].context);
		} else {
			snprintf(iflist[i].context, AST_MAX_CONTEXT, "default");
		}
	}
	return 0;
}

static int lantiq_setup_rtp(int c)
{
	/* Configure RTP payload type tables */
	IFX_TAPI_PKT_RTP_PT_CFG_t rtpPTConf;

	memset((char*)&rtpPTConf, '\0', sizeof(rtpPTConf));

	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_G723_63] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_G723_63] = RTP_G723_63;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_G723_53] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_G723_53] = RTP_G723_53;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_G729] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_G729] = RTP_G729;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_MLAW] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_MLAW] = RTP_PCMU;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_ALAW] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_ALAW] = RTP_PCMA;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_G726_32] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_G726_32] = RTP_G726;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_ILBC_152] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_ILBC_152] = RTP_ILBC;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_LIN16_8] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_LIN16_8] = RTP_SLIN8;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_LIN16_16] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_LIN16_16] = RTP_SLIN16;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_G722_64] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_G722_64] = RTP_G722;
	rtpPTConf.nPTup[IFX_TAPI_COD_TYPE_G7221_32] = rtpPTConf.nPTdown[IFX_TAPI_COD_TYPE_G7221_32] = RTP_G7221;

	int ret;
	if ((ret = ioctl(dev_ctx.ch_fd[c], IFX_TAPI_PKT_RTP_PT_CFG_SET, (IFX_int32_t) &rtpPTConf))) {
		ast_log(LOG_ERROR, "IFX_TAPI_PKT_RTP_PT_CFG_SET failed: ret=%i\n", ret);
		return -1;
	}

	return 0;
}

static int load_module(void)
{
	struct ast_config *cfg;
	struct ast_variable *v;
	int txgain = 0;
	int rxgain = 0;
	int wlec_type = 0;
	int wlec_nlp = 0;
	int wlec_nbfe = 0;
	int wlec_nbne = 0;
	int wlec_wbne = 0;
	int jb_type = IFX_TAPI_JB_TYPE_ADAPTIVE;
	int jb_pckadpt = IFX_TAPI_JB_PKT_ADAPT_VOICE;
	int jb_localadpt = IFX_TAPI_JB_LOCAL_ADAPT_DEFAULT;
	int jb_scaling = 0x10;
	int jb_initialsize = 0x2d0;
	int jb_minsize = 0x50;
	int jb_maxsize = 0x5a0;
	int cid_type = IFX_TAPI_CID_STD_TELCORDIA;
	int vad_type = IFX_TAPI_ENC_VAD_NOVAD;
	dev_ctx.dev_fd = -1;
	dev_ctx.channels = TAPI_AUDIO_PORT_NUM_MAX;
	dev_ctx.interdigit_timeout = DEFAULT_INTERDIGIT_TIMEOUT;
	struct ast_tone_zone *tz;
	struct ast_flags config_flags = { 0 };
	int c;

	if(!(lantiq_tech.capabilities = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT))) {
	  ast_log(LOG_ERROR, "Unable to allocate format capabilities.\n");
	  return AST_MODULE_LOAD_DECLINE;
	}

	/* channel format capabilities */
	ast_format_cap_append(lantiq_tech.capabilities, ast_format_ulaw, 0);
	ast_format_cap_append(lantiq_tech.capabilities, ast_format_alaw, 0);
	ast_format_cap_append(lantiq_tech.capabilities, ast_format_g722, 0);
	ast_format_cap_append(lantiq_tech.capabilities, ast_format_g726, 0);
	ast_format_cap_append(lantiq_tech.capabilities, ast_format_slin, 0);

	/* Turn off the LEDs, just in case */
	led_off(dev_ctx.voip_led);
	for(c = 0; c < TAPI_AUDIO_PORT_NUM_MAX; c++)
		led_off(dev_ctx.ch_led[c]);

	if ((cfg = ast_config_load(config, config_flags)) == CONFIG_STATUS_FILEINVALID) {
		ast_log(LOG_ERROR, "Config file %s is in an invalid format.  Aborting.\n", config);
		return AST_MODULE_LOAD_DECLINE;
	}

	/* We *must* have a config file otherwise stop immediately */
	if (!cfg) {
		ast_log(LOG_ERROR, "Unable to load config %s\n", config);
		return AST_MODULE_LOAD_DECLINE;
	}

	if (ast_mutex_lock(&iflock)) {
		ast_log(LOG_ERROR, "Unable to lock interface list.\n");
		goto cfg_error;
	}

	for (v = ast_variable_browse(cfg, "interfaces"); v; v = v->next) {
		if (!strcasecmp(v->name, "channels")) {
			dev_ctx.channels = atoi(v->value);
			if (!dev_ctx.channels) {
				ast_log(LOG_ERROR, "Invalid value for channels in config %s\n", config);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "firmwarefilename")) {
			ast_copy_string(firmware_filename, v->value, sizeof(firmware_filename));
		} else if (!strcasecmp(v->name, "bbdfilename")) {
			ast_copy_string(bbd_filename, v->value, sizeof(bbd_filename));
		} else if (!strcasecmp(v->name, "basepath")) {
			ast_copy_string(base_path, v->value, sizeof(base_path));
		} else if (!strcasecmp(v->name, "per_channel_context")) {
			if (!strcasecmp(v->value, "on")) {
				per_channel_context = 1;
			} else if (!strcasecmp(v->value, "off")) {
				per_channel_context = 0;
			} else {
				ast_log(LOG_ERROR, "Unknown per_channel_context value '%s'. Try 'on' or 'off'.\n", v->value);
				goto cfg_error_il;
			}
		}
	}

	for (v = ast_variable_browse(cfg, "general"); v; v = v->next) {
		if (!strcasecmp(v->name, "rxgain")) {
			rxgain = atoi(v->value);
			if (!rxgain) {
				rxgain = 0;
				ast_log(LOG_WARNING, "Invalid rxgain: %s, using default.\n", v->value);
			}
		} else if (!strcasecmp(v->name, "txgain")) {
			txgain = atoi(v->value);
			if (!txgain) {
				txgain = 0;
				ast_log(LOG_WARNING, "Invalid txgain: %s, using default.\n", v->value);
			}
		} else if (!strcasecmp(v->name, "echocancel")) {
			if (!strcasecmp(v->value, "off")) {
				wlec_type = IFX_TAPI_WLEC_TYPE_OFF;
			} else if (!strcasecmp(v->value, "nlec")) {
				wlec_type = IFX_TAPI_WLEC_TYPE_NE;
				if (!strcasecmp(v->name, "echocancelfixedwindowsize")) {
					wlec_nbne = atoi(v->value);
				}
			} else if (!strcasecmp(v->value, "wlec")) {
				wlec_type = IFX_TAPI_WLEC_TYPE_NFE;
				if (!strcasecmp(v->name, "echocancelnfemovingwindowsize")) {
					wlec_nbfe = atoi(v->value);
				} else if (!strcasecmp(v->name, "echocancelfixedwindowsize")) {
					wlec_nbne = atoi(v->value);
				} else if (!strcasecmp(v->name, "echocancelwidefixedwindowsize")) {
					wlec_wbne = atoi(v->value);
				}
			} else if (!strcasecmp(v->value, "nees")) {
				wlec_type = IFX_TAPI_WLEC_TYPE_NE_ES;
			} else if (!strcasecmp(v->value, "nfees")) {
				wlec_type = IFX_TAPI_WLEC_TYPE_NFE_ES;
			} else if (!strcasecmp(v->value, "es")) {
				wlec_type = IFX_TAPI_WLEC_TYPE_ES;
			} else {
				wlec_type = IFX_TAPI_WLEC_TYPE_OFF;
				ast_log(LOG_ERROR, "Unknown echo cancellation type '%s'\n", v->value);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "echocancelnlp")) {
			if (!strcasecmp(v->value, "on")) {
				wlec_nlp = IFX_TAPI_WLEC_NLP_ON;
			} else if (!strcasecmp(v->value, "off")) {
				wlec_nlp = IFX_TAPI_WLEC_NLP_OFF;
			} else {
				ast_log(LOG_ERROR, "Unknown echo cancellation nlp '%s'\n", v->value);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "jitterbuffertype")) {
			if (!strcasecmp(v->value, "fixed")) {
				jb_type = IFX_TAPI_JB_TYPE_FIXED;
			} else if (!strcasecmp(v->value, "adaptive")) {
				jb_type = IFX_TAPI_JB_TYPE_ADAPTIVE;
				jb_localadpt = IFX_TAPI_JB_LOCAL_ADAPT_DEFAULT;
				if (!strcasecmp(v->name, "jitterbufferadaptation")) {
					if (!strcasecmp(v->value, "on")) {
						jb_localadpt = IFX_TAPI_JB_LOCAL_ADAPT_ON;
					} else if (!strcasecmp(v->value, "off")) {
						jb_localadpt = IFX_TAPI_JB_LOCAL_ADAPT_OFF;
					}
				} else if (!strcasecmp(v->name, "jitterbufferscalling")) {
					jb_scaling = atoi(v->value);
				} else if (!strcasecmp(v->name, "jitterbufferinitialsize")) {
					jb_initialsize = atoi(v->value);
				} else if (!strcasecmp(v->name, "jitterbufferminsize")) {
					jb_minsize = atoi(v->value);
				} else if (!strcasecmp(v->name, "jitterbuffermaxsize")) {
					jb_maxsize = atoi(v->value);
				}
			} else {
				ast_log(LOG_ERROR, "Unknown jitter buffer type '%s'\n", v->value);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "jitterbufferpackettype")) {
			if (!strcasecmp(v->value, "voice")) {
				jb_pckadpt = IFX_TAPI_JB_PKT_ADAPT_VOICE;
			} else if (!strcasecmp(v->value, "data")) {
				jb_pckadpt = IFX_TAPI_JB_PKT_ADAPT_DATA;
			} else if (!strcasecmp(v->value, "datanorep")) {
				jb_pckadpt = IFX_TAPI_JB_PKT_ADAPT_DATA_NO_REP;
			} else {
				ast_log(LOG_ERROR, "Unknown jitter buffer packet adaptation type '%s'\n", v->value);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "calleridtype")) {
			ast_log(LOG_DEBUG, "Setting CID type to %s.\n", v->value);
			if (!strcasecmp(v->value, "telecordia")) {
				cid_type = IFX_TAPI_CID_STD_TELCORDIA;
			} else if (!strcasecmp(v->value, "etsifsk")) {
				cid_type = IFX_TAPI_CID_STD_ETSI_FSK;
			} else if (!strcasecmp(v->value, "etsidtmf")) {
				cid_type = IFX_TAPI_CID_STD_ETSI_DTMF;
			} else if (!strcasecmp(v->value, "sin")) {
				cid_type = IFX_TAPI_CID_STD_SIN;
			} else if (!strcasecmp(v->value, "ntt")) {
				cid_type = IFX_TAPI_CID_STD_NTT;
			} else if (!strcasecmp(v->value, "kpndtmf")) {
				cid_type = IFX_TAPI_CID_STD_KPN_DTMF;
			} else if (!strcasecmp(v->value, "kpndtmffsk")) {
				cid_type = IFX_TAPI_CID_STD_KPN_DTMF_FSK;
			} else {
				ast_log(LOG_ERROR, "Unknown caller id type '%s'\n", v->value);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "voiceactivitydetection")) {
			if (!strcasecmp(v->value, "on")) {
				vad_type = IFX_TAPI_ENC_VAD_ON;
			} else if (!strcasecmp(v->value, "g711")) {
				vad_type = IFX_TAPI_ENC_VAD_G711;
			} else if (!strcasecmp(v->value, "cng")) {
				vad_type = IFX_TAPI_ENC_VAD_CNG_ONLY;
			} else if (!strcasecmp(v->value, "sc")) {
				vad_type = IFX_TAPI_ENC_VAD_SC_ONLY;
			} else {
				ast_log(LOG_ERROR, "Unknown voice activity detection value '%s'\n", v->value);
				goto cfg_error_il;
			}
		} else if (!strcasecmp(v->name, "interdigit")) {
			dev_ctx.interdigit_timeout = atoi(v->value);
			ast_log(LOG_DEBUG, "Setting interdigit timeout to %s.\n", v->value);
			if (!dev_ctx.interdigit_timeout) {
				dev_ctx.interdigit_timeout = DEFAULT_INTERDIGIT_TIMEOUT;
				ast_log(LOG_WARNING, "Invalid interdigit timeout: %s, using default.\n", v->value);
			}
		} else if (!strcasecmp(v->name, "tone_generator")) {
			if (!strcasecmp(v->value, "integrated")) {
				tone_generator = TONE_INTEGRATED;
			} else if (!strcasecmp(v->value, "asterisk")) {
				tone_generator = TONE_ASTERISK;
			} else if (!strcasecmp(v->value, "media")) {
				tone_generator = TONE_MEDIA;
			} else {
				ast_log(LOG_ERROR, "Unknown tone_generator value '%s'. Try 'integrated', 'asterisk' or 'media'.\n", v->value);
				goto cfg_error_il;
			}
		}
	}

	lantiq_create_pvts();

	ast_mutex_unlock(&iflock);
	ast_config_destroy(cfg);

	if (!(sched = ast_sched_context_create())) {
		ast_log(LOG_ERROR, "Unable to create scheduler context\n");
		goto load_error;
	}

	if (ast_sched_start_thread(sched)) {
		ast_log(LOG_ERROR, "Unable to create scheduler context thread\n");
		goto load_error_st;
	}

	if (ast_channel_register(&lantiq_tech)) {
		ast_log(LOG_ERROR, "Unable to register channel class 'Phone'\n");
		goto load_error_st;
	}

	/* tapi */
	IFX_TAPI_TONE_t tone;
	IFX_TAPI_DEV_START_CFG_t dev_start;
	IFX_TAPI_MAP_DATA_t map_data;
	IFX_TAPI_LINE_TYPE_CFG_t line_type;
	IFX_TAPI_LINE_VOLUME_t line_vol;
	IFX_TAPI_WLEC_CFG_t wlec_cfg;
	IFX_TAPI_JB_CFG_t jb_cfg;
	IFX_TAPI_CID_CFG_t cid_cfg;

	/* open device */
	dev_ctx.dev_fd = lantiq_dev_open(base_path, 0);

	if (dev_ctx.dev_fd < 0) {
		ast_log(LOG_ERROR, "lantiq TAPI device open function failed\n");
		goto load_error_st;
	}

	snprintf(dev_ctx.voip_led, LED_NAME_LENGTH, "voice");
	for (c = 0; c < dev_ctx.channels ; c++) {
		dev_ctx.ch_fd[c] = lantiq_dev_open(base_path, c + 1);

		if (dev_ctx.ch_fd[c] < 0) {
			ast_log(LOG_ERROR, "lantiq TAPI channel %d open function failed\n", c);
			goto load_error_st;
		}
		snprintf(dev_ctx.ch_led[c], LED_NAME_LENGTH, "fxs%d", c + 1);
	}

	if (lantiq_dev_firmware_download(dev_ctx.dev_fd, firmware_filename)) {
		ast_log(LOG_ERROR, "voice firmware download failed\n");
		goto load_error_st;
	}

	if (ioctl(dev_ctx.dev_fd, IFX_TAPI_DEV_STOP, 0)) {
		ast_log(LOG_ERROR, "IFX_TAPI_DEV_STOP ioctl failed\n");
		goto load_error_st;
	}

	memset(&dev_start, 0x0, sizeof(IFX_TAPI_DEV_START_CFG_t));
	dev_start.nMode = IFX_TAPI_INIT_MODE_VOICE_CODER;

	/* Start TAPI */
	if (ioctl(dev_ctx.dev_fd, IFX_TAPI_DEV_START, &dev_start)) {
		ast_log(LOG_ERROR, "IFX_TAPI_DEV_START ioctl failed\n");
		goto load_error_st;
	}

	tz = ast_get_indication_zone(NULL);

	if (!tz) {
		ast_log(LOG_ERROR, "Unable to alloc tone zone\n");
		goto load_error_st;
	}

	ts_dial = ast_get_indication_tone(tz, "dial");
	ts_ring = ast_get_indication_tone(tz, "ring");
	ts_busy = ast_get_indication_tone(tz, "busy");
	ts_congestion = ast_get_indication_tone(tz, "congestion");

	if (!ts_dial || !ts_dial->data || !ts_ring || !ts_ring->data || !ts_busy || !ts_busy->data || !ts_congestion || !ts_congestion->data) {
		ast_log(LOG_ERROR, "Unable to get indication tones\n");
		goto load_error_st;
	}

	for (c = 0; c < dev_ctx.channels ; c++) {
		/* We're a FXS and want to switch between narrow & wide band automatically */
		memset(&line_type, 0, sizeof(IFX_TAPI_LINE_TYPE_CFG_t));
		line_type.lineType = IFX_TAPI_LINE_TYPE_FXS_AUTO;
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_LINE_TYPE_SET, &line_type)) {
			ast_log(LOG_ERROR, "IFX_TAPI_LINE_TYPE_SET %d failed\n", c);
			goto load_error_st;
		}

		/* tones */
		memset(&tone, 0, sizeof(IFX_TAPI_TONE_t));
		tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
		playlist_to_tapitone(ts_dial->data, TAPI_TONE_LOCALE_DIAL_CODE, &tone);
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_TONE_TABLE_CFG_SET, &tone)) {
			ast_log(LOG_ERROR, "IFX_TAPI_TONE_TABLE_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		memset(&tone, 0, sizeof(IFX_TAPI_TONE_t));
		tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
		playlist_to_tapitone(ts_ring->data, TAPI_TONE_LOCALE_RINGING_CODE, &tone);
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_TONE_TABLE_CFG_SET, &tone)) {
			ast_log(LOG_ERROR, "IFX_TAPI_TONE_TABLE_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		memset(&tone, 0, sizeof(IFX_TAPI_TONE_t));
		tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
		playlist_to_tapitone(ts_busy->data, TAPI_TONE_LOCALE_BUSY_CODE, &tone);
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_TONE_TABLE_CFG_SET, &tone)) {
			ast_log(LOG_ERROR, "IFX_TAPI_TONE_TABLE_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		memset(&tone, 0, sizeof(IFX_TAPI_TONE_t));
		tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
		playlist_to_tapitone(ts_congestion->data, TAPI_TONE_LOCALE_CONGESTION_CODE, &tone);
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_TONE_TABLE_CFG_SET, &tone)) {
			ast_log(LOG_ERROR, "IFX_TAPI_TONE_TABLE_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		/* ringing type */
		IFX_TAPI_RING_CFG_t ringingType;
		memset(&ringingType, 0, sizeof(IFX_TAPI_RING_CFG_t));
		ringingType.nMode = IFX_TAPI_RING_CFG_MODE_INTERNAL_BALANCED;
		ringingType.nSubmode = IFX_TAPI_RING_CFG_SUBMODE_DC_RNG_TRIP_FAST;
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_RING_CFG_SET, (IFX_int32_t) &ringingType)) {
			ast_log(LOG_ERROR, "IFX_TAPI_RING_CFG_SET failed\n");
			goto load_error_st;
		}

		/* ring cadence */
		IFX_char_t data[15] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00 };

		IFX_TAPI_RING_CADENCE_t ringCadence;
		memset(&ringCadence, 0, sizeof(IFX_TAPI_RING_CADENCE_t));
		memcpy(&ringCadence.data, data, sizeof(data));
		ringCadence.nr = sizeof(data) * 8;

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_RING_CADENCE_HR_SET, &ringCadence)) {
			ast_log(LOG_ERROR, "IFX_TAPI_RING_CADENCE_HR_SET failed\n");
			goto load_error_st;
		}

		/* perform mapping */
		memset(&map_data, 0x0, sizeof(IFX_TAPI_MAP_DATA_t));
		map_data.nDstCh = c;
		map_data.nChType = IFX_TAPI_MAP_TYPE_PHONE;

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_MAP_DATA_ADD, &map_data)) {
			ast_log(LOG_ERROR, "IFX_TAPI_MAP_DATA_ADD %d failed\n", c);
			goto load_error_st;
		}

		/* set line feed */
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_STANDBY)) {
			ast_log(LOG_ERROR, "IFX_TAPI_LINE_FEED_SET %d failed\n", c);
			goto load_error_st;
		}

		/* set volume */
		memset(&line_vol, 0, sizeof(line_vol));
		line_vol.nGainRx = rxgain;
		line_vol.nGainTx = txgain;

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_PHONE_VOLUME_SET, &line_vol)) {
			ast_log(LOG_ERROR, "IFX_TAPI_PHONE_VOLUME_SET %d failed\n", c);
			goto load_error_st;
		}

		/* Configure line echo canceller */
		memset(&wlec_cfg, 0, sizeof(wlec_cfg));
		wlec_cfg.nType = wlec_type;
		wlec_cfg.bNlp = wlec_nlp;
		wlec_cfg.nNBFEwindow = wlec_nbfe;
		wlec_cfg.nNBNEwindow = wlec_nbne;
		wlec_cfg.nWBNEwindow = wlec_wbne;

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_WLEC_PHONE_CFG_SET, &wlec_cfg)) {
			ast_log(LOG_ERROR, "IFX_TAPI_WLEC_PHONE_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		/* Configure jitter buffer */
		memset(&jb_cfg, 0, sizeof(jb_cfg));
		jb_cfg.nJbType = jb_type;
		jb_cfg.nPckAdpt = jb_pckadpt;
		jb_cfg.nLocalAdpt = jb_localadpt;
		jb_cfg.nScaling = jb_scaling;
		jb_cfg.nInitialSize = jb_initialsize;
		jb_cfg.nMinSize = jb_minsize;
		jb_cfg.nMaxSize = jb_maxsize;

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_JB_CFG_SET, &jb_cfg)) {
			ast_log(LOG_ERROR, "IFX_TAPI_JB_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		/* Configure Caller ID type */
		memset(&cid_cfg, 0, sizeof(cid_cfg));
		cid_cfg.nStandard = cid_type;

		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_CID_CFG_SET, &cid_cfg)) {
			ast_log(LOG_ERROR, "IIFX_TAPI_CID_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		/* Configure voice activity detection */
		if (ioctl(dev_ctx.ch_fd[c], IFX_TAPI_ENC_VAD_CFG_SET, vad_type)) {
			ast_log(LOG_ERROR, "IFX_TAPI_ENC_VAD_CFG_SET %d failed\n", c);
			goto load_error_st;
		}

		/* Setup TAPI <-> internal RTP codec type mapping */
		if (lantiq_setup_rtp(c)) {
			goto load_error_st;
		}

		/* Set initial hook status */
		iflist[c].channel_state = lantiq_get_hookstatus(c);

		if (iflist[c].channel_state == UNKNOWN) {
			goto load_error_st;
		}
	}

	/* make sure our device will be closed properly */
	ast_register_atexit(lantiq_cleanup);

	restart_monitor();
	led_on(dev_ctx.voip_led);
	return AST_MODULE_LOAD_SUCCESS;

cfg_error_il:
	ast_mutex_unlock(&iflock);
cfg_error:
	ast_config_destroy(cfg);
	return AST_MODULE_LOAD_DECLINE;

load_error_st:
	ast_sched_context_destroy(sched);
load_error:
	unload_module();
	ast_free(iflist);
	return AST_MODULE_LOAD_FAILURE;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_LOAD_ORDER, "Lantiq TAPI Telephony API Support",
	.load = load_module,
	.unload = unload_module,
	.load_pri = AST_MODPRI_CHANNEL_DRIVER
);

#include "Config.h"

#include "rk_ffplay.h"

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
#include "RKSoundTouch.h"
#endif

#ifdef CONFIG_AVFILTER
#undef CONFIG_AVFILTER
#endif

#if CONFIG_AVFILTER
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

//#include "cmdutils.h"

#include <assert.h>

#define printf RKLog
#undef LOG_TAG
#define LOG_TAG "RPLAYER"

const char program_name[] = "ffplay";
const int program_birth_year = 2003;

#define LOW_CACHE_SIZE 20480
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN 0.900
#define EXTERNAL_CLOCK_SPEED_MAX 1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
/*TODO: Now video player isn't necessary, so decrease the buffer size */

#define SAMPLE_ARRAY_SIZE 8

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

#include <pthread.h>

#ifdef CONFIG_AVFILTER
#undef CONFIG_AVFILTER
#endif

// set the default av_read_frame cache duration(s)
#define DEFAULT_CACHE_DURATION 1.0
#define MAX_CACHE_DURATION 10.0

#define fftime_to_milliseconds(ts) (av_rescale(ts, 1000, AV_TIME_BASE))
#define milliseconds_to_fftime(ms) (av_rescale(ms, AV_TIME_BASE, 1000))
static unsigned sws_flags = SWS_BICUBIC;
static pthread_mutex_t devices_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct MyAVPacketList
{
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue
{
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    SDL_mutex *mutex;
    SDL_cond *cond;
    int initialized;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct AudioParams
{
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Clock
{
    double pts;       /* clock base */
    double pts_drift; /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial; /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial; /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame
{
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;      /* presentation timestamp for the frame */
    double duration; /* estimated duration of the frame */
    int64_t pos;     /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;

typedef struct FrameQueue
{
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;

enum
{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

enum Status
{
    IDLE = 0,
    PREPARING,
    PREPARED,
    PAUSED,
    STOPPING,
    STOPPED,
    PLAYING,
    COMPLETE,
    ERROR,
};

typedef struct Decoder
{
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid;
} Decoder;

enum ShowMode
{
    SHOW_MODE_NONE = -1,
    SHOW_MODE_VIDEO = 0,
    SHOW_MODE_WAVES,
    SHOW_MODE_RDFT,
    SHOW_MODE_NB
} show_mode;

typedef struct UserOptions UserOptions;
struct UserOptions
{
    AVInputFormat *file_iformat;
    const char *input_filename;
    const char *window_title;
    int screen_width;
    int screen_height;
    int audio_disable;
    int video_disable;
    int subtitle_disable;
    const char *wanted_stream_spec[AVMEDIA_TYPE_NB];
    int seek_by_bytes;
    int display_disable;
    int startup_volume;
    int show_status;
    int av_sync_type;
    int64_t start_time;
    int64_t duration;
    int fast;
    int genpts;
    int lowres;
    int decoder_reorder_pts;
    int autoexit;
    int loop;
    int framedrop;
    int infinite_buffer;
    enum ShowMode show_mode;
    const char *audio_codec_name;
    const char *subtitle_codec_name;
    const char *video_codec_name;
    double rdftspeed;

    /*
       static int64_t cursor_last_shown;
       static int cursor_hidden = 0;
     */
#if CONFIG_AVFILTER
    const char **vfilters_list;
    int nb_vfilters;
    char *afilters;
#endif
    int autorotate;
    int find_stream_info;
    int default_width;
    int default_height;
};

struct RPlayer
{
    AVDictionary *codec_opts;
    AVDictionary *format_opts;

    SDL_Thread *read_tid;
    SDL_Thread *render_tid;
    SDL_Thread *stop_tid;
    SDL_TimerID notify_id;
    AVInputFormat *iformat;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic;
    int realtime;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    uint8_t *audio_buf2;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    unsigned int audio_buf2_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    struct AudioParams audio_src;
#if CONFIG_AVFILTER
    struct AudioParams audio_filter_src;
#endif
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;
    RDFTContext *rdft;
    int rdft_bits;
    FFTSample *rdft_data;
    int xpos;
    double last_vis_time;
    SDL_Texture *vis_texture;
    SDL_Texture *sub_texture;
    SDL_Texture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st;
    PacketQueue subtitleq;

    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    /* maximum duration of a frame - above this, we consider the jump a timestamp discontinuity */
    double max_frame_duration;
    struct SwsContext *img_convert_ctx;
    struct SwsContext *sub_convert_ctx;
    int eof;

    char *filename;
    int width, height, xleft, ytop;
    int step;

#if CONFIG_AVFILTER
    int vfilter_idx;
    AVFilterContext *in_video_filter;  // the first filter in the video chain
    AVFilterContext *out_video_filter; // the last filter in the video chain
    AVFilterContext *in_audio_filter;  // the first filter in the audio chain
    AVFilterContext *out_audio_filter; // the last filter in the audio chain
    AVFilterGraph *agraph;             // audio filter graph
#endif

    int last_video_stream, last_audio_stream, last_subtitle_stream;

    SDL_cond *continue_read_thread;

    RPlayerEventListener event_callback;
    void *event_cb_opaque;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_RendererInfo renderer_info;
    SDL_AudioDeviceID audio_dev;
    int loop_exit;
    int eof_notified;
    ;

    int64_t audio_callback_time;
    int is_full_screen;

    UserOptions user_options;
    enum ShowMode show_mode;
    SDL_mutex *ctrl_mutex;
    long total_duration_ms;
    char *media_tag;
    int buf_update_notified;
    int frame_consumed_notified;
#if BLOCK_PAUSE_MODE
    int block_paused;
    int block_paused_mode_enabled;
    int cur_block_paused_mode_sta;
    double cache_duration;
    double decode_frame_total_time;
    double last_decode_time;
    double last_start_timer_time;
    double last_audio_clock;
    double block_pause_start_time;
    double block_pause_end_time;
#endif
    enum Status status;
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
    struct tagSoundTouch *m_soundTouch;
#endif
    int recv_buf_size;
    int reconnect_at_eof;
    int reconnect_delay_max;
};

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts);

static void resize_window(RPlayer *is, int w, int h);
static void print_error(const char *filename, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}

static int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec)
{
    int ret = avformat_match_stream_specifier(s, st, spec);
    if (ret < 0)
        av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
    return ret;
}

static AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id, AVFormatContext *s, AVStream *st,
                                       AVCodec *codec)
{
    AVDictionary *ret = NULL;
    AVDictionaryEntry *t = NULL;
    int flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
    char prefix = 0;
    const AVClass *cc = avcodec_get_class();

    if (!codec)
        codec = s->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);

    switch (st->codecpar->codec_type)
    {
        case AVMEDIA_TYPE_VIDEO:
            prefix = 'v';
            flags |= AV_OPT_FLAG_VIDEO_PARAM;
            break;
        case AVMEDIA_TYPE_AUDIO:
            prefix = 'a';
            flags |= AV_OPT_FLAG_AUDIO_PARAM;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            prefix = 's';
            flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
            break;
        default:
            break;
    }

    while (!!(t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)))
    {
        char *p = strchr(t->key, ':');

        /* check stream specification in opt name */
        if (p)
            switch (check_stream_specifier(s, st, p + 1))
            {
                case 1:
                    *p = 0;
                    break;
                case 0:
                    continue;
                default:
                    return NULL;
            }

        if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec
            || (codec->priv_class && av_opt_find(&codec->priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)))
            av_dict_set(&ret, t->key, t->value, 0);
        else if (t->key[0] == prefix && av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))
            av_dict_set(&ret, t->key + 1, t->value, 0);

        if (p)
            *p = ':';
    }
    return ret;
}
void set_default_options(UserOptions *option)
{
    option->screen_width = 0;
    option->screen_height = 0;
    option->audio_disable = 0;
    option->video_disable = 0;
    option->subtitle_disable = 1;
    // option->wanted_stream_spec[AVMEDIA_TYPE_NB];
    memset(option->wanted_stream_spec, 0, sizeof(option->wanted_stream_spec));
    option->seek_by_bytes = -1;
#if ROKIDOS_FEATURES_HAS_GUI
    option->display_disable = 0;
#else
    option->display_disable = 1;
#endif
    option->startup_volume = 100;
    option->show_status = 1;
    option->av_sync_type = AV_SYNC_AUDIO_MASTER;
    option->start_time = AV_NOPTS_VALUE;
    option->duration = AV_NOPTS_VALUE;
    option->fast = 0;
    option->genpts = 0;
    option->lowres = 0;
    option->decoder_reorder_pts = -1;
    option->autoexit = 0;
    option->loop = 0;
    option->framedrop = -1;
    option->infinite_buffer = -1;
    option->show_mode = SHOW_MODE_NONE;
    option->audio_codec_name = NULL;
    option->subtitle_codec_name = NULL;
    option->video_codec_name = NULL;
    option->rdftspeed = 0.02;

    /*
       static int64_t cursor_last_shown;
       static int cursor_hidden = 0;
     */
#if CONFIG_AVFILTER
    option->vfilters_list = NULL;
    option->nb_vfilters = 0;
    option->afilters = NULL;
#endif
    option->autorotate = 1;
    option->find_stream_info = 1;
    option->default_width = 1920;
    option->default_height = 1080;
}

static AVPacket flush_pkt; //  drop falg for packet queue

void user_options_release(UserOptions *options)
{
    if (!options)
        return;
    if (options->audio_codec_name)
    {
        av_freep(&options->audio_codec_name);
    }
    if (options->video_codec_name)
    {
        av_freep(&options->video_codec_name);
    }
    if (options->subtitle_codec_name)
    {
        av_freep(&options->subtitle_codec_name);
    }
}

/* options specified by the user */

/* current context */

#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

static int video_output_create(RPlayer *is);
static void video_output_destroy(RPlayer *is);
static int render_thread(void *opaque);

typedef struct Message
{
    RPlayer *is;
    int msg;
    int ext1;
    int ext2;
    int from_thread;
} Message;

void notify_from_thread(RPlayer *is, int msg, int ext1, int ext2);

static const struct TextureFormatEntry
{
    enum AVPixelFormat format;
    int texture_fmt;
} sdl_texture_format_map[] = {
    { AV_PIX_FMT_RGB8, SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444, SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555, SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555, SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565, SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565, SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24, SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24, SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32, SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32, SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32, SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1, SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32, SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1, SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE, SDL_PIXELFORMAT_UNKNOWN },
};

#if CONFIG_AVFILTER
static int opt_add_vfilter(void *optctx, const char *opt, const char *arg)
{
    GROW_ARRAY(vfilters_list, nb_vfilters);
    vfilters_list[nb_vfilters - 1] = arg;
    return 0;
}
#endif

static inline int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1, enum AVSampleFormat fmt2,
                                 int64_t channel_count2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
    if (channel_count1 == 1 && channel_count2 == 1)
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    else
        return channel_count1 != channel_count2 || fmt1 != fmt2;
}

static inline int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
        return channel_layout;
    else
        return 0;
}

static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
    MyAVPacketList *pkt1;

    if (q->abort_request)
        return -1;

    pkt1 = av_malloc(sizeof(MyAVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if (pkt == &flush_pkt)
        q->serial++;
    pkt1->serial = q->serial;

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    q->duration += pkt1->pkt.duration;
    /* XXX: should duplicate packet data in DV case */
    SDL_CondSignal(q->cond);
    return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    int ret;

    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt);
    SDL_UnlockMutex(q->mutex);

    if (pkt != &flush_pkt && ret < 0)
        av_packet_unref(pkt);

    return ret;
}

static int packet_queue_put_nullpacket(PacketQueue *q, int stream_index)
{
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* packet queue handling */
static int packet_queue_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    if (!q->mutex)
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if (!q->cond)
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    q->initialized = 1;
    return 0;
}

static void packet_queue_flush(PacketQueue *q)
{
    MyAVPacketList *pkt, *pkt1;

    SDL_LockMutex(q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1)
    {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q)
{
    if (q->initialized)
    {
        packet_queue_flush(q);
        SDL_DestroyMutex(q->mutex);
        q->mutex = NULL;
        SDL_DestroyCond(q->cond);
        q->cond = NULL;
        q->initialized = 0;
    }
}

static void packet_queue_abort(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);

    q->abort_request = 1;

    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_start(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, &flush_pkt);
    SDL_UnlockMutex(q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
    MyAVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;)
    {
        if (q->abort_request)
        {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1)
        {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            q->duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            if (serial)
                *serial = pkt1->serial;
            av_free(pkt1);
            ret = 1;
            break;
        }
        else if (!block)
        {
            ret = 0;
            break;
        }
        else
        {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
// Sets the 'SoundTouch' object up according to input file sound format &
// command line parameters
static int setupSoundTouch(struct tagSoundTouch *pSoundTouch, int sampleRate, int channels)
{
    Soundtouch_SetSampleRate(pSoundTouch, sampleRate);
    Soundtouch_SetChannels(pSoundTouch, channels);

    Soundtouch_SetPitchSemiTones(pSoundTouch, 0);
    Soundtouch_SetRateChange(pSoundTouch, 0);

    Soundtouch_SetSetting(pSoundTouch, SETTING_USE_QUICKSEEK, 0);
    Soundtouch_SetSetting(pSoundTouch, SETTING_USE_AA_FILTER, 0);
    return 0;
}
#endif

static void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond)
{
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
}

static int decoder_decode_frame(RPlayer *is, Decoder *d, AVFrame *frame, AVSubtitle *sub)
{
    int ret = AVERROR(EAGAIN);

    for (;;)
    {
        AVPacket pkt;

        // comment get
        if (d->queue->serial == d->pkt_serial)
        {
            do
            {
                if (d->queue->abort_request)
                    return -1;

                switch (d->avctx->codec_type)
                {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0)
                        {
                            if (is->user_options.decoder_reorder_pts == -1)
                            {
                                frame->pts = frame->best_effort_timestamp;
                            }
                            else if (!is->user_options.decoder_reorder_pts)
                            {
                                frame->pts = frame->pkt_dts;
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0)
                        {
                            AVRational tb = (AVRational){ 1, frame->sample_rate };
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(d->avctx), tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE)
                            {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                    default:
                        break;
                }
                if (ret == AVERROR_EOF)
                {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0)
                    return 1;
            } while (ret != AVERROR(EAGAIN));
        }

        // COMMENT : clear flush frames and wait packet coming.
        do
        {
            if (d->queue->nb_packets == 0)
                SDL_CondSignal(d->empty_queue_cond);
            if (d->packet_pending)
            {
                av_packet_move_ref(&pkt, &d->pkt);
                d->packet_pending = 0;
            }
            else
            {
                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
                    return -1;
            }
        } while (d->queue->serial != d->pkt_serial);

        if (pkt.data == flush_pkt.data)
        {
            avcodec_flush_buffers(d->avctx);
            d->finished = 0;
            d->next_pts = d->start_pts;
            d->next_pts_tb = d->start_pts_tb;
        }
        else
        {
            if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE)
            {
                int got_frame = 0;
                ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &pkt);
                if (ret < 0)
                {
                    ret = AVERROR(EAGAIN);
                }
                else
                {
                    if (got_frame && !pkt.data)
                    {
                        d->packet_pending = 1;
                        av_packet_move_ref(&d->pkt, &pkt);
                    }
                    ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
                }
            }
            else
            {
                if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN))
                {
                    av_log(d->avctx, AV_LOG_ERROR,
                           "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                    d->packet_pending = 1;
                    av_packet_move_ref(&d->pkt, &pkt);
                }
            }
            av_packet_unref(&pkt);
        }
    }
}

static void decoder_destroy(Decoder *d)
{
    av_packet_unref(&d->pkt);
    avcodec_free_context(&d->avctx);
}

static void frame_queue_unref_item(Frame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
    int i;
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = SDL_CreateMutex()))
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    if (!(f->cond = SDL_CreateCond()))
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

static void frame_queue_destory(FrameQueue *f)
{
    int i;
    for (i = 0; i < f->max_size; i++)
    {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
    if (f->mutex)
    {
        SDL_DestroyMutex(f->mutex);
        f->mutex = NULL;
    }
    if (f->cond)
    {
        SDL_DestroyCond(f->cond);
        f->cond = NULL;
    }
}

static void frame_queue_signal(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static Frame *frame_queue_peek(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *frame_queue_peek_next(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame *frame_queue_peek_last(FrameQueue *f)
{
    return &f->queue[f->rindex];
}

static Frame *frame_queue_peek_writable(FrameQueue *f)
{
    /* wait until we have space to put a new frame */
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size && !f->pktq->abort_request)
    {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);

    if (f->pktq->abort_request)
        return NULL;

    return &f->queue[f->windex];
}

static Frame *frame_queue_peek_readable(FrameQueue *f)
{
    int result = 0;
    /* wait until we have a readable a new frame */
    SDL_LockMutex(f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->pktq->abort_request
           && (result = SDL_CondWaitTimeout(f->cond, f->mutex, 1000)) == 0)
    {
    }
    SDL_UnlockMutex(f->mutex);

    if (f->pktq->abort_request)
        return NULL;

    if (f->size - f->rindex_shown <= 0 && result != 0)
        return NULL;

    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void frame_queue_push(FrameQueue *f)
{
    if (++f->windex == f->max_size)
        f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

static void frame_queue_next(FrameQueue *f)
{
    if (f->keep_last && !f->rindex_shown)
    {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
        f->rindex = 0;
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

/* return the number of undisplayed frames in the queue */
static int frame_queue_nb_remaining(FrameQueue *f)
{
    return f->size - f->rindex_shown;
}

/* return last shown position */
static int64_t frame_queue_last_pos(FrameQueue *f)
{
    Frame *fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->pktq->serial)
        return fp->pos;
    else
        return -1;
}

static void decoder_abort(Decoder *d, FrameQueue *fq)
{
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    SDL_WaitThread(d->decoder_tid, NULL);
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}

static inline void fill_rectangle(RPlayer *is, int x, int y, int w, int h)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    if (w && h)
        SDL_RenderFillRect(is->renderer, &rect);
}

static int realloc_texture(RPlayer *is, SDL_Texture **texture, Uint32 new_format, int new_width, int new_height,
                           SDL_BlendMode blendmode, int init_texture)
{
    Uint32 format;
    int access, w, h;
    if (SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h
        || new_format != format)
    {
        void *pixels;
        int pitch;
        SDL_DestroyTexture(*texture);
        if (!(*texture
              = SDL_CreateTexture(is->renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height)))
            return -1;
        if (SDL_SetTextureBlendMode(*texture, blendmode) < 0)
            return -1;
        if (init_texture)
        {
            if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0)
                return -1;
            memset(pixels, 0, pitch * new_height);
            SDL_UnlockTexture(*texture);
        }
        av_log(NULL, AV_LOG_VERBOSE, "Created %dx%d texture with %s.\n", new_width, new_height,
               SDL_GetPixelFormatName(new_format));
    }
    return 0;
}

static void calculate_display_rect(SDL_Rect *rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height,
                                   int pic_width, int pic_height, AVRational pic_sar)
{
    float aspect_ratio;
    int width, height, x, y;

    if (pic_sar.num == 0)
        aspect_ratio = 0;
    else
        aspect_ratio = av_q2d(pic_sar);

    if (aspect_ratio <= 0.0)
        aspect_ratio = 1.0;
    aspect_ratio *= (float)pic_width / (float)pic_height;

    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    height = scr_height;
    width = lrint(height * aspect_ratio) & ~1;
    if (width > scr_width)
    {
        width = scr_width;
        height = lrint(width / aspect_ratio) & ~1;
    }
    x = (scr_width - width) / 2;
    y = (scr_height - height) / 2;
    rect->x = scr_xleft + x;
    rect->y = scr_ytop + y;
    rect->w = FFMAX(width, 1);
    rect->h = FFMAX(height, 1);
}

static void get_sdl_pix_fmt_and_blendmode(int format, Uint32 *sdl_pix_fmt, SDL_BlendMode *sdl_blendmode)
{
    int i;
    *sdl_blendmode = SDL_BLENDMODE_NONE;
    *sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    if (format == AV_PIX_FMT_RGB32 || format == AV_PIX_FMT_RGB32_1 || format == AV_PIX_FMT_BGR32
        || format == AV_PIX_FMT_BGR32_1)
        *sdl_blendmode = SDL_BLENDMODE_BLEND;
    for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++)
    {
        if (format == sdl_texture_format_map[i].format)
        {
            *sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
            return;
        }
    }
}

static int upload_texture(RPlayer *is, SDL_Texture **tex, AVFrame *frame, struct SwsContext **img_convert_ctx)
{
    int ret = 0;
    Uint32 sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    SDL_BlendMode sdl_blendmode = SDL_BLENDMODE_NONE;
    // get_sdl_pix_fmt_and_blendmode(frame->format, &sdl_pix_fmt, &sdl_blendmode);
    if (realloc_texture(is, tex, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt,
                        frame->width, frame->height, sdl_blendmode, 0)
        < 0)
        return -1;
    switch (sdl_pix_fmt)
    {
        case SDL_PIXELFORMAT_UNKNOWN:
            /* This should only happen if we are not using avfilter... */
            *img_convert_ctx
                = sws_getCachedContext(*img_convert_ctx, frame->width, frame->height, frame->format, frame->width,
                                       frame->height, AV_PIX_FMT_BGRA, sws_flags, NULL, NULL, NULL);
            if (*img_convert_ctx != NULL)
            {
                uint8_t *pixels[4];
                int pitch[4];
                if (!SDL_LockTexture(*tex, NULL, (void **)pixels, pitch))
                {
                    sws_scale(*img_convert_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height,
                              pixels, pitch);
                    SDL_UnlockTexture(*tex);
                }
            }
            else
            {
                av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
                ret = -1;
            }
            break;
        case SDL_PIXELFORMAT_IYUV:
            if (frame->linesize[0] > 0 && frame->linesize[1] > 0 && frame->linesize[2] > 0)
            {
                ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                                           frame->linesize[1], frame->data[2], frame->linesize[2]);
            }
            else if (frame->linesize[0] < 0 && frame->linesize[1] < 0 && frame->linesize[2] < 0)
            {
                ret = SDL_UpdateYUVTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                                           -frame->linesize[0],
                                           frame->data[1] + frame->linesize[1] * (AV_CEIL_RSHIFT(frame->height, 1) - 1),
                                           -frame->linesize[1],
                                           frame->data[2] + frame->linesize[2] * (AV_CEIL_RSHIFT(frame->height, 1) - 1),
                                           -frame->linesize[2]);
            }
            else
            {
                av_log(NULL, AV_LOG_ERROR, "Mixed negative and positive linesizes are not supported.\n");
                return -1;
            }
            break;
        default:
            if (frame->linesize[0] < 0)
            {
                ret = SDL_UpdateTexture(*tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                                        -frame->linesize[0]);
            }
            else
            {
                ret = SDL_UpdateTexture(*tex, NULL, frame->data[0], frame->linesize[0]);
            }
            break;
    }
    return ret;
}

static void video_image_display(RPlayer *is)
{
    Frame *vp;
    Frame *sp = NULL;
    SDL_Rect rect;

    vp = frame_queue_peek_last(&is->pictq);
    if (is->subtitle_st)
    {
        if (frame_queue_nb_remaining(&is->subpq) > 0)
        {
            sp = frame_queue_peek(&is->subpq);

            if (vp->pts >= sp->pts + ((float)sp->sub.start_display_time / 1000))
            {
                if (!sp->uploaded)
                {
                    uint8_t *pixels[4];
                    int pitch[4];
                    int i;
                    if (!sp->width || !sp->height)
                    {
                        sp->width = vp->width;
                        sp->height = vp->height;
                    }
                    if (realloc_texture(is, &is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height,
                                        SDL_BLENDMODE_BLEND, 1)
                        < 0)
                        return;

                    for (i = 0; i < sp->sub.num_rects; i++)
                    {
                        AVSubtitleRect *sub_rect = sp->sub.rects[i];

                        sub_rect->x = av_clip(sub_rect->x, 0, sp->width);
                        sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
                        sub_rect->w = av_clip(sub_rect->w, 0, sp->width - sub_rect->x);
                        sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

                        is->sub_convert_ctx
                            = sws_getCachedContext(is->sub_convert_ctx, sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                                                   sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA, 0, NULL, NULL, NULL);
                        if (!is->sub_convert_ctx)
                        {
                            av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
                            return;
                        }
                        if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)pixels, pitch))
                        {
                            sws_scale(is->sub_convert_ctx, (const uint8_t *const *)sub_rect->data, sub_rect->linesize,
                                      0, sub_rect->h, pixels, pitch);
                            SDL_UnlockTexture(is->sub_texture);
                        }
                    }
                    sp->uploaded = 1;
                }
            }
            else
                sp = NULL;
        }
    }

    // resize_window(is, vp->width, vp->height);
    calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);

    if (!vp->uploaded)
    {
        if (upload_texture(is, &is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
            return;
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

    SDL_RenderCopyEx(is->renderer, is->vid_texture, NULL, &rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : 0);
    if (sp)
    {
#if USE_ONEPASS_SUBTITLE_RENDER
        SDL_RenderCopy(is->renderer, is->sub_texture, NULL, &rect);
#else
        int i;
        double xratio = (double)rect.w / (double)sp->width;
        double yratio = (double)rect.h / (double)sp->height;
        for (i = 0; i < sp->sub.num_rects; i++)
        {
            SDL_Rect *sub_rect = (SDL_Rect *)sp->sub.rects[i];
            SDL_Rect target = { .x = rect.x + sub_rect->x * xratio,
                                .y = rect.y + sub_rect->y * yratio,
                                .w = sub_rect->w * xratio,
                                .h = sub_rect->h * yratio };
            SDL_RenderCopy(is->renderer, is->sub_texture, sub_rect, &target);
        }
#endif
    }
}

static inline int compute_mod(int a, int b)
{
    return a < 0 ? a % b + b : a % b;
}

static void video_audio_display(RPlayer *s)
{
    // printf("%s thread 0x%lx\n", __func__, (unsigned long)pthread_self());
    int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
    int ch, channels, h, h2;
    int64_t time_diff;
    int rdft_bits, nb_freq;

    for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
        ;
    nb_freq = 1 << (rdft_bits - 1);

    /* compute display index : center on currently output samples */
    channels = s->audio_tgt.channels;
    nb_display_channels = channels;
    if (!s->paused)
    {
        int data_used = s->show_mode == SHOW_MODE_WAVES ? s->width : (2 * nb_freq);
        n = 2 * channels;
        delay = s->audio_write_buf_size;
        delay /= n;

        /* to be more precise, we take into account the time spent since
           the last buffer computation */
        if (s->audio_callback_time)
        {
            time_diff = av_gettime_relative() - s->audio_callback_time;
            delay -= (time_diff * s->audio_tgt.freq) / 1000000;
        }

        delay += 2 * data_used;
        if (delay < data_used)
            delay = data_used;

        i_start = x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
        if (s->show_mode == SHOW_MODE_WAVES)
        {
            h = INT_MIN;
            for (i = 0; i < 1000; i += channels)
            {
                int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
                int a = s->sample_array[idx];
                int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
                int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
                int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
                int score = a - d;
                if (h < score && (b ^ c) < 0)
                {
                    h = score;
                    i_start = idx;
                }
            }
        }

        s->last_i_start = i_start;
    }
    else
    {
        i_start = s->last_i_start;
    }

    if (s->show_mode == SHOW_MODE_WAVES)
    {
        SDL_SetRenderDrawColor(s->renderer, 0, 0, 0, 255);

        /* total height for one channel */
        h = s->height / nb_display_channels;
        /* graph height / 2 */
        h2 = (h * 9) / 20;
        for (ch = 0; ch < nb_display_channels; ch++)
        {
            i = i_start + ch;
            y1 = s->ytop + ch * h + (h / 2); /* position of center line */
            for (x = 0; x < s->width; x++)
            {
                y = (s->sample_array[i] * h2) >> 15;
                if (y < 0)
                {
                    y = -y;
                    ys = y1 - y;
                }
                else
                {
                    ys = y1;
                }
                fill_rectangle(s, s->xleft + x, ys, 1, y);
                i += channels;
                if (i >= SAMPLE_ARRAY_SIZE)
                    i -= SAMPLE_ARRAY_SIZE;
            }
        }

        SDL_SetRenderDrawColor(s->renderer, 0, 0, 255, 255);

        for (ch = 1; ch < nb_display_channels; ch++)
        {
            y = s->ytop + ch * h;
            fill_rectangle(s, s->xleft, y, s->width, 1);
        }
    }
    else
    {
        if (realloc_texture(s, &s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height, SDL_BLENDMODE_NONE, 1)
            < 0)
            return;

        nb_display_channels = FFMIN(nb_display_channels, 2);
        if (rdft_bits != s->rdft_bits)
        {
            av_rdft_end(s->rdft);
            av_free(s->rdft_data);
            s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
            s->rdft_bits = rdft_bits;
            s->rdft_data = av_malloc_array(nb_freq, 4 * sizeof(*s->rdft_data));
        }
        if (!s->rdft || !s->rdft_data)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
            s->show_mode = SHOW_MODE_WAVES;
        }
        else
        {
            FFTSample *data[2];
            SDL_Rect rect = { .x = s->xpos, .y = 0, .w = 1, .h = s->height };
            uint32_t *pixels;
            int pitch;
            for (ch = 0; ch < nb_display_channels; ch++)
            {
                data[ch] = s->rdft_data + 2 * nb_freq * ch;
                i = i_start + ch;
                for (x = 0; x < 2 * nb_freq; x++)
                {
                    double w = (x - nb_freq) * (1.0 / nb_freq);
                    data[ch][x] = s->sample_array[i] * (1.0 - w * w);
                    i += channels;
                    if (i >= SAMPLE_ARRAY_SIZE)
                        i -= SAMPLE_ARRAY_SIZE;
                }
                av_rdft_calc(s->rdft, data[ch]);
            }
            /* Least efficient way to do this, we should of course
             * directly access it but it is more than fast enough. */
            if (!SDL_LockTexture(s->vis_texture, &rect, (void **)&pixels, &pitch))
            {
                pitch >>= 2;
                pixels += pitch * s->height;
                for (y = 0; y < s->height; y++)
                {
                    double w = 1 / sqrt(nb_freq);
                    int a = sqrt(
                        w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
                    int b = (nb_display_channels == 2) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1])) : a;
                    a = FFMIN(a, 255);
                    b = FFMIN(b, 255);
                    pixels -= pitch;
                    *pixels = (a << 16) + (b << 8) + ((a + b) >> 1);
                }
                SDL_UnlockTexture(s->vis_texture);
            }
            SDL_RenderCopy(s->renderer, s->vis_texture, NULL, NULL);
        }
        if (!s->paused)
            s->xpos++;
        if (s->xpos >= s->width)
            s->xpos = s->xleft;
    }
}

static void stream_component_close(RPlayer *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;
    if (ic)
    {
        if (stream_index < 0 || stream_index >= ic->nb_streams)
            return;
        codecpar = ic->streams[stream_index]->codecpar;

        switch (codecpar->codec_type)
        {
            case AVMEDIA_TYPE_AUDIO:
                decoder_abort(&is->auddec, &is->sampq);
		printf("closeAudio_Devic =%d begin\n ", is->audio_dev);
                SDL_CloseAudioDevice(is->audio_dev);
                decoder_destroy(&is->auddec);
                swr_free(&is->swr_ctx);
                av_freep(&is->audio_buf1);
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
                av_freep(&is->audio_buf2);
#endif
                is->audio_buf1_size = 0;
                is->audio_buf2_size = 0;
                is->audio_buf = NULL;

                if (is->rdft)
                {
                    av_rdft_end(is->rdft);
                    av_freep(&is->rdft_data);
                    is->rdft = NULL;
                    is->rdft_bits = 0;
                }
                break;
            case AVMEDIA_TYPE_VIDEO:
                decoder_abort(&is->viddec, &is->pictq);
                decoder_destroy(&is->viddec);
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                decoder_abort(&is->subdec, &is->subpq);
                decoder_destroy(&is->subdec);
                break;
            default:
                break;
        }

        ic->streams[stream_index]->discard = AVDISCARD_ALL;
        switch (codecpar->codec_type)
        {
            case AVMEDIA_TYPE_AUDIO:
                is->audio_st = NULL;
                is->audio_stream = -1;
                break;
            case AVMEDIA_TYPE_VIDEO:
                is->video_st = NULL;
                is->video_stream = -1;
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                is->subtitle_st = NULL;
                is->subtitle_stream = -1;
                break;
            default:
                break;
        }
    }
}

static void stream_close(RPlayer *is)
{
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
    if (is->read_tid)
    {
        SDL_WaitThread(is->read_tid, NULL);
        printf("%s: wait read_thread OK!\n", __func__);
        is->read_tid = 0;
    }

    if (is->render_tid)
    {
        SDL_WaitThread(is->render_tid, NULL);
        is->render_tid = 0;
    }

    /* close each stream */
    if (is->audio_stream >= 0)
    {
        stream_component_close(is, is->audio_stream);
        is->audio_stream = -1;
    }
    if (is->video_stream >= 0)
    {
        stream_component_close(is, is->video_stream);
        is->video_stream = -1;
    }
    if (is->subtitle_stream >= 0)
    {
        stream_component_close(is, is->subtitle_stream);
        is->subtitle_stream = -1;
    }
    if (is->ic)
    {
        avformat_close_input(&is->ic);
    }

    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);
    packet_queue_destroy(&is->subtitleq);

    /* free all pictures */
    frame_queue_destory(&is->pictq);
    frame_queue_destory(&is->sampq);
    frame_queue_destory(&is->subpq);
    if (is->continue_read_thread)
    {
        SDL_DestroyCond(is->continue_read_thread);
        is->continue_read_thread = NULL;
    }
    if (is->img_convert_ctx)
    {
        sws_freeContext(is->img_convert_ctx);
        is->img_convert_ctx = NULL;
    }
    if (is->sub_convert_ctx)
    {
        sws_freeContext(is->sub_convert_ctx);
        is->sub_convert_ctx = NULL;
    }
    if (is->filename)
    {
        av_free(is->filename);
        is->filename = NULL;
    }
    if (is->vis_texture)
    {
        SDL_DestroyTexture(is->vis_texture);
        is->vis_texture = NULL;
    }
    if (is->vid_texture)
    {
        SDL_DestroyTexture(is->vid_texture);
        is->vid_texture = NULL;
    }
    if (is->sub_texture)
    {
        SDL_DestroyTexture(is->sub_texture);
        is->sub_texture = NULL;
    }
    is->audclk.paused = is->vidclk.paused = is->extclk.paused = 1;
    is->paused = 1;
    is->loop_exit = 1;
    is->user_options.loop = 0;
    is->seek_pos = 0;
#if BLOCK_PAUSE_MODE
    is->block_paused = 0;
    is->decode_frame_total_time = 0.0;
    is->last_decode_time = 0.0;
    is->last_start_timer_time = 0.0;
    is->last_audio_clock = 0.0;
    is->block_pause_start_time = 0.0;
    is->block_pause_end_time = 0.0;
#endif
    printf("%s over!",  __func__);
}

static void set_default_window_size(RPlayer *is, int width, int height, AVRational sar)
{
    SDL_Rect rect;
    calculate_display_rect(&rect, 0, 0, INT_MAX, height, width, height, sar);
    is->user_options.default_width = rect.w;
    is->user_options.default_height = rect.h;
}

static int video_open(RPlayer *is)
{
    int w, h;
#if 1
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    w = DM.w;
    h = DM.h;

    printf("### screen width %d screen height %d\n", w, h);
#else
    if (is->user_options.screen_width)
    {
        w = is->user_options.screen_width;
        h = is->user_options.screen_height;
    }
    else
    {
        w = is->user_options.default_width;
        h = is->user_options.default_height;
    }
#endif

    SDL_SetWindowTitle(is->window, is->filename);

    SDL_SetWindowSize(is->window, w, h);
    SDL_SetWindowPosition(is->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (is->is_full_screen)
        SDL_SetWindowFullscreen(is->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_ShowWindow(is->window);

    is->width = w;
    is->height = h;

    return 0;
}

static void resize_window(RPlayer *is, int w, int h)
{
    if (w != is->width && h != is->height)
    {
        SDL_SetWindowSize(is->window, w, h);
        SDL_SetWindowPosition(is->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        if (is->is_full_screen)
            SDL_SetWindowFullscreen(is->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowWindow(is->window);

        is->width = w;
        is->height = h;
    }
}

/* display the current picture, if any */
static void video_display(RPlayer *is)
{
    if (!is->width)
        video_open(is);

    SDL_SetRenderDrawColor(is->renderer, 225, 225, 225, 255);
    SDL_RenderClear(is->renderer);
    if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
        video_audio_display(is);
    else if (is->video_st)
        video_image_display(is);
    SDL_RenderPresent(is->renderer);
}

static double get_clock(Clock *c)
{
    if (*c->queue_serial != c->serial)
        return NAN;
    if (c->paused)
    {
        return c->pts;
    }
    else
    {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
    }
}

static void set_clock_at(Clock *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

static void set_clock(Clock *c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void set_clock_speed(Clock *c, double speed)
{
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}

static void init_clock(Clock *c, int *queue_serial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

static void sync_clock_to_slave(Clock *c, Clock *slave)
{
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->serial);
}

static int get_master_sync_type(RPlayer *is)
{
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER)
    {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    }
    else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER)
    {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    }
    else
    {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

/* get the current master clock value */
static double get_master_clock(RPlayer *is)
{
    double val;

    switch (get_master_sync_type(is))
    {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->vidclk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->audclk);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}

static void check_external_clock_speed(RPlayer *is)
{
    if ((is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)
        || (is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES))
    {
        set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
    }
    else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)
             && (is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES))
    {
        set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
    }
    else
    {
        double speed = is->extclk.speed;
        if (speed != 1.0)
            set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
    }
}

/* seek in the stream */
static void stream_seek(RPlayer *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (!is->seek_req)
    {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        SDL_CondSignal(is->continue_read_thread);
#if BLOCK_PAUSE_MODE
        if (is->block_paused)
        {
            is->block_paused = 0;
            notify_from_thread(is, MEDIA_BLOCK_PAUSE_MODE, BLOCK_PAUSE_OFF, STREAM_SEEK);
            printf("Stream start seeking, and quit block paused mode!");
        }
        is->decode_frame_total_time = 0.0f;
        is->last_start_timer_time = av_gettime_relative() / 1000000.0;
#endif
    }
}

/* pause or resume the video */
static void stream_toggle_pause(RPlayer *is, int pasue_on)
{
    if (is->paused)
    {
        printf("%s begin\n media is paused\n",__func__);
        is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS))
        {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }
    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
    if (is->paused)
    {
        // is->frame_consumed_notified = 0;
    }
#if BLOCK_PAUSE_MODE
    else
    {
        if (is->block_paused)
        {
            is->block_paused = 0;
            notify_from_thread(is, MEDIA_BLOCK_PAUSE_MODE, BLOCK_PAUSE_OFF, STREAM_RESUME);
            printf("Stream resume and quit block paused mode!");
        }
        is->decode_frame_total_time = 0.0f;
        is->last_start_timer_time = av_gettime_relative() / 1000000.0;
    }
#endif
    printf("%s is over\n",__func__);
}

static void toggle_pause(RPlayer *is, int pause_on)
{
    stream_toggle_pause(is, pause_on);
    is->step = 0;
}

static void toggle_mute(RPlayer *is)
{
    is->muted = !is->muted;
}

static void update_volume(RPlayer *is, int sign, double step)
{
    double volume_level
        = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
    int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0));
    is->audio_volume
        = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME);
}

static void step_to_next_frame(RPlayer *is)
{
    /* if the stream is paused unpause it, then step */
    // fix :cancle if the stream is paused unpause it
    if (is->paused)
        set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->step = 1;
}

static double compute_target_delay(double delay, RPlayer *is)
{
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)
    {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&is->vidclk) - get_master_clock(is);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < is->max_frame_duration)
        {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }

    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);

    return delay;
}

static double vp_duration(RPlayer *is, Frame *vp, Frame *nextvp)
{
    if (vp->serial == nextvp->serial)
    {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
            return vp->duration;
        else
            return duration;
    }
    else
    {
        return 0.0;
    }
}

static void update_video_pts(RPlayer *is, double pts, int64_t pos, int serial)
{
    /* update current video pts */
    set_clock(&is->vidclk, pts, serial);
    sync_clock_to_slave(&is->extclk, &is->vidclk);
}

/* called to display each frame */
static void video_refresh(void *opaque, double *remaining_time)
{
    RPlayer *is = opaque;
    double time;

    Frame *sp, *sp2;

    if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
        check_external_clock_speed(is);

    if (!is->user_options.display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st)
    {
        time = av_gettime_relative() / 1000000.0;
        if (is->force_refresh || is->last_vis_time + is->user_options.rdftspeed < time)
        {
            video_display(is);
            is->last_vis_time = time;
        }
        *remaining_time = FFMIN(*remaining_time, is->last_vis_time + is->user_options.rdftspeed - time);
    }

    if (is->video_st)
    {
    retry:
        if (frame_queue_nb_remaining(&is->pictq) == 0)
        {
            // nothing to do, no picture to display in the queue
        }
        else
        {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;

            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->pictq);
            vp = frame_queue_peek(&is->pictq);

            if (vp->serial != is->videoq.serial)
            {
                frame_queue_next(&is->pictq);
                goto retry;
            }

            if (lastvp->serial != vp->serial)
                is->frame_timer = av_gettime_relative() / 1000000.0;

            if (is->paused)
                goto display;

            /* compute nominal last_duration */
            last_duration = vp_duration(is, lastvp, vp);
            delay = compute_target_delay(last_duration, is);

            time = av_gettime_relative() / 1000000.0;
            if (time < is->frame_timer + delay)
            {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                goto display;
            }

            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->frame_timer = time;

            SDL_LockMutex(is->pictq.mutex);
            if (!isnan(vp->pts))
                update_video_pts(is, vp->pts, vp->pos, vp->serial);
            SDL_UnlockMutex(is->pictq.mutex);

            if (frame_queue_nb_remaining(&is->pictq) > 1)
            {
                Frame *nextvp = frame_queue_peek_next(&is->pictq);
                duration = vp_duration(is, vp, nextvp);
                if (!is->step
                    && (is->user_options.framedrop > 0
                        || (is->user_options.framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER))
                    && time > is->frame_timer + duration)
                {
                    is->frame_drops_late++;
                    frame_queue_next(&is->pictq);
                    goto retry;
                }
            }

            if (is->subtitle_st)
            {
                while (frame_queue_nb_remaining(&is->subpq) > 0)
                {
                    sp = frame_queue_peek(&is->subpq);

                    if (frame_queue_nb_remaining(&is->subpq) > 1)
                        sp2 = frame_queue_peek_next(&is->subpq);
                    else
                        sp2 = NULL;

                    if (sp->serial != is->subtitleq.serial
                        || (is->vidclk.pts > (sp->pts + ((float)sp->sub.end_display_time / 1000)))
                        || (sp2 && is->vidclk.pts > (sp2->pts + ((float)sp2->sub.start_display_time / 1000))))
                    {
                        if (sp->uploaded)
                        {
                            int i;
                            for (i = 0; i < sp->sub.num_rects; i++)
                            {
                                AVSubtitleRect *sub_rect = sp->sub.rects[i];
                                uint8_t *pixels;
                                int pitch, j;

                                if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)&pixels, &pitch))
                                {
                                    for (j = 0; j < sub_rect->h; j++, pixels += pitch)
                                        memset(pixels, 0, sub_rect->w << 2);
                                    SDL_UnlockTexture(is->sub_texture);
                                }
                            }
                        }
                        frame_queue_next(&is->subpq);
                    }
                    else
                    {
                        break;
                    }
                }
            }

            frame_queue_next(&is->pictq);
            is->force_refresh = 1;

            if (is->step && !is->paused)
                stream_toggle_pause(is, 1);
        }
    display:
        /* display picture */
        if (!is->user_options.display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO
            && is->pictq.rindex_shown)
            video_display(is);
    }
    is->force_refresh = 0;
    if (is->user_options.show_status)
    {
        static int64_t last_time;
        int64_t cur_time;
        int aqsize, vqsize, sqsize;
        double av_diff;

        cur_time = av_gettime_relative();
        if (!last_time || (cur_time - last_time) >= 30000)
        {
            aqsize = 0;
            vqsize = 0;
            sqsize = 0;
            if (is->audio_st)
                aqsize = is->audioq.size;
            if (is->video_st)
                vqsize = is->videoq.size;
            if (is->subtitle_st)
                sqsize = is->subtitleq.size;
            av_diff = 0;
            if (is->audio_st && is->video_st)
                av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
            else if (is->video_st)
                av_diff = get_master_clock(is) - get_clock(&is->vidclk);
            else if (is->audio_st)
                av_diff = get_master_clock(is) - get_clock(&is->audclk);
            av_log(NULL, AV_LOG_INFO, "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%" PRId64 "/%" PRId64 "   \r",
                   get_master_clock(is),
                   (is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
                   av_diff, is->frame_drops_early + is->frame_drops_late, aqsize / 1024, vqsize / 1024, sqsize,
                   is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0,
                   is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0);
            fflush(stdout);
            last_time = cur_time;
        }
    }
}

static int queue_picture(RPlayer *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *vp;

#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(src_frame->pict_type), pts);
#endif

    if (!(vp = frame_queue_peek_writable(&is->pictq)))
        return -1;

    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    set_default_window_size(is, vp->width, vp->height, vp->sar);

    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->pictq);
    return 0;
}

static int get_video_frame(RPlayer *is, AVFrame *frame)
{
    int got_picture;

    if ((got_picture = decoder_decode_frame(is, &is->viddec, frame, NULL)) < 0)
        return -1;

    if (got_picture)
    {
        double dpts = NAN;

        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(is->video_st->time_base) * frame->pts;

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

        if (is->user_options.framedrop > 0
            || (is->user_options.framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER))
        {
            if (frame->pts != AV_NOPTS_VALUE)
            {
                double diff = dpts - get_master_clock(is);
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD && diff - is->frame_last_filter_delay < 0
                    && is->viddec.pkt_serial == is->vidclk.serial && is->videoq.nb_packets)
                {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}

#if CONFIG_AVFILTER
static int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx,
                                 AVFilterContext *sink_ctx)
{
    int ret, i;
    int nb_filters = graph->nb_filters;
    AVFilterInOut *outputs = NULL, *inputs = NULL;

    if (filtergraph)
    {
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();
        if (!outputs || !inputs)
        {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        outputs->name = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
            goto fail;
    }
    else
    {
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
            goto fail;
    }

    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
    for (i = 0; i < graph->nb_filters - nb_filters; i++)
        FFSWAP(AVFilterContext *, graph->filters[i], graph->filters[i + nb_filters]);

    ret = avfilter_graph_config(graph, NULL);
fail:
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    return ret;
}

static int configure_video_filters(AVFilterGraph *graph, RPlayer *is, const char *vfilters, AVFrame *frame)
{
    enum AVPixelFormat pix_fmts[FF_ARRAY_ELEMS(sdl_texture_format_map)];
    char sws_flags_str[512] = "";
    char buffersrc_args[256];
    int ret;
    AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
    AVCodecParameters *codecpar = is->video_st->codecpar;
    AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
    AVDictionaryEntry *e = NULL;
    int nb_pix_fmts = 0;
    int i, j;

    for (i = 0; i < is->renderer_info.num_texture_formats; i++)
    {
        for (j = 0; j < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; j++)
        {
            if (is->renderer_info.texture_formats[i] == sdl_texture_format_map[j].texture_fmt)
            {
                pix_fmts[nb_pix_fmts++] = sdl_texture_format_map[j].format;
                break;
            }
        }
    }
    pix_fmts[nb_pix_fmts] = AV_PIX_FMT_NONE;

    while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX)))
    {
        if (!strcmp(e->key, "sws_flags"))
        {
            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
        }
        else
            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
    }
    if (strlen(sws_flags_str))
        sws_flags_str[strlen(sws_flags_str) - 1] = '\0';

    graph->scale_sws_opts = av_strdup(sws_flags_str);

    snprintf(buffersrc_args, sizeof(buffersrc_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width, frame->height, frame->format, is->video_st->time_base.num, is->video_st->time_base.den,
             codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
    if (fr.num && fr.den)
        av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

    if ((ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"), "ffplay_buffer", buffersrc_args,
                                            NULL, graph))
        < 0)
        goto fail;

    ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink") "ffplay_buffersink", NULL, NULL,
                                       graph);
    if (ret < 0)
        goto fail;

    if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto fail;

    last_filter = filt_out;

/* Note: this macro adds a filter before the lastly added filter, so the
 * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg)                                                                                       \
    do                                                                                                               \
    {                                                                                                                \
        AVFilterContext *filt_ctx;                                                                                   \
                                                                                                                     \
        ret = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name), "ffplay_" name, arg, NULL, graph); \
        if (ret < 0)                                                                                                 \
            goto fail;                                                                                               \
                                                                                                                     \
        ret = avfilter_link(filt_ctx, 0, last_filter, 0);                                                            \
        if (ret < 0)                                                                                                 \
            goto fail;                                                                                               \
                                                                                                                     \
        last_filter = filt_ctx;                                                                                      \
    } while (0)

    if (autorotate)
    {
        double theta = get_rotation(is->video_st);

        if (fabs(theta - 90) < 1.0)
        {
            INSERT_FILT("transpose", "clock");
        }
        else if (fabs(theta - 180) < 1.0)
        {
            INSERT_FILT("hflip", NULL);
            INSERT_FILT("vflip", NULL);
        }
        else if (fabs(theta - 270) < 1.0)
        {
            INSERT_FILT("transpose", "cclock");
        }
        else if (fabs(theta) > 1.0)
        {
            char rotate_buf[64];
            snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
            INSERT_FILT("rotate", rotate_buf);
        }
    }

    if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0)
        goto fail;

    is->in_video_filter = filt_src;
    is->out_video_filter = filt_out;

fail:
    return ret;
}

static int configure_audio_filters(RPlayer *is, const char *afilters, int force_output_format)
{
    static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    int sample_rates[2] = { 0, -1 };
    int64_t channel_layouts[2] = { 0, -1 };
    int channels[2] = { 0, -1 };
    AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
    char aresample_swr_opts[512] = "";
    AVDictionaryEntry *e = NULL;
    char asrc_args[256];
    int ret;

    avfilter_graph_free(&is->agraph);
    if (!(is->agraph = avfilter_graph_alloc()))
        return AVERROR(ENOMEM);

    while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
        av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
    if (strlen(aresample_swr_opts))
        aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
    av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

    ret = snprintf(asrc_args, sizeof(asrc_args), "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                   is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
                   is->audio_filter_src.channels, 1, is->audio_filter_src.freq);
    if (is->audio_filter_src.channel_layout)
        snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%" PRIx64,
                 is->audio_filter_src.channel_layout);

    ret = avfilter_graph_create_filter(&filt_asrc, avfilter_get_by_name("abuffer"), "ffplay_abuffer", asrc_args, NULL,
                                       is->agraph);
    if (ret < 0)
        goto end;

    ret = avfilter_graph_create_filter(&filt_asink, avfilter_get_by_name("abuffersink"), "ffplay_abuffersink", NULL,
                                       NULL, is->agraph);
    if (ret < 0)
        goto end;

    if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN))
        < 0)
        goto end;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;

    if (force_output_format)
    {
        channel_layouts[0] = is->audio_tgt.channel_layout;
        channels[0] = is->audio_tgt.channels;
        sample_rates[0] = is->audio_tgt.freq;
        if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
    }

    if ((ret = configure_filtergraph(is->agraph, afilters, filt_asrc, filt_asink)) < 0)
        goto end;

    is->in_audio_filter = filt_asrc;
    is->out_audio_filter = filt_asink;

end:
    if (ret < 0)
        avfilter_graph_free(&is->agraph);
    return ret;
}
#endif /* CONFIG_AVFILTER */

static int audio_thread(void *arg)
{
    RPlayer *is = arg;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
#if CONFIG_AVFILTER
    int last_serial = -1;
    int64_t dec_channel_layout;
    int reconfigure;
#endif
    int got_frame = 0;
    AVRational tb;
    int ret = 0;

    if (!frame)
        return AVERROR(ENOMEM);

    do
    {
        if ((got_frame = decoder_decode_frame(is, &is->auddec, frame, NULL)) < 0)
            goto the_end;

        if (got_frame)
        {
            tb = (AVRational){ 1, frame->sample_rate };

#if CONFIG_AVFILTER
            dec_channel_layout = get_valid_channel_layout(frame->channel_layout, frame->channels);

            reconfigure = cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels, frame->format,
                                         frame->channels)
                          || is->audio_filter_src.channel_layout != dec_channel_layout
                          || is->audio_filter_src.freq != frame->sample_rate || is->auddec.pkt_serial != last_serial;

            if (reconfigure)
            {
                char buf1[1024], buf2[1024];
                av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
                av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
                av_log(NULL, AV_LOG_DEBUG,
                       "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s "
                       "layout:%s serial:%d\n",
                       is->audio_filter_src.freq, is->audio_filter_src.channels,
                       av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial, frame->sample_rate,
                       frame->channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

                is->audio_filter_src.fmt = frame->format;
                is->audio_filter_src.channels = frame->channels;
                is->audio_filter_src.channel_layout = dec_channel_layout;
                is->audio_filter_src.freq = frame->sample_rate;
                last_serial = is->auddec.pkt_serial;

                if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
                    goto the_end;
            }

            if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
                goto the_end;

            while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0)
            {
                tb = av_buffersink_get_time_base(is->out_audio_filter);
#endif
                if (!(af = frame_queue_peek_writable(&is->sampq)))
                    goto the_end;

                af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
                af->pos = frame->pkt_pos;
                af->serial = is->auddec.pkt_serial;
                af->duration = av_q2d((AVRational){ frame->nb_samples, frame->sample_rate });

                av_frame_move_ref(af->frame, frame);
                frame_queue_push(&is->sampq);

#if CONFIG_AVFILTER
                if (is->audioq.serial != is->auddec.pkt_serial)
                    break;
            }
            if (ret == AVERROR_EOF)
                is->auddec.finished = is->auddec.pkt_serial;
#endif
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
#if CONFIG_AVFILTER
    avfilter_graph_free(&is->agraph);
#endif
    av_frame_free(&frame);
    return ret;
}

static int decoder_start(Decoder *d, int (*fn)(void *), void *arg)
{
    packet_queue_start(d->queue);
    d->decoder_tid = SDL_CreateThread(fn, "decoder", arg);
    if (!d->decoder_tid)
    {
        av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int video_thread(void *arg)
{
    RPlayer *is = arg;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

#if CONFIG_AVFILTER
    AVFilterGraph *graph = avfilter_graph_alloc();
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = -2;
    int last_serial = -1;
    int last_vfilter_idx = 0;
    if (!graph)
    {
        av_frame_free(&frame);
        return AVERROR(ENOMEM);
    }

#endif

    if (!frame)
    {
#if CONFIG_AVFILTER
        avfilter_graph_free(&graph);
#endif
        return AVERROR(ENOMEM);
    }

    for (;;)
    {
        ret = get_video_frame(is, frame);
        if (ret < 0)
            goto the_end;
        if (!ret)
            continue;

#if CONFIG_AVFILTER
        if (last_w != frame->width || last_h != frame->height || last_format != frame->format
            || last_serial != is->viddec.pkt_serial || last_vfilter_idx != is->vfilter_idx)
        {
            av_log(NULL, AV_LOG_DEBUG,
                   "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
                   last_w, last_h, (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
                   frame->width, frame->height, (const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"),
                   is->viddec.pkt_serial);
            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if ((ret = configure_video_filters(graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL, frame))
                < 0)
            {
                SDL_Event event;
                event.type = FF_QUIT_EVENT;
                event.user.data1 = is;
                SDL_PushEvent(&event);
                goto the_end;
            }
            filt_in = is->in_video_filter;
            filt_out = is->out_video_filter;
            last_w = frame->width;
            last_h = frame->height;
            last_format = frame->format;
            last_serial = is->viddec.pkt_serial;
            last_vfilter_idx = is->vfilter_idx;
            frame_rate = av_buffersink_get_frame_rate(filt_out);
        }

        ret = av_buffersrc_add_frame(filt_in, frame);
        if (ret < 0)
            goto the_end;

        while (ret >= 0)
        {
            is->frame_last_returned_time = av_gettime_relative() / 1000000.0;
            ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
            if (ret < 0)
            {
                if (ret == AVERROR_EOF)
                    is->viddec.finished = is->viddec.pkt_serial;
                ret = 0;
                break;
            }

            is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
                is->frame_last_filter_delay = 0;
            tb = av_buffersink_get_time_base(filt_out);
#endif
            duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){ frame_rate.den, frame_rate.num }) : 0);
            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            ret = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
            av_frame_unref(frame);
#if CONFIG_AVFILTER
        }
#endif

        if (ret < 0)
            goto the_end;
    }
the_end:
#if CONFIG_AVFILTER
    avfilter_graph_free(&graph);
#endif
    av_frame_free(&frame);
    return 0;
}

static int subtitle_thread(void *arg)
{
    RPlayer *is = arg;
    Frame *sp;
    int got_subtitle;
    double pts;

    for (;;)
    {
        if (!(sp = frame_queue_peek_writable(&is->subpq)))
            return 0;

        if ((got_subtitle = decoder_decode_frame(is, &is->subdec, NULL, &sp->sub)) < 0)
            break;

        pts = 0;

        if (got_subtitle && sp->sub.format == 0)
        {
            if (sp->sub.pts != AV_NOPTS_VALUE)
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            sp->pts = pts;
            sp->serial = is->subdec.pkt_serial;
            sp->width = is->subdec.avctx->width;
            sp->height = is->subdec.avctx->height;
            sp->uploaded = 0;

            /* now we can update the picture count */
            frame_queue_push(&is->subpq);
        }
        else if (got_subtitle)
        {
            avsubtitle_free(&sp->sub);
        }
    }
    return 0;
}

/* copy samples for viewing in editor window */
static void update_sample_display(RPlayer *is, short *samples, int samples_size)
{
    int size, len;

    size = samples_size / sizeof(short);
    while (size > 0)
    {
        len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
        if (len > size)
            len = size;
        memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
        samples += len;
        is->sample_array_index += len;
        if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
            is->sample_array_index = 0;
        size -= len;
    }
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
static int synchronize_audio(RPlayer *is, int nb_samples)
{
    int wanted_nb_samples = nb_samples;

    /* if not master, then we try to remove or add samples to correct the clock */
    if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER)
    {
        double diff, avg_diff;
        int min_nb_samples, max_nb_samples;

        diff = get_clock(&is->audclk) - get_master_clock(is);

        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
        {
            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
            {
                /* not enough measures to have a correct estimate */
                is->audio_diff_avg_count++;
            }
            else
            {
                /* estimate the A-V difference */
                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

                if (fabs(avg_diff) >= is->audio_diff_threshold)
                {
                    wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
                    min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
                }
                av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n", diff, avg_diff,
                       wanted_nb_samples - nb_samples, is->audio_clock, is->audio_diff_threshold);
            }
        }
        else
        {
            /* too big difference : may be initial PTS errors, so
               reset A-V filter */
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum = 0;
        }
    }

    return wanted_nb_samples;
}
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
#define SOUNDTOUCH_SAMPLE_BUFFERSIZE 10000

static uint SoundTouch_BufferHandle(RPlayer *is, bool isFlush, int inBuffers)
{
    uint sum_size = 0;
    CALLC_SAMPLETYPE sampleBuffer[SOUNDTOUCH_SAMPLE_BUFFERSIZE];
    uint nSamples = 0, nBuffersize;
    int nChannels = is->audio_src.channels;
    if (isFlush)
    {
        if (Soundtouch_GetState(is->m_soundTouch) == SOUNDTOUCH_PLAYING)
        {
            Soundtouch_Flush(is->m_soundTouch);
            Soundtouch_SetState(is->m_soundTouch, SOUNDTOUCH_OVER);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (Soundtouch_GetState(is->m_soundTouch) == SOUNDTOUCH_INIT)
        {
            setupSoundTouch(is->m_soundTouch, is->audio_src.freq, is->audio_src.channels);
        }
        nSamples = (inBuffers >> 1) / nChannels;
        Soundtouch_PutSamples(is->m_soundTouch, (CALLC_SAMPLETYPE *)is->audio_buf, nSamples);
        Soundtouch_SetState(is->m_soundTouch, SOUNDTOUCH_PLAYING);
    }
    nSamples = Soundtouch_NumSamples(is->m_soundTouch) * nChannels * sizeof(CALLC_SAMPLETYPE);
    if (isFlush)
    {
        nSamples += inBuffers;
    }
    av_fast_malloc(&is->audio_buf2, &is->audio_buf2_size, nSamples);
    do
    {
        nSamples = Soundtouch_ReceiveSamples(is->m_soundTouch, sampleBuffer,
                                             SOUNDTOUCH_SAMPLE_BUFFERSIZE / nChannels / sizeof(CALLC_SAMPLETYPE));
        nBuffersize = nSamples * nChannels * sizeof(CALLC_SAMPLETYPE);
        memcpy(is->audio_buf2 + sum_size, (uint8_t *)sampleBuffer, nBuffersize);
        sum_size += nBuffersize;
    } while (nSamples != 0);
    return sum_size;
}
#endif

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
static int audio_decode_frame(RPlayer *is)
{
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    av_unused double audio_clock0;
    int wanted_nb_samples;
    uint output_size;
    Frame *af;

    if (is->paused)
        return -1;

#if BLOCK_PAUSE_MODE
    if (is->block_paused && !is->abort_request && !is->eof)
    {
        double queueDuration = av_q2d(is->audio_st->time_base) * is->audioq.duration;
        if (queueDuration > is->cache_duration)
        {
            if (is->block_paused)
            {
                is->block_paused = 0;
                notify_from_thread(is, MEDIA_BLOCK_PAUSE_MODE, BLOCK_PAUSE_OFF, PACKET_COME);
                is->block_pause_end_time = av_gettime_relative() / 1000000.0;
                printf("Quit block paused mode(consuming time = %lf) %lf %lf\n",
                       is->block_pause_end_time - is->block_pause_start_time, queueDuration, is->cache_duration);
            }
            is->decode_frame_total_time = 0.0f;
            is->last_start_timer_time = av_gettime_relative() / 1000000.0;
        }
        else
        {
            return -1;
        }
    }
#endif

    do
    {
#if defined(_WIN32)
        while (frame_queue_nb_remaining(&is->sampq) == 0)
        {
            if ((av_gettime_relative() - s->audio_callback_time)
                > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
                return -1;
            av_usleep(1000);
        }
#endif
        if (!(af = frame_queue_peek_readable(&is->sampq)))
        {
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
            if (is->eof)
            {
                if (Soundtouch_IsChangeTempo(is->m_soundTouch))
                {
                    output_size = SoundTouch_BufferHandle(is, true, 0);
                    if (output_size)
                    {
                        is->audio_buf = is->audio_buf2;
                        return output_size;
                    }
                }
            }
#endif
            return -1;
        }
        frame_queue_next(&is->sampq);
    } while (af->serial != is->audioq.serial);

    data_size = av_samples_get_buffer_size(NULL, af->frame->channels, af->frame->nb_samples, af->frame->format, 1);

    dec_channel_layout = (af->frame->channel_layout
                          && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
                             af->frame->channel_layout :
                             av_get_default_channel_layout(af->frame->channels);
    wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);

    if (af->frame->format != is->audio_src.fmt || dec_channel_layout != is->audio_src.channel_layout
        || af->frame->sample_rate != is->audio_src.freq || (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx))
    {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL, is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
                                         dec_channel_layout, af->frame->format, af->frame->sample_rate, 0, NULL);
        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0)
        {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d "
                   "channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format), af->frame->channels,
                   is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            if (is->swr_ctx)
                swr_free(&is->swr_ctx);
            return -1;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels = af->frame->channels;
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = af->frame->format;
    }

    if (is->swr_ctx)
    {
        const uint8_t **in = (const uint8_t **)af->frame->extended_data;
        uint8_t **out = &is->audio_buf1;
        int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples)
        {
            if (swr_set_compensation(is->swr_ctx,
                                     (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq
                                         / af->frame->sample_rate,
                                     wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate)
                < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1)
            return AVERROR(ENOMEM);
        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count)
        {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0)
                swr_free(&is->swr_ctx);
        }
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
    }
    else
    {
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }
    output_size = resampled_data_size;
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
    if (Soundtouch_IsChangeTempo(is->m_soundTouch))
    {
        output_size = SoundTouch_BufferHandle(is, false, resampled_data_size);
        if (output_size > 0)
        {
            is->audio_buf = is->audio_buf2;
        }
    }
    else
    {
        output_size = SoundTouch_BufferHandle(is, true, resampled_data_size);
        if (output_size > 0)
        {
            if (resampled_data_size > 0)
            {
                memcpy(is->audio_buf2 + output_size, is->audio_buf1, resampled_data_size);
            }
            is->audio_buf = is->audio_buf2;
            output_size += resampled_data_size;
        }
        else
        {
            output_size = resampled_data_size;
        }
    }
#endif
    audio_clock0 = is->audio_clock;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
        is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
    else
        is->audio_clock = NAN;
    is->audio_clock_serial = af->serial;
#ifdef DEBUG
{
    static double last_clock;
    printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n", is->audio_clock - last_clock, is->audio_clock,
           audio_clock0);
    last_clock = is->audio_clock;
}
#endif
    return output_size;
}

/* prepare a new audio buffer */
static void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
    RPlayer *is = opaque;
    int audio_size, len1;

    is->audio_callback_time = av_gettime_relative();

    while (len > 0)
    {
        if (is->audio_buf_index >= is->audio_buf_size)
        {
            audio_size = audio_decode_frame(is);
#if BLOCK_PAUSE_MODE
            {
                double delay = is->audio_clock - is->last_audio_clock;
                double currentTime = av_gettime_relative() / 1000000.0;

                is->last_audio_clock = is->audio_clock;
                is->decode_frame_total_time += delay;

                double deltaTime = currentTime - is->last_start_timer_time;

                if (deltaTime >= 1.0 && is->last_start_timer_time != 0)
                {
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
                    int isStuck
                        = is->decode_frame_total_time
                              < (deltaTime * ((100.0f + Soundtouch_GetTempoDelta(is->m_soundTouch)) / 100.0f) - 0.100)
                          && !is->paused && is->eof != 1;
#else
                    int isStuck = is->decode_frame_total_time < (deltaTime - 0.100) && !is->paused && is->eof != 1;
#endif
                    if (is->cur_block_paused_mode_sta && !is->block_paused && isStuck)
                    {
                        is->block_paused = 1;
                        notify_from_thread(is, MEDIA_BLOCK_PAUSE_MODE, BLOCK_PAUSE_ON, 0);
                        is->block_pause_start_time = av_gettime_relative() / 1000000.0;
                        printf(
                            "block_paused = %d, deltaTime = %lf, decode_frame_time = %lf, remaining queue = %d, "
                            "packetDuration = %lf \n",
                            is->block_paused, deltaTime, is->decode_frame_total_time,
                            frame_queue_nb_remaining(&is->sampq),
                            av_q2d(is->audio_st->time_base) * is->audioq.duration);
                    }
                    else if (isStuck)
                    {
                        notify_from_thread(is, MEDIA_PLAYING_STATUS, deltaTime * 1000,
                                           is->decode_frame_total_time * 1000);
                        printf(
                            "Warning! sdl may become blocking.deltaTime = %lf decode_frame_time = %lf, delay = %lf, "
                            "clock = %lf last clock = %lf \n",
                            deltaTime, is->decode_frame_total_time, delay, is->audio_clock, is->last_audio_clock);
                    }
                    is->decode_frame_total_time = 0.0f;
                    is->last_start_timer_time = currentTime;
                }
                else if (is->last_start_timer_time == 0)
                    is->last_start_timer_time = av_gettime_relative() / 1000000.0;
                is->last_decode_time = currentTime;
            }
#endif
            if (audio_size < 0)
            {
                /* if error, just output silence */
                is->audio_buf = NULL;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            }
            else
            {
                if (is->show_mode != SHOW_MODE_VIDEO)
                    update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)
            len1 = len;
        if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME)
        {
            memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
        }
        else
        {
            memset(stream, 0, len1);
            if (!is->muted && is->audio_buf)
                SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1,
                                   is->audio_volume);
        }

        if (is->audio_buf)
        {
            if (is->frame_consumed_notified == 0)
            {
                is->frame_consumed_notified = 1;
                is->status = PLAYING;
                notify_from_thread(is, MEDIA_PLAYING, 0, 0);
            }
        }

        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(is->audio_clock))
    {
        set_clock_at(&is->audclk,
                     is->audio_clock
                         - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec,
                     is->audio_clock_serial, is->audio_callback_time / 1000000.0);
        sync_clock_to_slave(&is->extclk, &is->audclk);
    }
}

static int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate,
                      struct AudioParams *audio_hw_params)
{
    SDL_AudioSpec wanted_spec, spec;
    RPlayer *is = opaque;
    const char *env;
    static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
    static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env)
    {
        wanted_nb_channels = atoi(env);
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    }
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout))
    {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq = wanted_sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples
        = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = opaque;
    pthread_mutex_lock(&devices_mutex);
    while (!(is->audio_dev = SDL_OpenAudioDevice(is->media_tag, 0, &wanted_spec, &spec,
                                                 SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE)))
    {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n", wanted_spec.channels, wanted_spec.freq,
               SDL_GetError());
	printf("SDL_OpenAudio id = %d \n", is->audio_dev);
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels)
        {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq)
            {
                av_log(NULL, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
                pthread_mutex_unlock(&devices_mutex);
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }
    pthread_mutex_unlock(&devices_mutex);
    if (spec.format != AUDIO_S16SYS)
    {
        av_log(NULL, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels)
    {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout)
        {
            av_log(NULL, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels = spec.channels;
    audio_hw_params->frame_size
        = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec
        = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0)
    {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }
    return spec.size;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(RPlayer *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int ret = 0;
    int stream_lowres = is->user_options.lowres;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return AVERROR(ENOMEM);

    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
        goto fail;
    av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);

    codec = avcodec_find_decoder(avctx->codec_id);

    switch (avctx->codec_type)
    {
        case AVMEDIA_TYPE_AUDIO:
            is->last_audio_stream = stream_index;
            forced_codec_name = is->user_options.audio_codec_name;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            is->last_subtitle_stream = stream_index;
            forced_codec_name = is->user_options.subtitle_codec_name;
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->last_video_stream = stream_index;
            forced_codec_name = is->user_options.video_codec_name;
            break;
        default:
            break;
    }
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec)
    {
        if (forced_codec_name)
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with name '%s'\n", forced_codec_name);
        else
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n", avctx->codec_id);
        ret = AVERROR(EINVAL);
        goto fail;
    }

    avctx->codec_id = codec->id;
    if (stream_lowres > av_codec_get_max_lowres(codec))
    {
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
               av_codec_get_max_lowres(codec));
        stream_lowres = av_codec_get_max_lowres(codec);
    }
    av_codec_set_lowres(avctx, stream_lowres);

    if (is->user_options.fast)
        avctx->flags2 |= AV_CODEC_FLAG2_FAST;

    opts = filter_codec_opts(is->codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0)
    {
        goto fail;
    }
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
    {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type)
    {
        case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
        {
            AVFilterContext *sink;

            is->audio_filter_src.freq = avctx->sample_rate;
            is->audio_filter_src.channels = avctx->channels;
            is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
            is->audio_filter_src.fmt = avctx->sample_fmt;
            if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
                goto fail;
            sink = is->out_audio_filter;
            sample_rate = av_buffersink_get_sample_rate(sink);
            nb_channels = av_buffersink_get_channels(sink);
            channel_layout = av_buffersink_get_channel_layout(sink);
        }
#else
            sample_rate = avctx->sample_rate;
            nb_channels = avctx->channels;
            channel_layout = avctx->channel_layout;
#endif

            /* prepare audio output */
            if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
                goto fail;
            is->audio_hw_buf_size = ret;
            is->audio_src = is->audio_tgt;
            is->audio_buf_size = 0;
            is->audio_buf_index = 0;

            /* init averaging filter */
            is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
            is->audio_diff_avg_count = 0;
            /* since we do not have a precise anough audio FIFO fullness,
               we correct audio sync only if larger than this threshold */
            is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

            is->audio_stream = stream_index;
            is->audio_st = ic->streams[stream_index];

            decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
            if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK))
                && !is->ic->iformat->read_seek)
            {
                is->auddec.start_pts = is->audio_st->start_time;
                is->auddec.start_pts_tb = is->audio_st->time_base;
            }
#if 1
            if ((ret = decoder_start(&is->auddec, audio_thread, is)) < 0)
                goto out;
            SDL_PauseAudioDevice(is->audio_dev, 0);
	    printf("%s: SDL_PauseAudioDevices audio_dev = %d\n", __func__, is->audio_dev);
#endif
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_stream = stream_index;
            is->video_st = ic->streams[stream_index];

            decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
#if 1
            if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
                goto out;
            is->queue_attachments_req = 1;
#endif
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            is->subtitle_stream = stream_index;
            is->subtitle_st = ic->streams[stream_index];

            decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
#if 1
            if ((ret = decoder_start(&is->subdec, subtitle_thread, is)) < 0)
                goto out;
#endif
            break;
        default:
            break;
    }
    goto out;

fail:
    avcodec_free_context(&avctx);
out:
    av_dict_free(&opts);

    return ret;
}

static int decode_interrupt_cb(void *ctx)
{
    RPlayer *is = ctx;
    return is->abort_request;
}

static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue, double duration)
{
    return stream_id < 0 || queue->abort_request || (st->disposition & AV_DISPOSITION_ATTACHED_PIC)
           || (queue->nb_packets > MIN_FRAMES
               && (!queue->duration || av_q2d(st->time_base) * queue->duration > duration));
}

static int is_realtime(AVFormatContext *s)
{
    if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") || !strcmp(s->iformat->name, "sdp"))
        return 1;

    if (s->pb && (!strncmp(s->filename, "rtp:", 4) || !strncmp(s->filename, "udp:", 4)))
        return 1;
    return 0;
}
#if BLOCK_PAUSE_MODE
static int is_http(AVFormatContext *s)
{
    if (s->pb && (!strncmp(s->filename, "http:", 5) || !strncmp(s->filename, "https:", 6)))
    {
        printf("Block paused mode is enabled!\n");
        return 1;
    }
    return 0;
}
#endif
/* this thread gets the stream from the disk or the network */
static int read_thread(void *arg)
{
    RPlayer *is = arg;
    AVFormatContext *ic = NULL;
    int err, i, ret;
    RplayerMediaErrorType media_err;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    AVDictionaryEntry *t;
    SDL_mutex *wait_mutex = SDL_CreateMutex();
    int scan_all_pmts_set = 0;
    int64_t pkt_ts;
    int last_pkt_time = 0;
    int stream_total_time = 0;
    if (!wait_mutex)
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        ret = AVERROR(ENOMEM);
        media_err = MEDIA_ERROR_OUT_OF_MEMORY;
        goto fail;
    }

    memset(st_index, -1, sizeof(st_index));
    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    is->last_subtitle_stream = is->subtitle_stream = -1;
    is->eof = 0;

    ic = avformat_alloc_context();
    if (!ic)
    {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        media_err = MEDIA_ERROR_OUT_OF_MEMORY;
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = is;
    if (!av_dict_get(is->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE))
    {
        av_dict_set(&is->format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }

    av_dict_set(&is->format_opts, "reconnect", "1", 0);
    if (is->reconnect_at_eof > 0)
    {
        av_dict_set(&is->format_opts, "reconnect_at_eof", "1", 0);
    }
    char buffer[32];
    if (is->reconnect_delay_max >= 0)
    {
        snprintf(buffer, sizeof(buffer), "%d\n", is->reconnect_delay_max);
        av_dict_set(&is->format_opts, "reconnect_delay_max", buffer, 0);
    }
    av_dict_set(&is->format_opts, "stimeout", "3000000", 0);
    av_dict_set(&is->format_opts, "timeout", "3000000", 0);
    if (is->recv_buf_size > 0)
    {
        snprintf(buffer, sizeof(buffer), "%d\n", is->recv_buf_size);
        av_dict_set(&is->format_opts, "recv_buffer_size", buffer, 0);
    }
    if (is->filename != NULL)
        err = avformat_open_input(&ic, is->filename, is->iformat, &is->format_opts);
    else {
        err = -1;
        media_err = MEDIA_ERROR_IO;
    }
    if (err < 0)
    {
        print_error(is->filename, err);
        printf("avformat_open_input error, file=%s error=%d.\n", is->filename, err);
        if (is->abort_request)
            ret = 0;
        else {
            media_err = MEDIA_ERROR_IO;
            ret = -1;
        }
        goto fail;
    }
    if (scan_all_pmts_set)
        av_dict_set(&is->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

    if ((t = av_dict_get(is->format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
    {
        if (strcmp(t->key, "reconnect") != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
            ret = AVERROR_OPTION_NOT_FOUND;
        }
        // goto fail;
    }
    is->ic = ic;

    if (is->user_options.genpts)
        ic->flags |= AVFMT_FLAG_GENPTS;

    av_format_inject_global_side_data(ic);

    if (is->user_options.find_stream_info)
    {
        AVDictionary **opts = setup_find_stream_info_opts(ic, is->codec_opts);
        int orig_nb_streams = ic->nb_streams;

        err = avformat_find_stream_info(ic, opts);

        if (opts != NULL)
        {
            for (i = 0; i < orig_nb_streams; i++)
                av_dict_free(&opts[i]);
            av_freep(&opts);
        }

        if (err < 0)
        {
            av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n", is->filename);
            ret = -1;
            media_err = MEDIA_ERROR_UNSUPPORTED;
            goto fail;
        }
    }

    if (ic->pb)
        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

    if (is->user_options.seek_by_bytes < 0)
        is->user_options.seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);

    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    // FIXME
#if 0
    if (!window_title && (t = av_dict_get(ic->metadata, "title", NULL, 0)))
        window_title = av_asprintf("%s - %s", t->value, input_filename);
#endif

    /* if seeking requested, we execute it */
    if (is->user_options.start_time != AV_NOPTS_VALUE)
    {
        int64_t timestamp;

        timestamp = is->user_options.start_time;
        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n", is->filename,
                   (double)timestamp / AV_TIME_BASE);
        }
    }

    is->realtime = is_realtime(ic);
#if BLOCK_PAUSE_MODE
    is->cur_block_paused_mode_sta = is->block_paused_mode_enabled && is_http(ic);
#endif
    if (is->user_options.show_status)
        av_dump_format(ic, 0, is->filename, 0);

    for (i = 0; i < ic->nb_streams; i++)
    {
        AVStream *st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        if (type >= 0 && is->user_options.wanted_stream_spec[type] && st_index[type] == -1)
            if (avformat_match_stream_specifier(ic, st, is->user_options.wanted_stream_spec[type]) > 0)
                st_index[type] = i;
    }
    for (i = 0; i < AVMEDIA_TYPE_NB; i++)
    {
        if (is->user_options.wanted_stream_spec[i] && st_index[i] == -1)
        {
            av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n",
                   is->user_options.wanted_stream_spec[i], av_get_media_type_string(i));
            st_index[i] = INT_MAX;
        }
    }

    if (!is->user_options.video_disable)
        st_index[AVMEDIA_TYPE_VIDEO]
            = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    if (!is->user_options.audio_disable)
        st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO],
                                                           st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
    if (!is->user_options.video_disable && !is->user_options.subtitle_disable)
        st_index[AVMEDIA_TYPE_SUBTITLE]
            = av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE, st_index[AVMEDIA_TYPE_SUBTITLE],
                                  (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ? st_index[AVMEDIA_TYPE_AUDIO] :
                                                                       st_index[AVMEDIA_TYPE_VIDEO]),
                                  NULL, 0);

    is->show_mode = show_mode;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {
        AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters *codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
        if (codecpar->width)
            set_default_window_size(is, codecpar->width, codecpar->height, sar);
    }

    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0)
    {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
        if (ret == -1) {
            media_err = MEDIA_ERROR_MALFORMED;
            goto fail;
        }
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }
    if (is->show_mode == SHOW_MODE_NONE)
    {
        is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;
        printf("SHOW MODE %d\n", is->show_mode);
    }

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0)
    {
        stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (is->video_stream < 0 && is->audio_stream < 0)
    {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n", is->filename);
        is->status = ERROR;
	media_err = MEDIA_ERROR_IO;
        ret = -1;
        goto fail;
    }

    if (is->user_options.infinite_buffer < 0 && is->realtime)
        is->user_options.infinite_buffer = 1;
    is->total_duration_ms = fftime_to_milliseconds(ic->duration);
    is->buf_update_notified = 0;
    is->frame_consumed_notified = 0;
    if (is->status != STOPPED && is->status != PAUSED && is->status != STOPPING)
    {
        is->status = PREPARED;
        notify_from_thread(is, MEDIA_PREPARED, 0, 0);
    }

    stream_total_time = (int)(ic->duration / AV_TIME_BASE);
    for (;;)
    {
        if (is->abort_request)
            break;
        if (is->paused != is->last_paused)
        {
            is->last_paused = is->paused;
            if (is->paused)
                is->read_pause_return = av_read_pause(ic);
            else
                av_read_play(ic);
        }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (is->paused && (!strcmp(ic->iformat->name, "rtsp") || (ic->pb && !strncmp(is->filename, "mmsh:", 5))))
        {
            /* wait 10 ms to avoid trying to get another packet */
            /* XXX: horrible */
            SDL_Delay(10);
            continue;
        }
#endif

        if (is->seek_req)
        {
            int64_t seek_target = is->seek_pos;
            int64_t seek_min = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
            int64_t seek_max = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
            // FIXME the +-2 is due to rounding being not done in the correct direction in generation
            //      of the seek_pos/seek_rel variables

            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", is->ic->filename);
            }
            else
            {
                if (is->audio_stream >= 0)
                {
                    packet_queue_flush(&is->audioq);
                    packet_queue_put(&is->audioq, &flush_pkt);
                }
                if (is->subtitle_stream >= 0)
                {
                    packet_queue_flush(&is->subtitleq);
                    packet_queue_put(&is->subtitleq, &flush_pkt);
                }
                if (is->video_stream >= 0)
                {
                    packet_queue_flush(&is->videoq);
                    packet_queue_put(&is->videoq, &flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE)
                {
                    set_clock(&is->extclk, NAN, 0);
                }
                else
                {
                    set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused)
                step_to_next_frame(is);
            notify_from_thread(is, MEDIA_SEEK_COMPLETE, 0, 0);
        }

        if (is->paused)
        {
            SDL_Delay(10);
            continue;
        }

        if (is->queue_attachments_req)
        {
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)
            {
                AVPacket copy = { 0 };
                if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
                    goto fail;
                packet_queue_put(&is->videoq, &copy);
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }

        /* if the queue are full, no need to read more */
        if (is->user_options.infinite_buffer < 1
            && (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
                || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq,
#if BLOCK_PAUSE_MODE
                                              is->cache_duration
#else
                                              DEFAULT_CACHE_DURATION
#endif
                                              )
                    && stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq, DEFAULT_CACHE_DURATION)
                    && stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq,
                                                 DEFAULT_CACHE_DURATION))))
        {
            /* wait 10 ms */
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        }

        if (!is->eof)
            is->eof_notified = 0;

        if (!is->paused
            && (!is->audio_st
                || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0))
            && (!is->video_st
                || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0)))
        {
            if (!is->eof)
            {
                if (is->buf_update_notified == 0)
                    notify_from_thread(is, MEDIA_BUFFERING_UPDATE, 2, 0);
                is->buf_update_notified = 1;
            }
            else
            {
                // if (is->user_options.loop != 1 && (!is->user_options.loop || --is->user_options.loop)) {
                if (is->user_options.loop >= 1)
                {
                    printf("playback loop%d\n", is->user_options.loop);
                    stream_seek(is, is->user_options.start_time != AV_NOPTS_VALUE ? is->user_options.start_time : 0, 0,
                                0);
                }
                else
                {
                    if (!is->eof_notified)
                    {
                        is->status = COMPLETE;
                        notify_from_thread(is, MEDIA_PLAYBACK_COMPLETE, 0, 0);
                        is->eof_notified = 1;
                    }
                    if (is->user_options.autoexit)
                    {
                        ret = AVERROR_EOF;
                        goto fail;
                    }
                }
            }
        }
        else
        {
            if (is->buf_update_notified)
            {
                notify_from_thread(is, MEDIA_BUFFERING_UPDATE, 1, 0);
                is->buf_update_notified = 0;
            }
        }

        if (is->eof == 1)
        {
            SDL_Delay(10);
            continue;
        }

        ret = av_read_frame(ic, pkt);
        if (ret < 0)
        {
            printf("rk_ffplay av_read_frame ret %d \n", ret);
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof)
            {
                if (is->video_stream >= 0)
                    packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (is->audio_stream >= 0)
                    packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                if (is->subtitle_stream >= 0)
                    packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);

                printf("rk_ffplay stream_total_time:%d last_pkt_time:%d\n", stream_total_time, last_pkt_time);
                if ((stream_total_time - last_pkt_time) < 3)
                {
                    is->eof = 1;
#if BLOCK_PAUSE_MODE
                    if (is->block_paused)
                    {
                        is->block_paused = 0;
                        notify_from_thread(is, MEDIA_BLOCK_PAUSE_MODE, BLOCK_PAUSE_OFF, STREAM_END);
                        printf("Stream end and quit block mode\n");
                    }
                    is->decode_frame_total_time = 0.0f;
                    is->last_start_timer_time = av_gettime_relative() / 1000000.0;
#endif
                }
                else if (ret == AVERROR_EOF && !is->reconnect_at_eof)
                {
                    printf("av_read_frame ret %d and reconnect at eof is off!", ret);
                    goto fail;
                }
            }

            if (ic->pb && ic->pb->error)
            {
                char ebuf[156] = { 0 };
                av_strerror(ic->pb->error, ebuf, sizeof(ebuf));
                printf("av_read_frame ret %d error : %s\n", ret, ebuf);
                if (!is->eof && ic->pb->error != -ETIMEDOUT)
                {
                    goto fail;
                }
            }
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);

            continue;
        }
        else
        {
            is->eof = 0;
        }
        /* check if packet is in play range specified by user, then queue, otherwise discard */
        stream_start_time = ic->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        pkt_in_play_range
            = is->user_options.duration == AV_NOPTS_VALUE
              || (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0))
                             * av_q2d(ic->streams[pkt->stream_index]->time_base)
                         - (double)(is->user_options.start_time != AV_NOPTS_VALUE ? is->user_options.start_time : 0)
                               / 1000000
                     <= ((double)is->user_options.duration / 1000000);

        last_pkt_time = (int)(pkt_ts * av_q2d(ic->streams[pkt->stream_index]->time_base));

#if 0
        //fake network is not well, and can't got packet, by guzhimin
        if(last_pkt_time>15 && last_pkt_time <25)
        {

            if (is->video_stream >= 0)
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            if (is->audio_stream >= 0)
                packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
            if (is->subtitle_stream >= 0)
                packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);

            SDL_Delay(50);
            continue;
        }
#endif

        if (pkt->stream_index == is->audio_stream && pkt_in_play_range)
        {
            packet_queue_put(&is->audioq, pkt);
        }
        else if (pkt->stream_index == is->video_stream && pkt_in_play_range
                 && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC))
        {
            packet_queue_put(&is->videoq, pkt);
        }
        else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range)
        {
            packet_queue_put(&is->subtitleq, pkt);
        }
        else
        {
            av_packet_unref(pkt);
        }
    }

    ret = 0;
fail:
    printf("%s fail!\n", __func__);
    if (ic && !is->ic)
        avformat_close_input(&ic);

    if (ret != 0 && !is->abort_request)
    {
        is->status = ERROR;
        notify_from_thread(is, MEDIA_ERROR, media_err, 0);
    }
    SDL_DestroyMutex(wait_mutex);
    return -1;
}

static int stream_open(RPlayer *is, const char *filename, AVInputFormat *iformat)
{
    if (is->stop_tid)
    {
        SDL_WaitThread(is->stop_tid, NULL);
        printf("%s: wait stop_thread OK!\n", __func__);
        is->stop_tid = NULL;
    }
    is->filename = av_strdup(filename);
    if (!is->filename)
    {
        printf("media file name is null \n");
        goto fail;
    }
    is->iformat = iformat;
    is->ytop = 0;
    is->xleft = 0;
    is->abort_request = 0;
    video_output_create(is);
    /* start video display */
    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0) {
        printf("video fream queque init errr\n");
        goto fail;
    }
    if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0) {
        printf("sub frame quuque init errr\n");
        goto fail;
    }
    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0){
        printf("audio frame quuque init errr\n");
        goto fail;
    }
    if (packet_queue_init(&is->videoq) < 0 || packet_queue_init(&is->audioq) < 0
        || packet_queue_init(&is->subtitleq) < 0){
        printf("media package queque init errr\n");
        goto fail;
    }
    if (!(is->continue_read_thread = SDL_CreateCond()))
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        goto fail;
    }

    init_clock(&is->vidclk, &is->videoq.serial);
    init_clock(&is->audclk, &is->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);
    is->audio_clock_serial = -1;
    if (is->user_options.startup_volume < 0)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", is->user_options.startup_volume);
    if (is->user_options.startup_volume > 100)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", is->user_options.startup_volume);
    is->user_options.startup_volume = av_clip(is->user_options.startup_volume, 0, 100);
    is->user_options.startup_volume
        = av_clip(SDL_MIX_MAXVOLUME * is->user_options.startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
    is->audio_volume = is->user_options.startup_volume;
    is->muted = 0;
    is->av_sync_type = is->user_options.av_sync_type;

    is->paused = 1;
#if BLOCK_PAUSE_MODE
    is->block_paused = 0;
#endif
    if (!is->user_options.video_disable)
    {
        is->render_tid = SDL_CreateThread(render_thread, "render_thread", is);
        if (!is->render_tid)
        {
            av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
            goto fail;
        }
    }
    is->read_tid = SDL_CreateThread(read_thread, "read_thread", is);
    if (!is->read_tid)
    {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
    fail:
        is->status = ERROR;
        notify_from_thread(is, MEDIA_ERROR, MEDIA_ERROR_OUT_OF_MEMORY, 0);
        printf("%s fail!\n", __func__);
        stream_close(is);
        return -1;
    }
    return 0;
}

static void stream_cycle_channel(RPlayer *is, int codec_type)
{
    AVFormatContext *ic = is->ic;
    int start_index, stream_index;
    int old_index;
    AVStream *st;
    AVProgram *p = NULL;
    int nb_streams = is->ic->nb_streams;

    if (codec_type == AVMEDIA_TYPE_VIDEO)
    {
        start_index = is->last_video_stream;
        old_index = is->video_stream;
    }
    else if (codec_type == AVMEDIA_TYPE_AUDIO)
    {
        start_index = is->last_audio_stream;
        old_index = is->audio_stream;
    }
    else
    {
        start_index = is->last_subtitle_stream;
        old_index = is->subtitle_stream;
    }
    stream_index = start_index;

    if (codec_type != AVMEDIA_TYPE_VIDEO && is->video_stream != -1)
    {
        p = av_find_program_from_stream(ic, NULL, is->video_stream);
        if (p)
        {
            nb_streams = p->nb_stream_indexes;
            for (start_index = 0; start_index < nb_streams; start_index++)
                if (p->stream_index[start_index] == stream_index)
                    break;
            if (start_index == nb_streams)
                start_index = -1;
            stream_index = start_index;
        }
    }

    for (;;)
    {
        if (++stream_index >= nb_streams)
        {
            if (codec_type == AVMEDIA_TYPE_SUBTITLE)
            {
                stream_index = -1;
                is->last_subtitle_stream = -1;
                goto the_end;
            }
            if (start_index == -1)
                return;
            stream_index = 0;
        }
        if (stream_index == start_index)
            return;
        st = is->ic->streams[p ? p->stream_index[stream_index] : stream_index];
        if (st->codecpar->codec_type == codec_type)
        {
            /* check that parameters are OK */
            switch (codec_type)
            {
                case AVMEDIA_TYPE_AUDIO:
                    if (st->codecpar->sample_rate != 0 && st->codecpar->channels != 0)
                        goto the_end;
                    break;
                case AVMEDIA_TYPE_VIDEO:
                case AVMEDIA_TYPE_SUBTITLE:
                    goto the_end;
                default:
                    break;
            }
        }
    }
the_end:
    if (p && stream_index != -1)
        stream_index = p->stream_index[stream_index];
    av_log(NULL, AV_LOG_INFO, "Switch %s stream from #%d to #%d\n", av_get_media_type_string(codec_type), old_index,
           stream_index);

    stream_component_close(is, old_index);
    stream_component_open(is, stream_index);
}

static void toggle_full_screen(RPlayer *is)
{
    is->is_full_screen = !is->is_full_screen;
    SDL_SetWindowFullscreen(is->window, is->is_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static void toggle_audio_display(RPlayer *is)
{
    int next = is->show_mode;
    do
    {
        next = (next + 1) % SHOW_MODE_NB;
    } while ((next != is->show_mode)
             && ((next == SHOW_MODE_VIDEO && !is->video_st) || (next != SHOW_MODE_VIDEO && !is->audio_st)));
    if (is->show_mode != next)
    {
        is->force_refresh = 1;
        is->show_mode = next;
    }
}

#if 0
static void refresh_loop_wait_event(RPlayer *is, SDL_Event *event) {
    double remaining_time = 0.0;
    SDL_PumpEvents();
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) {
            SDL_ShowCursor(0);
            cursor_hidden = 1;
        }
        if (remaining_time > 0.0)
            av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
            video_refresh(is, &remaining_time);
        SDL_PumpEvents();
    }
}
#endif

static void seek_chapter(RPlayer *is, int incr)
{
    int64_t pos = get_master_clock(is) * AV_TIME_BASE;
    int i;

    if (!is->ic->nb_chapters)
        return;

    /* find the current chapter */
    for (i = 0; i < is->ic->nb_chapters; i++)
    {
        AVChapter *ch = is->ic->chapters[i];
        if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0)
        {
            i--;
            break;
        }
    }

    i += incr;
    i = FFMAX(i, 0);
    if (i >= is->ic->nb_chapters)
        return;

    av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
    stream_seek(is, av_rescale_q(is->ic->chapters[i]->start, is->ic->chapters[i]->time_base, AV_TIME_BASE_Q), 0, 0);
}

#if 0
/* handle an event sent by the GUI */
static void event_loop(RPlayer *cur_stream)
{
    printf("### call %s %d\n", __func__, __LINE__);
    SDL_Event event;
    double incr, pos, frac;

    for ( ;!cur_stream->loop_exit; ) {
        double x;
        refresh_loop_wait_event(cur_stream, &event);
        switch (event.type) {
        case SDL_KEYDOWN:
            if (exit_on_keydown) {
                printf("### %s %d\n", __func__, __LINE__);
                do_exit(cur_stream);
                break;
            }
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_q:
                printf("### %s %d\n", __func__, __LINE__);
                do_exit(cur_stream);
                break;
            case SDLK_f:
                toggle_full_screen(cur_stream);
                cur_stream->force_refresh = 1;
                break;
            case SDLK_p:
            case SDLK_SPACE:
                toggle_pause(cur_stream);
                break;
            case SDLK_m:
                toggle_mute(cur_stream);
                break;
            case SDLK_KP_MULTIPLY:
            case SDLK_0:
                update_volume(cur_stream, 1, SDL_VOLUME_STEP);
                break;
            case SDLK_KP_DIVIDE:
            case SDLK_9:
                update_volume(cur_stream, -1, SDL_VOLUME_STEP);
                break;
            case SDLK_s: // S: Step to next frame
                step_to_next_frame(cur_stream);
                break;
            case SDLK_a:
                stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
                break;
            case SDLK_v:
                stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
                break;
            case SDLK_c:
                stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
                stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
                stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
                break;
            case SDLK_t:
                stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
                break;
            case SDLK_w:
#if CONFIG_AVFILTER
                if (cur_stream->show_mode == SHOW_MODE_VIDEO && cur_stream->vfilter_idx < nb_vfilters - 1) {
                    if (++cur_stream->vfilter_idx >= nb_vfilters)
                        cur_stream->vfilter_idx = 0;
                } else {
                    cur_stream->vfilter_idx = 0;
                    toggle_audio_display(cur_stream);
                }
#else
                toggle_audio_display(cur_stream);
#endif
                break;
            case SDLK_PAGEUP:
                if (cur_stream->ic->nb_chapters <= 1) {
                    incr = 600.0;
                    goto do_seek;
                }
                seek_chapter(cur_stream, 1);
                break;
            case SDLK_PAGEDOWN:
                if (cur_stream->ic->nb_chapters <= 1) {
                    incr = -600.0;
                    goto do_seek;
                }
                seek_chapter(cur_stream, -1);
                break;
            case SDLK_LEFT:
                incr = -10.0;
                goto do_seek;
            case SDLK_RIGHT:
                incr = 10.0;
                goto do_seek;
            case SDLK_UP:
                incr = 60.0;
                goto do_seek;
            case SDLK_DOWN:
                incr = -60.0;
            do_seek:
                    if (seek_by_bytes) {
                        pos = -1;
                        if (pos < 0 && cur_stream->video_stream >= 0)
                            pos = frame_queue_last_pos(&cur_stream->pictq);
                        if (pos < 0 && cur_stream->audio_stream >= 0)
                            pos = frame_queue_last_pos(&cur_stream->sampq);
                        if (pos < 0)
                            pos = avio_tell(cur_stream->ic->pb);
                        if (cur_stream->ic->bit_rate)
                            incr *= cur_stream->ic->bit_rate / 8.0;
                        else
                            incr *= 180000.0;
                        pos += incr;
                        stream_seek(cur_stream, pos, incr, 1);
                    } else {
                        pos = get_master_clock(cur_stream);
                        if (isnan(pos))
                            pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
                        pos += incr;
                        if (cur_stream->ic->start_time != AV_NOPTS_VALUE && pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
                            pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
                        stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
                    }
                break;
            default:
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (exit_on_mousedown) {
                printf("### %s %d\n", __func__, __LINE__);
                do_exit(cur_stream);
                break;
            }
            if (event.button.button == SDL_BUTTON_LEFT) {
                static int64_t last_mouse_left_click = 0;
                if (av_gettime_relative() - last_mouse_left_click <= 500000) {
                    toggle_full_screen(cur_stream);
                    cur_stream->force_refresh = 1;
                    last_mouse_left_click = 0;
                } else {
                    last_mouse_left_click = av_gettime_relative();
                }
            }
        case SDL_MOUSEMOTION:
            if (cursor_hidden) {
                SDL_ShowCursor(1);
                cursor_hidden = 0;
            }
            cursor_last_shown = av_gettime_relative();
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button != SDL_BUTTON_RIGHT)
                    break;
                x = event.button.x;
            } else {
                if (!(event.motion.state & SDL_BUTTON_RMASK))
                    break;
                x = event.motion.x;
            }
                if (seek_by_bytes || cur_stream->ic->duration <= 0) {
                    uint64_t size =  avio_size(cur_stream->ic->pb);
                    stream_seek(cur_stream, size*x/cur_stream->width, 0, 1);
                } else {
                    int64_t ts;
                    int ns, hh, mm, ss;
                    int tns, thh, tmm, tss;
                    tns  = cur_stream->ic->duration / 1000000LL;
                    thh  = tns / 3600;
                    tmm  = (tns % 3600) / 60;
                    tss  = (tns % 60);
                    frac = x / cur_stream->width;
                    ns   = frac * tns;
                    hh   = ns / 3600;
                    mm   = (ns % 3600) / 60;
                    ss   = (ns % 60);
                    av_log(NULL, AV_LOG_INFO,
                           "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
                            hh, mm, ss, thh, tmm, tss);
                    ts = frac * cur_stream->ic->duration;
                    if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
                        ts += cur_stream->ic->start_time;
                    stream_seek(cur_stream, ts, 0, 0);
                }
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    screen_width  = cur_stream->width  = event.window.data1;
                    screen_height = cur_stream->height = event.window.data2;
                    if (cur_stream->vis_texture) {
                        SDL_DestroyTexture(cur_stream->vis_texture);
                        cur_stream->vis_texture = NULL;
                    }
                case SDL_WINDOWEVENT_EXPOSED:
                    cur_stream->force_refresh = 1;
            }
            break;
        case SDL_QUIT:
        case FF_QUIT_EVENT:
                printf("### %s %d\n", __func__, __LINE__);
            do_exit(cur_stream);
            break;
        default:
            break;
        }
    }
}
#endif

static int dummy;

#if 0
static const OptionDef options[] = {
    CMDUTILS_COMMON_OPTIONS
    { "x", HAS_ARG, { .func_arg = opt_width }, "force displayed width", "width" },
    { "y", HAS_ARG, { .func_arg = opt_height }, "force displayed height", "height" },
    { "s", HAS_ARG | OPT_VIDEO, { .func_arg = opt_frame_size }, "set frame size (WxH or abbreviation)", "size" },
    { "fs", OPT_BOOL, { &is_full_screen }, "force full screen" },
    { "an", OPT_BOOL, { &audio_disable }, "disable audio" },
    { "vn", OPT_BOOL, { &video_disable }, "disable video" },
    { "sn", OPT_BOOL, { &subtitle_disable }, "disable subtitling" },
    { "ast", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_AUDIO] }, "select desired audio stream", "stream_specifier" },
    { "vst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_VIDEO] }, "select desired video stream", "stream_specifier" },
    { "sst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] }, "select desired subtitle stream", "stream_specifier" },
    { "ss", HAS_ARG, { .func_arg = opt_seek }, "seek to a given position in seconds", "pos" },
    { "t", HAS_ARG, { .func_arg = opt_duration }, "play  \"duration\" seconds of audio/video", "duration" },
    { "bytes", OPT_INT | HAS_ARG, { &seek_by_bytes }, "seek by bytes 0=off 1=on -1=auto", "val" },
    { "nodisp", OPT_BOOL, { &display_disable }, "disable graphical display" },
    { "noborder", OPT_BOOL, { &borderless }, "borderless window" },
    { "volume", OPT_INT | HAS_ARG, { &startup_volume}, "set startup volume 0=min 100=max", "volume" },
    { "f", HAS_ARG, { .func_arg = opt_format }, "force format", "fmt" },
    { "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = opt_frame_pix_fmt }, "set pixel format", "format" },
    { "stats", OPT_BOOL | OPT_EXPERT, { &show_status }, "show status", "" },
    { "fast", OPT_BOOL | OPT_EXPERT, { &fast }, "non spec compliant optimizations", "" },
    { "genpts", OPT_BOOL | OPT_EXPERT, { &genpts }, "generate pts", "" },
    { "drp", OPT_INT | HAS_ARG | OPT_EXPERT, { &decoder_reorder_pts }, "let decoder reorder pts 0=off 1=on -1=auto", ""},
    { "lowres", OPT_INT | HAS_ARG | OPT_EXPERT, { &lowres }, "", "" },
    { "sync", HAS_ARG | OPT_EXPERT, { .func_arg = opt_sync }, "set audio-video sync. type (type=audio/video/ext)", "type" },
    { "autoexit", OPT_BOOL | OPT_EXPERT, { &autoexit }, "exit at the end", "" },
    { "exitonkeydown", OPT_BOOL | OPT_EXPERT, { &exit_on_keydown }, "exit on key down", "" },
    { "exitonmousedown", OPT_BOOL | OPT_EXPERT, { &exit_on_mousedown }, "exit on mouse down", "" },
    { "loop", OPT_INT | HAS_ARG | OPT_EXPERT, { &loop }, "set bool of the playback,1 means circular play", 0 means no cycle},
    { "framedrop", OPT_BOOL | OPT_EXPERT, { &framedrop }, "drop frames when cpu is too slow", "" },
    { "infbuf", OPT_BOOL | OPT_EXPERT, { &infinite_buffer }, "don't limit the input buffer size (useful with realtime streams)", "" },
    { "window_title", OPT_STRING | HAS_ARG, { &window_title }, "set window title", "window title" },
#if CONFIG_AVFILTER
    { "vf", OPT_EXPERT | HAS_ARG, { .func_arg = opt_add_vfilter }, "set video filters", "filter_graph" },
    { "af", OPT_STRING | HAS_ARG, { &afilters }, "set audio filters", "filter_graph" },
#endif
    { "rdftspeed", OPT_INT | HAS_ARG| OPT_AUDIO | OPT_EXPERT, { &rdftspeed }, "rdft speed", "msecs" },
    { "showmode", HAS_ARG, { .func_arg = opt_show_mode}, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" },
    { "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, { .func_arg = opt_default }, "generic catch all option", "" },
    { "i", OPT_BOOL, { &dummy}, "read specified file", "input_file"},
    { "codec", HAS_ARG, { .func_arg = opt_codec}, "force decoder", "decoder_name" },
    { "acodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &audio_codec_name }, "force audio decoder",    "decoder_name" },
    { "scodec", HAS_ARG | OPT_STRING | OPT_EXPERT, { &subtitle_codec_name }, "force subtitle decoder", "decoder_name" },
    { "vcodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &video_codec_name }, "force video decoder",    "decoder_name" },
    { "autorotate", OPT_BOOL, { &autorotate }, "automatically rotate video", "" },
    { "find_stream_info", OPT_BOOL | OPT_INPUT | OPT_EXPERT, { &find_stream_info },
        "read and decode the streams to fill missing information with heuristics" },
    { NULL, },
};

static void show_usage(void)
{
    av_log(NULL, AV_LOG_INFO, "Simple media player\n");
    av_log(NULL, AV_LOG_INFO, "usage: %s [options] input_file\n", program_name);
    av_log(NULL, AV_LOG_INFO, "\n");
}

void show_help_default(const char *opt, const char *arg)
{
    av_log_set_callback(log_callback_help);
    show_usage();
    printf("\n");
#if !CONFIG_AVFILTER
    show_help_children(sws_get_class(), AV_OPT_FLAG_ENCODING_PARAM);
#else
    show_help_children(avfilter_get_class(), AV_OPT_FLAG_FILTERING_PARAM);
#endif
    printf("\nWhile playing:\n"
           "q, ESC              quit\n"
           "f                   toggle full screen\n"
           "p, SPC              pause\n"
           "m                   toggle mute\n"
           "9, 0                decrease and increase volume respectively\n"
           "/, *                decrease and increase volume respectively\n"
           "a                   cycle audio channel in the current program\n"
           "v                   cycle video channel\n"
           "t                   cycle subtitle channel in the current program\n"
           "c                   cycle program\n"
           "w                   cycle video filters or show modes\n"
           "s                   activate frame-step mode\n"
           "left/right          seek backward/forward 10 seconds\n"
           "down/up             seek backward/forward 1 minute\n"
           "page down/page up   seek backward/forward 10 minutes\n"
           "right mouse click   seek to percentage in file corresponding to fraction of width\n"
           "left double-click   toggle full screen\n"
           );
}
#endif

static int lockmgr(void **mtx, enum AVLockOp op)
{
    switch (op)
    {
        case AV_LOCK_CREATE:
            *mtx = SDL_CreateMutex();
            if (!*mtx)
            {
                av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
                return 1;
            }
            return 0;
        case AV_LOCK_OBTAIN:
            return !!SDL_LockMutex(*mtx);
        case AV_LOCK_RELEASE:
            return !!SDL_UnlockMutex(*mtx);
        case AV_LOCK_DESTROY:
            SDL_DestroyMutex(*mtx);
            return 0;
    }
    return 1;
}

#if 0
/* Called from the main */
int main(int argc, char **argv)
{
    RPlayer *is;

    init_dynload();

    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    parse_loglevel(argc, argv, options);

    /* register all codecs, demux and protocols */
    avdevice_register_all();
    avfilter_register_all();
    av_register_all();
    avformat_network_init();

    init_opts();

    signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).    */
    signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

    parse_options(NULL, argc, argv, options, opt_input_file);

    if (!input_filename) {
        show_usage();
        av_log(NULL, AV_LOG_FATAL, "An input file must be specified\n");
        av_log(NULL, AV_LOG_FATAL,
               "Use -h to get full help or, even better, run 'man %s'\n", program_name);
        exit(1);
    }
    if (av_lockmgr_register(lockmgr)) {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize lock manager!\n");
        do_exit(NULL);
    }

#if 0
    if (display_disable) {
        video_disable = 1;
    }
    flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    if (audio_disable)
        flags &= ~SDL_INIT_AUDIO;
    else {
        /* Try to work around an occasional ALSA buffer underflow issue when the
         * period size is NPOT due to ALSA resampling by forcing the buffer size. */
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
            SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
    }
    if (display_disable)
        flags &= ~SDL_INIT_VIDEO;
    if (SDL_Init (flags)) {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
        av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
        exit(1);
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);


    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;

    if (!display_disable) {
        int flags = SDL_WINDOW_HIDDEN;
        if (borderless)
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;
        window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, flags);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if (window) {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!renderer) {
                av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
                renderer = SDL_CreateRenderer(window, -1, 0);
            }
            if (renderer) {
                if (!SDL_GetRendererInfo(renderer, &is->renderer_info))
                    av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", is->renderer_info.name);
            }
        }
        if (!window || !renderer || !is->renderer_info.num_texture_formats) {
            av_log(NULL, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
            do_exit(NULL);
        }
    }
#endif

    is = rplayer_create();
    int r = rplayer_prepare(is, input_filename);
    if (r != 0) 
    {
        av_log(NULL, AV_LOG_FATAL, "Failed to prepare"); 
        return -1;
    }
    r = rplayer_start(is);
    if (r != 0) 
    {
        av_log(NULL, AV_LOG_FATAL, "Failed to start"); 
        return -1;
    }
    //stream_open(is, input_filename, file_iformat);

    //event_loop(is);

    /* never returns */

    return 0;
}
#endif

static int global_initialized = 0;
void rplayer_global_init()
{
    if (global_initialized == 1)
        return;

    /* register all codecs, demux and protocols */
    avdevice_register_all();
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();
    avformat_network_init();
    av_lockmgr_register(lockmgr);

    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;

    global_initialized = 1;
}

void rplayer_global_deinit()
{
    if (global_initialized)
    {
        avformat_network_deinit();
        av_lockmgr_register(NULL);
        SDL_Quit();
    }
    global_initialized = 0;
}

void rplayer_set_media_tag(RPlayer *is, const char *media_tag)
{
    if (!is || !media_tag)
        return;
    if (is->media_tag)
        free(is->media_tag);

    is->media_tag = strdup(media_tag);
}
#if BLOCK_PAUSE_MODE
void rplayer_set_cache_duration(RPlayer *is, double duration, int blockPauseMode)
{
    if (!is)
        return;
    if (duration < 0)
        duration = 0;
    else if (duration > MAX_CACHE_DURATION)
        duration = MAX_CACHE_DURATION;
    is->cache_duration = duration;
    is->block_paused_mode_enabled = blockPauseMode;
}
void rplayer_set_recv_buf_size(RPlayer *is, int size)
{
    if(!is)
      return;
    is->recv_buf_size = size > 0 ? size:LOW_CACHE_SIZE;
    printf("set recv_buf_size %d = \n",is->recv_buf_size);
}

void rplayer_enable_cache_mode(RPlayer *is, int enabled)
{
    if (is)
        is->block_paused_mode_enabled = enabled;
}
#endif
RPlayer *rplayer_create()
{
    RPlayer *is = malloc(sizeof(*is));
    memset(is, 0, sizeof(*is));
    if (NULL == is)
        return NULL;
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
    is->m_soundTouch = GetInstance();
#endif
    rplayer_global_init();
    set_default_options(&is->user_options);
    is->recv_buf_size = LOW_CACHE_SIZE;
    is->reconnect_at_eof = 1;
    is->status = IDLE;
    is->ctrl_mutex = SDL_CreateMutex();
    is->reconnect_delay_max = -1;
    return is;
}

void rplayer_destroy(RPlayer *is)
{
    printf("### %s\n", __func__);
    if (is)
    {
        rplayer_set_event_listener(is, NULL, NULL);
        av_dict_free(&is->codec_opts);
        av_dict_free(&is->format_opts);
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
        if (is->m_soundTouch)
        {
            ReleaseInstance(&is->m_soundTouch);
        }
#endif
        if (is->stop_tid)
        {
            SDL_WaitThread(is->stop_tid, NULL);
            is->stop_tid = NULL;
        }
        else
        {
            stream_close(is);
            video_output_destroy(is);
        }
        SDL_DestroyMutex(is->ctrl_mutex);
        is->ctrl_mutex = NULL;
        if (is->media_tag)
        {
            free(is->media_tag);
        }
        if (is->notify_id)
            SDL_RemoveTimer(is->notify_id);
        free(is);
        is = NULL;
    }
}

void rplayer_enable_display(RPlayer *is, int display_enabled)
{
    if (is)
        is->user_options.display_disable = !display_enabled;
}

int rplayer_prepare(RPlayer *is, const char *file_name)
{
    if (NULL == is || file_name == NULL)
        return -1;

    int r;
    SDL_LockMutex(is->ctrl_mutex);
    printf("%s begin!\n", __func__);
    if (is->status == PREPARING || (is->status != IDLE && is->status != STOPPED))
    {
        SDL_UnlockMutex(is->ctrl_mutex);
        return -1;
    }

    is->status = PREPARING;
    is->eof_notified = 0;
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
    Soundtouch_SetState(is->m_soundTouch, SOUNDTOUCH_INIT);
#endif
    r = stream_open(is, file_name, is->user_options.file_iformat);
    printf("%s: create read thread OK! (ret = %d)\n", __func__, r);
    SDL_UnlockMutex(is->ctrl_mutex);
    if (r == 0)
    {
        while (!is->abort_request && is->status != PREPARED)
        {
            if (is->status == ERROR)
            {
                r = -1;
                break;
            }

            SDL_Delay(10);
        }
    }
    return r;
}

int rplayer_prepare_async(RPlayer *is, const char *file_name)
{
    if (NULL == is || file_name == NULL)
        return -1;

    int r;
    SDL_LockMutex(is->ctrl_mutex);
    printf("prepare async begin\n");
    if (is->status != IDLE && is->status != STOPPED)
    {
        SDL_UnlockMutex(is->ctrl_mutex);
        return -1;
    }
    is->status = PREPARING;
    is->eof_notified = 0;
    r = stream_open(is, file_name, is->user_options.file_iformat);
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
    Soundtouch_SetState(is->m_soundTouch, SOUNDTOUCH_INIT);
#endif
    printf("prepare wait async\n");
    SDL_UnlockMutex(is->ctrl_mutex);
    return r;
}

int rplayer_start(RPlayer *is)
{
#if 0
    int ret = -1;
    if (!is)
        return -1;

    if (is->audio_st) {
        if ((ret = decoder_start(&is->auddec, audio_thread, is)) < 0)
            goto out;
        SDL_PauseAudioDevice(is->audio_dev, 0);
    }
                              
    if (is->video_st) {
        if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
            goto out;
        is->queue_attachments_req = 1;
    }

    if (is->subtitle_st) {
        if ((ret = decoder_start(&is->subdec, subtitle_thread, is)) < 0)
            goto out;
    }
out:
    if (ret == 0)
        is->paused = 0;
    return ret;
#else
    if (NULL == is)
        return -1;

    rplayer_resume(is);
    return is ? 0 : -1;
#endif
}

void rplayer_pause(RPlayer *is)
{
    if (NULL == is)
        return;

    SDL_LockMutex(is->ctrl_mutex);
    is->status = PAUSED;
    if (is && !is->paused)
    {
        toggle_pause(is, 1);
    }
    SDL_UnlockMutex(is->ctrl_mutex);
}

void rplayer_resume(RPlayer *is)
{
    if (NULL == is)
        return;
    SDL_LockMutex(is->ctrl_mutex);
    printf("%s begin\n", __func__);
    if (is && is->paused)
    {
        toggle_pause(is, 0);
    }
    is->status = PLAYING;
    printf("%s over\n", __func__);
    SDL_UnlockMutex(is->ctrl_mutex);
}

void rplayer_mute(RPlayer *is)
{
    if (NULL == is)
        return;

    toggle_mute(is);
}

int rplayer_is_playing(RPlayer *is)
{
    if (NULL == is)
        return 0;
    if (!is->paused && is->status == PLAYING)
        return 1;
        return 0;

}

int rplayer_is_paused(RPlayer *is)
{
    if (NULL == is)
        return 0;

    if (is->status == PAUSED && is->paused)
        return 1;
    else
        return 0;
}

void rplayer_stop(RPlayer *is)
{
    // rplayer_set_event_listener(is, NULL, NULL);
    if (NULL == is || is->status == STOPPED)
        return;

    is->status = STOPPING;
    SDL_LockMutex(is->ctrl_mutex);
    printf("%s begin!\n", __func__);
    stream_close(is);
    video_output_destroy(is);
    is->status = STOPPED;
    SDL_UnlockMutex(is->ctrl_mutex);
}

int stop_play_thread(void *arg)
{
    if (NULL == arg)
        return -1;

    RPlayer *is = arg;
    is->status = STOPPING;
    SDL_LockMutex(is->ctrl_mutex);
    printf("%s begin!\n", __func__);
    stream_close(is);
    video_output_destroy(is);
    is->status = STOPPED;
    notify_from_thread(is, MEDIA_STOPED, 0, 0);
    printf("%s quit!\n", __func__);
    SDL_UnlockMutex(is->ctrl_mutex);
    return 0;
}

void rplayer_stop_async(RPlayer *is)
{
    if (NULL == is || is->status == STOPPED)
        return;

    if (!is->stop_tid)
    {
        is->stop_tid = SDL_CreateThread(stop_play_thread, "stop play", is);
        if (!is->stop_tid)
        {
            av_log(NULL, AV_LOG_WARNING, "SDL_CreateThread() stop_play_thread: %s\n", SDL_GetError());
        }
    }
    else
    {
        av_log(NULL, AV_LOG_WARNING, "stop_play_thread is alreay created!");
    }
}

int rplayer_seek_to(RPlayer *is, int msec)
{
    if (!is || !is->ic || msec < 0)
        return -1;

    double pos = milliseconds_to_fftime(msec);
    if (isnan(pos))
        pos = (double)is->seek_pos / AV_TIME_BASE;

    if (is->ic->start_time != AV_NOPTS_VALUE && pos < is->ic->start_time / (double)AV_TIME_BASE)
        pos += is->ic->start_time;
    stream_seek(is, pos, 0, 0);
    return 0;
}

long rplayer_get_duration(RPlayer *is)
{
    if (!is)
        return 0;
    return is->total_duration_ms;
}

long rplayer_current_position(RPlayer *is)
{
    double clock = get_master_clock(is);
    if (isnan(clock))
        return fftime_to_milliseconds(is->seek_pos);
    else
        return clock * 1000;
}

int rplayer_get_loop(RPlayer *is)
{
    return is->user_options.loop;
}

void rplayer_set_loop(RPlayer *is, int lp)
{
    is->user_options.loop = lp;
}

void rplayer_reset(RPlayer *is)
{
    if (!is)
        return;
    is->abort_request = 0;
    is->total_duration_ms = 0;
#if BLOCK_PAUSE_MODE
    is->block_paused = 0;
    is->block_paused_mode_enabled = 0;
    is->cur_block_paused_mode_sta = 0;
#endif
    is->recv_buf_size = LOW_CACHE_SIZE;
    is->reconnect_at_eof = 0;
    is->reconnect_delay_max = -1;
    is->status = IDLE;
}

long rplayer_get_volume(RPlayer *is)
{
    long v = 0;
    if (is)
    {
        SDL_LockMutex(is->ctrl_mutex);
        v = lrint((double)is->audio_volume * 100 / SDL_MIX_MAXVOLUME);
        SDL_UnlockMutex(is->ctrl_mutex);
    }
    return v;
}

long rplayer_set_volume(RPlayer *is, long volume)
{
    long v = 0;

    if (is)
    {
        SDL_LockMutex(is->ctrl_mutex);
        v = volume;
        volume = lrint((double)volume * SDL_MIX_MAXVOLUME / 100);
        if (volume >= 0 && volume <= 128)
            is->audio_volume = volume;
        else if (volume > 128)
            is->audio_volume = SDL_MIX_MAXVOLUME;

        SDL_UnlockMutex(is->ctrl_mutex);
    }
    return v;
}

int rplayer_set_reconnect_at_eof(RPlayer *is, int enabled)
{
    int ret = -1;
    if (is)
    {
        is->reconnect_at_eof = enabled>0 ? 1 : 0;
        ret = 0;
    }
    return ret;
}

int rplayer_set_reconnect_delay_max(RPlayer *is, int seconds)
{
    int ret = -1;
    if (is)
    {
        is->reconnect_delay_max = seconds>=0 ? seconds : -1;
        ret = 0;
    }
    return ret;
}

#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
int rplayer_set_tempo(RPlayer *is, float tempo)
{
    int ret;
    if (is->m_soundTouch == NULL)
    {
        return -1;
    }
    Soundtouch_SetTempoDelta(is->m_soundTouch, tempo);
    Soundtouch_SetTempoChange(is->m_soundTouch, tempo);
    return 0;
}
#endif

void rplayer_set_event_listener(RPlayer *is, RPlayerEventListener listener, void *opaque)
{
    SDL_LockMutex(is->ctrl_mutex);
    is->event_callback = listener;
    is->event_cb_opaque = opaque;
    SDL_UnlockMutex(is->ctrl_mutex);
}

static uint32_t notify_from_thread_cb(uint32_t interval, void *opaque)
{
    Message *message = (Message *)opaque;

    if (!message)
    {
        return 0;
    }

    if (message->is && message->is->event_callback)
    {
        message->is->event_callback(message->is->event_cb_opaque, message->msg, message->ext1, message->ext2,
                                    message->from_thread);
        printf("Event callback\n");
    }

    free(message);

    return 0; /* 0 means stop timer */
}

void notify_from_thread(RPlayer *is, int msg, int ext1, int ext2)
{
    if (!is->event_callback)
        return;
    Message *message = malloc(sizeof(Message));
    message->is = is;
    message->msg = msg;
    message->ext1 = ext1;
    message->ext2 = ext2;
    message->from_thread = 1;
    is->notify_id = SDL_AddTimer(0, notify_from_thread_cb, message);
    printf("Notify message %d!\n", message->msg);
    if (!is->notify_id)
    {
        av_log(NULL, AV_LOG_FATAL, "Could not add timer for notification\n");
    }
}

static int video_output_create(RPlayer *is)
{
    int flags;

    if (is->user_options.display_disable)
    {
        is->user_options.video_disable = 1;
    }
    flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
    if (is->user_options.audio_disable)
        flags &= ~SDL_INIT_AUDIO;
    else
    {
        /* Try to work around an occasional ALSA buffer underflow issue when the
         * period size is NPOT due to ALSA resampling by forcing the buffer size. */
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
            SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
    }
    if (is->user_options.display_disable)
        flags &= ~SDL_INIT_VIDEO;
retry:
    if (!SDL_WasInit(flags) && SDL_Init(flags))
    {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
        av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
        if (flags & SDL_INIT_VIDEO)
        {
            flags &= ~SDL_INIT_VIDEO;
            is->user_options.video_disable = 1;
            is->user_options.display_disable = 1;
            goto retry;
        }
        is->user_options.display_disable = 1;
        is->user_options.video_disable = 1;
        return -1;
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    if (!is->user_options.display_disable)
    {
        int flags = SDL_WINDOW_HIDDEN;
        if (0)
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;
        is->window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      is->user_options.default_width, is->user_options.default_height, flags);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if (is->window)
        {
            is->renderer = SDL_CreateRenderer(is->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!is->renderer)
            {
                av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n",
                       SDL_GetError());
                is->renderer = SDL_CreateRenderer(is->window, -1, 0);
            }
            if (is->renderer)
            {
                if (!SDL_GetRendererInfo(is->renderer, &is->renderer_info))
                    av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", is->renderer_info.name);
            }
        }
        if (!is->window || !is->renderer || !is->renderer_info.num_texture_formats)
        {
            av_log(NULL, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());

            return -1;
        }
    }

    return 0;
}

static void video_output_destroy(RPlayer *is)
{
    if (is->renderer)
    {
        SDL_DestroyRenderer(is->renderer);
        is->renderer = NULL;
    }
    if (is->window)
    {
        SDL_DestroyWindow(is->window);
        is->window = NULL;
    }
#if CONFIG_AVFILTER
    av_freep(&vfilters_list);
#endif
    if (is->user_options.show_status)
        printf("\n");
}

static int render_thread(void *opaque)
{
    RPlayer *is = (RPlayer *)opaque;
    double remaining_time = 0.0;
    while (!is->abort_request)
    {
        if (remaining_time > 0.0)
            av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
            video_refresh(is, &remaining_time);
        SDL_Delay(10);
    }
    return 0;
}

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts)
{
    int i;
    AVDictionary **opts;

    if (!s->nb_streams)
        return NULL;
    opts = av_mallocz_array(s->nb_streams, sizeof(*opts));
    if (!opts)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
        return NULL;
    }
    for (i = 0; i < s->nb_streams; i++)
        opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
    return opts;
}

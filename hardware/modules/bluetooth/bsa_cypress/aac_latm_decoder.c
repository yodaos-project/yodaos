#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "aac_latm_decoder.h"
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>

typedef struct AACDFFmpeg {
    AVCodecContext *pCodecCtx;
    AVFrame *pFrame;
    struct SwrContext *au_convert_ctx;
    int out_buffer_size;
} AACDFFmpeg;

void *aac_decoder_create(int sample_rate, int channels, int bit)
{
    AACDFFmpeg *pComponent = (AACDFFmpeg *)malloc(sizeof(AACDFFmpeg));
    if (!pComponent)
       return NULL;

    memset(pComponent, 0, sizeof(AACDFFmpeg));
    av_register_all();
    AVCodec *pCodec = avcodec_find_decoder(AV_CODEC_ID_AAC_LATM);
    if (!pCodec) {
        printf("failed to find aac decoder!\r\n");
        goto find_decoder_err;
    }
    pComponent->pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pComponent->pCodecCtx) {
        printf("failed to alloc context3!\r\n");
        goto alloc_context_err;
    }
    pComponent->pCodecCtx->codec_type= AVMEDIA_TYPE_AUDIO;
    pComponent->pCodecCtx->channels = channels;
    pComponent->pCodecCtx->sample_rate = sample_rate;
    pComponent->pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    if(avcodec_open2(pComponent->pCodecCtx, pCodec, NULL) < 0) {
        printf("failed to open codec\r\n");
        goto codec_open_err;
    }

    pComponent->pFrame = av_frame_alloc();
    if (!pComponent->pFrame) {
        printf("failed to av_frame_alloc!\r\n");
        goto alloc_frame_err;
    }

    pComponent->au_convert_ctx = swr_alloc();
    if (!pComponent->au_convert_ctx) {
        printf("failed to swr_alloc!\r\n");
        goto alloc_swr_err;
    }

    return (void *)pComponent;

alloc_swr_err:
    if (pComponent->pFrame) {
        av_frame_free(&pComponent->pFrame);
        pComponent->pFrame = NULL;
    }
alloc_frame_err:
    avcodec_close(pComponent->pCodecCtx);
codec_open_err:
    if (pComponent->pCodecCtx) {
        avcodec_free_context(&pComponent->pCodecCtx);
        pComponent->pCodecCtx = NULL;
    }
alloc_context_err:
find_decoder_err:
    if (pComponent)
        free(pComponent);
    return NULL;
}

int aac_decode_frame(void *pParam, unsigned char *pData, int nLen,  acc_decode_cb completion, void *caller_handle)
{
    AACDFFmpeg *pAACD = (AACDFFmpeg *)pParam;
    AVPacket packet;
    av_init_packet(&packet);

    packet.size = nLen;
    packet.data = pData;

    int nRet = 0;
    if (packet.size > 0) {
        avcodec_send_packet(pAACD->pCodecCtx, &packet);
        nRet = avcodec_receive_frame(pAACD->pCodecCtx, pAACD->pFrame);
        if (nRet) {
            printf("faile to avcodec_receive_frame ret : %d  samples = %d \r\n", nRet, pAACD->pFrame->nb_samples);
            return nRet;
        }
#if 0//debug
        printf("avcodec_receive_frame  samples = %d, nb_samples =%d, format %d, channel %ld \r\n", pAACD->pFrame->sample_rate, 
	        pAACD->pFrame->nb_samples, pAACD->pFrame->format, pAACD->pFrame->channel_layout);
#endif
        pAACD->au_convert_ctx = swr_alloc_set_opts(pAACD->au_convert_ctx, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, pAACD->pCodecCtx->sample_rate,
                                      pAACD->pFrame->channel_layout, pAACD->pFrame->format, pAACD->pFrame->sample_rate, 0, NULL);
        swr_init(pAACD->au_convert_ctx);
        int out_linesize;
        int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
        pAACD->out_buffer_size = av_samples_get_buffer_size(&out_linesize, out_channels, pAACD->pFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
        //printf("av_samples_get_buffer_size %d\n", pAACD->out_buffer_size);
        uint8_t *out_buffer=(uint8_t *)av_malloc(pAACD->out_buffer_size);
        if (out_buffer) {
            swr_convert(pAACD->au_convert_ctx, &out_buffer, pAACD->out_buffer_size, (const uint8_t **)pAACD->pFrame->data, pAACD->pFrame->nb_samples);
            if (completion) {
                completion(caller_handle, out_buffer, pAACD->out_buffer_size);
            }
            av_free(out_buffer);
        }
    }

    return 0;
}

void aac_decode_close(void *pParam)
{
    AACDFFmpeg *pComponent = (AACDFFmpeg *)pParam;
    if (pComponent == NULL)
        return;

    if (pComponent->au_convert_ctx) {
        swr_free(&pComponent->au_convert_ctx);
        pComponent->au_convert_ctx = NULL;
    }

    if (pComponent->pFrame) {
        av_frame_free(&pComponent->pFrame);
        pComponent->pFrame = NULL;
    }

    if (pComponent->pCodecCtx) {
        avcodec_close(pComponent->pCodecCtx);
        avcodec_free_context(&pComponent->pCodecCtx);
        pComponent->pCodecCtx = NULL;
    }

    free(pComponent);
    pComponent = NULL;
    return;
}

/**
 * Parse MPEG-4 audio configuration for ALS object type.
 * @param[in] gb       bit reader context
 * @param[in] c        MPEG4AudioConfig structure to fill
 * @return on success 0 is returned, otherwise a value < 0
 */
static int parse_config_ALS(GetBitContext *gb, MPEG4AudioConfig *c)
{
    if (get_bits_left(gb) < 112)
        return -1;

    if (get_bits_long(gb, 32) != MKBETAG('A','L','S','\0'))
        return -1;

    // override AudioSpecificConfig channel configuration and sample rate
    // which are buggy in old ALS conformance files
    c->sample_rate = get_bits_long(gb, 32);

    // skip number of samples
    skip_bits_long(gb, 32);

    // read number of channels
    c->chan_config = 0;
    c->channels    = get_bits(gb, 16) + 1;

    return 0;
}

/* XXX: make sure to update the copies in the different encoders if you change
 * this table */
static const int avpriv_mpeg4audio_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static const uint8_t ff_mpeg4audio_channels[8] = {
    0, 1, 2, 3, 4, 5, 6, 8
};

static inline int get_object_type(GetBitContext *gb)
{
    int object_type = get_bits(gb, 5);
    if (object_type == AOT_ESCAPE)
        object_type = 32 + get_bits(gb, 6);
    return object_type;
}

static inline int get_sample_rate(GetBitContext *gb, int *index)
{
    *index = get_bits(gb, 4);
    return *index == 0x0f ? get_bits(gb, 24) :
        avpriv_mpeg4audio_sample_rates[*index];
}


static inline uint32_t latm_get_value(GetBitContext *b)
{
    int length = get_bits(b, 2);

    return get_bits_long(b, (length+1)*8);
}

static int ff_mpeg4audio_get_config_gb(MPEG4AudioConfig *c, GetBitContext *gb,
                                int sync_extension)
{
    int specific_config_bitindex, ret;
    int start_bit_index = get_bits_count(gb);
    c->object_type = get_object_type(gb);
    c->sample_rate = get_sample_rate(gb, &c->sampling_index);
    c->chan_config = get_bits(gb, 4);
    if (c->chan_config < FF_ARRAY_ELEMS(ff_mpeg4audio_channels))
        c->channels = ff_mpeg4audio_channels[c->chan_config];
    c->sbr = -1;
    c->ps  = -1;
    if (c->object_type == AOT_SBR || (c->object_type == AOT_PS &&
        // check for W6132 Annex YYYY draft MP3onMP4
        !(show_bits(gb, 3) & 0x03 && !(show_bits(gb, 9) & 0x3F)))) {
        if (c->object_type == AOT_PS)
            c->ps = 1;
        c->ext_object_type = AOT_SBR;
        c->sbr = 1;
        c->ext_sample_rate = get_sample_rate(gb, &c->ext_sampling_index);
        c->object_type = get_object_type(gb);
        if (c->object_type == AOT_ER_BSAC)
            c->ext_chan_config = get_bits(gb, 4);
    } else {
        c->ext_object_type = AOT_NULL;
        c->ext_sample_rate = 0;
    }
    specific_config_bitindex = get_bits_count(gb);

    if (c->object_type == AOT_ALS) {
        skip_bits(gb, 5);
        if (show_bits_long(gb, 24) != MKBETAG('\0','A','L','S'))
            skip_bits_long(gb, 24);

        specific_config_bitindex = get_bits_count(gb);

        ret = parse_config_ALS(gb, c);
        if (ret < 0)
            return ret;
    }

    if (c->ext_object_type != AOT_SBR && sync_extension) {
        while (get_bits_left(gb) > 15) {
            if (show_bits(gb, 11) == 0x2b7) { // sync extension
                get_bits(gb, 11);
                c->ext_object_type = get_object_type(gb);
                if (c->ext_object_type == AOT_SBR && (c->sbr = get_bits1(gb)) == 1) {
                    c->ext_sample_rate = get_sample_rate(gb, &c->ext_sampling_index);
                    if (c->ext_sample_rate == c->sample_rate)
                        c->sbr = -1;
                }
                if (get_bits_left(gb) > 11 && get_bits(gb, 11) == 0x548)
                    c->ps = get_bits1(gb);
                break;
            } else
                get_bits1(gb); // skip 1 bit
        }
    }

    //PS requires SBR
    if (!c->sbr)
        c->ps = 0;
    //Limit implicit PS to the HE-AACv2 Profile
    if ((c->ps == -1 && c->object_type != AOT_AAC_LC) || c->channels & ~0x01)
        c->ps = 0;

    return specific_config_bitindex - start_bit_index;
}

/**
 * Decode GA "General Audio" specific configuration; reference: table 4.1.
 *
 * @param   ac          pointer to AACContext, may be null
 * @param   avctx       pointer to AVCCodecContext, used for logging
 *
 * @return  Returns error status. 0 - OK, !0 - error
 */
static int decode_ga_specific_config(GetBitContext *gb,
                                     int get_bit_alignment,
                                     MPEG4AudioConfig *m4ac,
                                     int channel_config)
{
    int extension_flag;

    if (get_bits1(gb)) { // frameLengthFlag
        printf("960/120 MDCT window, need support ,welcome patch!!");
        return -1;
    }
    m4ac->frame_length_short = 0;

    if (get_bits1(gb))       // dependsOnCoreCoder
        skip_bits(gb, 14);   // coreCoderDelay
    extension_flag = get_bits1(gb);

    if (m4ac->object_type == AOT_AAC_SCALABLE ||
        m4ac->object_type == AOT_ER_AAC_SCALABLE)
        skip_bits(gb, 3);     // layerNr

    if (channel_config == 0) {
        skip_bits(gb, 4);  // element_instance_tag
        printf("not support yet ,welcome patch!!");
        return -1;
        #if 0
        tags = decode_pce(avctx, m4ac, layout_map, gb, get_bit_alignment);
        if (tags < 0)
            return tags;
	#endif
    } else {
        //if ((ret = set_default_channel_config(avctx, layout_map,
        //                                      &tags, channel_config)))
        //    return ret;
    }

    return 0;
}

static int decode_eld_specific_config(GetBitContext *gb,
                                     MPEG4AudioConfig *m4ac,
                                     int channel_config)
{
    m4ac->ps  = 0;
    m4ac->sbr = 0;
    m4ac->frame_length_short = get_bits1(gb);
    return 0;
}

static int decode_audio_specific_config_gb(MPEG4AudioConfig *m4ac,
                                           GetBitContext *gb,
                                           int get_bit_alignment,
                                        int sync_extension)
{
    int i, ret;
    GetBitContext gbc = *gb;

    if ((i = ff_mpeg4audio_get_config_gb(m4ac, &gbc, sync_extension)) < 0) {
        printf("failed to ff_mpeg4audio_get_config_gb %d\n", i);
        return -1;
    }

    if (m4ac->sampling_index > 12) {
        printf("failed:invalid sampling rate index %d\n",
               m4ac->sampling_index);
        return -1;
    }
    if (m4ac->object_type == AOT_ER_AAC_LD &&
        (m4ac->sampling_index < 3 || m4ac->sampling_index > 7)) {
        printf("failed: invalid low delay sampling rate index %d\n",
               m4ac->sampling_index);
        return -1;
    }

    skip_bits_long(gb, i);

    switch (m4ac->object_type) {
    case AOT_AAC_MAIN:
    case AOT_AAC_LC:
    case AOT_AAC_SSR:
    case AOT_AAC_LTP:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LD:
        if ((ret = decode_ga_specific_config(gb, get_bit_alignment,
                                            m4ac, m4ac->chan_config)) < 0)
            return ret;
        break;
    case AOT_ER_AAC_ELD:
        if ((ret = decode_eld_specific_config(gb,
                                              m4ac, m4ac->chan_config)) < 0)
            return ret;
        break;
    default:
        printf("Audio object type %s%d",
                                      m4ac->sbr == 1 ? "SBR+" : "",
                                      m4ac->object_type);
        return -ENOSYS;
    }

    printf("object_type %d chan config %d sampling index %d (%d) SBR %d PS %d\n",
            m4ac->object_type, m4ac->chan_config, m4ac->sampling_index,
            m4ac->sample_rate, m4ac->sbr,
            m4ac->ps);

    return get_bits_count(gb);
}

static int latm_decode_audio_specific_config(struct latm_parse_data *latmctx,
                                             GetBitContext *gb, int asclen)
{
    GetBitContext gbc;
    int config_start_bit  = get_bits_count(gb);
    int sync_extension    = 0;
    int bits_consumed;

    if (asclen > 0) {
        sync_extension = 1;
        asclen         = FFMIN(asclen, get_bits_left(gb));
        init_get_bits(&gbc, gb->buffer, config_start_bit + asclen);
        skip_bits_long(&gbc, config_start_bit);
    } else if (asclen == 0) {
        gbc = *gb;
    } else {
        return -1;
    }

    if (get_bits_left(gb) <= 0)
        return -1;

    bits_consumed = decode_audio_specific_config_gb(&latmctx->m4ac,
                                                    &gbc, config_start_bit,
                                                    sync_extension);

    if (bits_consumed < config_start_bit)
        return -1;
    bits_consumed -= config_start_bit;

    if (asclen == 0)
      asclen = bits_consumed;

    skip_bits_long(gb, asclen);

    return 0;
}

static int read_stream_mux_config(struct latm_parse_data *latmctx,
                                  GetBitContext *gb)
{
    int ret, audio_mux_version = get_bits(gb, 1);

    latmctx->audio_mux_version_A = 0;
    if (audio_mux_version)
        latmctx->audio_mux_version_A = get_bits(gb, 1);

    if (!latmctx->audio_mux_version_A) {

        if (audio_mux_version)
            latm_get_value(gb);                 // taraFullness

        skip_bits(gb, 1);                       // allStreamSameTimeFraming
        skip_bits(gb, 6);                       // numSubFrames
        // numPrograms
        if (get_bits(gb, 4)) {                  // numPrograms
            printf("not yet support ,welcome patchs!");
            return -1;
        }

        // for each program (which there is only one in DVB)

        // for each layer (which there is only one in DVB)
        if (get_bits(gb, 3)) {                   // numLayer
            printf("not yet support ,welcome patchs!");
            return -1;
        }

        // for all but first stream: use_same_config = get_bits(gb, 1);
        if (!audio_mux_version) {
            if ((ret = latm_decode_audio_specific_config(latmctx, gb, 0)) < 0)
                return ret;
        } else {
            int ascLen = latm_get_value(gb);
            if ((ret = latm_decode_audio_specific_config(latmctx, gb, ascLen)) < 0)
                return ret;
        }

        latmctx->frame_length_type = get_bits(gb, 3);
        switch (latmctx->frame_length_type) {
        case 0:
            skip_bits(gb, 8);       // latmBufferFullness
            break;
        case 1:
            latmctx->frame_length = get_bits(gb, 9);
            break;
        case 3:
        case 4:
        case 5:
            skip_bits(gb, 6);       // CELP frame length table index
            break;
        case 6:
        case 7:
            skip_bits(gb, 1);       // HVXC frame length table index
            break;
        }

        if (get_bits(gb, 1)) {                  // other data
            if (audio_mux_version) {
                latm_get_value(gb);             // other_data_bits
            } else {
                int esc;
                do {
                    esc = get_bits(gb, 1);
                    skip_bits(gb, 8);
                } while (esc);
            }
        }

        if (get_bits(gb, 1))                     // crc present
            skip_bits(gb, 8);                    // config_crc
    }

    return 0;
}

static int read_payload_length_info(struct latm_parse_data *ctx, GetBitContext *gb)
{
    uint8_t tmp;

    if (ctx->frame_length_type == 0) {
        int mux_slot_length = 0;
        do {
            if (get_bits_left(gb) < 8)
                return -1;
            tmp = get_bits(gb, 8);
            mux_slot_length += tmp;
        } while (tmp == 255);
        return mux_slot_length;
    } else if (ctx->frame_length_type == 1) {
        return ctx->frame_length;
    } else if (ctx->frame_length_type == 3 ||
               ctx->frame_length_type == 5 ||
               ctx->frame_length_type == 7) {
        skip_bits(gb, 2);          // mux_slot_length_coded
    }
    return 0;
}

static int read_audio_mux_element(struct latm_parse_data *latmctx, GetBitContext *gb)
{
    int err;
    uint8_t use_same_mux = get_bits(gb, 1);
    if (!use_same_mux) {
        if ((err = read_stream_mux_config(latmctx, gb)) < 0) {
            printf("failed to : read_stream_mux_config\n");
            return err;
        }
    } else {
        printf("failed to: no decoder config found\n");
        return -1;
    }
    if (latmctx->audio_mux_version_A == 0) {
        latmctx->mux_slot_length_offset =  get_bits_count(gb);
        printf("mux_slot_length_offset %d\n", latmctx->mux_slot_length_offset);
        latmctx->mux_slot_length_bytes = read_payload_length_info(latmctx, gb);
        if (latmctx->mux_slot_length_bytes <= 0 ) {
            printf("failed to parse mux_slot_length_bytes %d\n", latmctx->mux_slot_length_bytes);
            return -1;
        }
    }
    return 0;
}

int get_latm_frame_config(struct latm_parse_data *latmctx, uint8_t *buf, int len)
{
    int                 err;
    GetBitContext       gb;

    if ((err = init_get_bits8(&gb, buf, len)) < 0)
        return err;
    if ((err = read_audio_mux_element(latmctx, &gb)))
        return err;

    printf("%s, mux_slot_length_bytes %d, object_type %d, channel: %d, rate: %d\n", __func__,
            latmctx->mux_slot_length_bytes,latmctx->m4ac.object_type,
            latmctx->m4ac.channels, latmctx->m4ac.sample_rate);
    return 0;
}


int get_aac_latm_length(struct latm_parse_data *latmctx,uint8_t *packet, int packetLen)
{
    int i = 0;
    int length = 0;
    uint8_t tmp;
    int begin = latmctx->mux_slot_length_offset /8;

    if (packetLen < CHECK_MUX_HEADER_MAX + MUX_SLOT_LENGTH_SIZE) return -1;
    if (!check_latm_mux_header(packet, packetLen)) {
        printf("%s, faile to check_latm_mux_header :", __func__);
        for (i = 0; i < CHECK_MUX_HEADER_MAX; i++) {
            printf("0x%x,",packet[i]);
        }
        printf("\n");
        return -1;
    }
    i = 0;
    do {
        tmp = packet[begin + i];//mux slot length
        length += tmp;
        i++;
        if (i > MUX_SLOT_LENGTH_SIZE) return -1;//not support
    } while(tmp == 0xff);

    length += i + begin;

    return length;
}

static uint8_t latm_mux_header[CHECK_MUX_HEADER_MAX] =
{
    0x47,
    0xfc,
    0x00,
    0x00,
    0xb0,
    0x90,
    0x80,
    0x03,
    0x00,
};

void addADTStoPacket(uint8_t *packet, int packetLen)
{
    int profile = 2; // AAC LC
    int freqIdx = 4; // 44.1KHz
    int chanCfg = 2;

    // fill in ADTS data
    packet[0] = 0xFF;
    packet[1] = 0xF1;
    packet[2] = (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    packet[3] = (((chanCfg & 3) << 6) + (packetLen >> 11));
    packet[4] = ((packetLen & 0x7FF) >> 3);
    packet[5] = (((packetLen & 7) << 5) + 0x1F);
    packet[6] = 0xFC;
}

int save_latm_mux_header(uint8_t *packet, int packetLen)
{
    int i;

    if (packetLen < CHECK_MUX_HEADER_MAX) return -1;
    printf("%s, :", __func__);
    for (i = 0; i < CHECK_MUX_HEADER_MAX; i++) {
         latm_mux_header[i] =  packet[i];
         printf("0x%x,",latm_mux_header[i]);
    }
    printf("\n");

    return 0;
}


int check_latm_mux_header(uint8_t *packet, int packetLen)
{
    int i;
    int ret = 0;

    if (packetLen < CHECK_MUX_HEADER_MAX) return 0;
    for (i = 0; i < CHECK_MUX_HEADER_MAX; i++) {
        ret += (latm_mux_header[i] ==  packet[i]) ? 1 : 0;
    }
    if (ret == CHECK_MUX_HEADER_MAX) return 1;

    return 0;
}

int find_latm_mux_header(uint8_t *packet, int packetLen)
{
    int i;

    if (packetLen < CHECK_MUX_HEADER_MAX) return -1;
    for (i =0; i <= packetLen - CHECK_MUX_HEADER_MAX; i++) {
        if ((packet[i] == latm_mux_header[0])
                && (packet[i + 1] == latm_mux_header[1])
                && (packet[i + 2] == latm_mux_header[2])
                && (packet[i + 3] == latm_mux_header[3])
                && (packet[i + 4] == latm_mux_header[4])
                && (packet[i + 5] == latm_mux_header[5])
                && (packet[i + 6] == latm_mux_header[6])
                && (packet[i + 7] == latm_mux_header[7])
                && (packet[i + 8] == latm_mux_header[8])
                ) {
            //printf("pos=%d, packetLen=%d\n", i , packetLen);
            return i;
        }
    }

    return -1;
}

void addLATMSyncStreamtoPacket(uint8_t *packet, int packetLen)
{

    packet[0] = (0x2b7 >> 3);//LOAS_SYNC_WORD
    packet[1] = ((0x2b7 << 5) & 0xE0) | ((packetLen >> 8) & 0x1f);
    packet[2] = packetLen;
}


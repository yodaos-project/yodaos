#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "adpcm_decoder.h"
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>

typedef struct myFFmpeg {
    AVCodecContext *pCodecCtx;
    AVFrame *pFrame;
    struct SwrContext *au_convert_ctx;
    int out_buffer_size;
} myFFmpeg;

void *adpcm_decoder_create(int sample_rate, int channels, int bit)
{
    myFFmpeg *pComponent = (myFFmpeg *)malloc(sizeof(myFFmpeg));
    if (!pComponent)
       return NULL;

    memset(pComponent, 0, sizeof(myFFmpeg));
    av_register_all();
    AVCodec *pCodec = avcodec_find_decoder(AV_CODEC_ID_ADPCM_IMA_DAT4);
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
    pComponent->pCodecCtx->channel_layout = AV_CH_LAYOUT_MONO;
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

int adpcm_decode_frame(void *pParam, unsigned char *pData, int nLen,  adpcm_decode_cb completion, void *caller_handle)
{
    myFFmpeg *pAACD = (myFFmpeg *)pParam;
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
        pAACD->au_convert_ctx = swr_alloc_set_opts(pAACD->au_convert_ctx, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, pAACD->pCodecCtx->sample_rate,
                                      pAACD->pFrame->channel_layout, pAACD->pFrame->format, pAACD->pFrame->sample_rate, 0, NULL);
        swr_init(pAACD->au_convert_ctx);
        int out_linesize;
        int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);
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

void adpcm_decode_close(void *pParam)
{
    myFFmpeg *pComponent = (myFFmpeg *)pParam;
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

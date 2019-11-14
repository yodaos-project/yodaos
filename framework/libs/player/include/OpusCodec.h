#ifndef OPUS_CODEC_H
#define OPUS_CODEC_H

#include <opus/opus.h>
#include <opus/opus_multistream.h>
#include <opus/opus_types.h>
#include <stdio.h>
#include <utils/String8.h>

typedef enum
{
    SR_8K = 8000,
    SR_16K = 16000,
    SR_24K = 24000,
    SR_48K = 48000,
} OpusSampleRate;

typedef enum
{
    AUDIO = 2048,
    VOIP = 2049,
} OpusApplication;

class OpusCodec
{
public:
    OpusCodec(int sample_rate, int channels, int bitrate, int application);
    ~OpusCodec();
    long mEncoder;
    long mDecoder;

    long native_opus_encoder_create(int sample_rate, int channels, int bitrate, int application);
    long native_opus_decoder_create(int sample_rate, int channels, int bitrate);
    uint32_t native_opus_encode(long enc, const char *in, size_t length, unsigned char *&opus);
    uint32_t native_opus_decode(long dec, const char *in, size_t length, char *&pcm_out);
};

#endif

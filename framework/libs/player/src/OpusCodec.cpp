#include "OpusCodec.h"
#include <cstdlib>
#include <math.h>
#include "utils/misc.h"
#include "rklog/RKLog.h"
#include <fstream>
typedef struct
{
    OpusEncoder *enc;
    OpusDecoder *dec;
    int sample_rate;
    int channels;
    int application;
    int duration;
    int opusBitrate;
    int pcmBitrate;
    int pcmFrameSize;
    int opusFrameSize;
} opus_st;

OpusCodec::OpusCodec(int sampleRate, int channels, int bitrate, int application)
{
    if ((mDecoder = native_opus_decoder_create(sampleRate, channels, bitrate)) == 0)
    {
        RKLog("decoder create failed");
    }
    if ((mEncoder = native_opus_encoder_create(sampleRate, channels, bitrate, application)) == 0)
    {
        RKLog("endecoder create failed");
    }
}

OpusCodec::~OpusCodec()
{
    if (mEncoder)
    {
        opus_encoder_destroy(((opus_st *)mEncoder)->enc);
        free((opus_st *)mEncoder);
    }
    if (mDecoder)
    {
        opus_decoder_destroy(((opus_st *)mDecoder)->dec);
        free((opus_st *)mDecoder);
    }
}

long OpusCodec::native_opus_encoder_create(int sampleRate, int channels, int bitrate, int application)
{
    int error;
    opus_st *encoder = (opus_st *)calloc(1, sizeof(opus_st));
    encoder->enc = opus_encoder_create(sampleRate, channels, application, &error);
    if (error != OPUS_OK)
    {
        RKLog("opus encoder create error %s\n", opus_strerror(error));
        free(encoder);
        return 0;
    }
    else
    {
        opus_encoder_ctl(encoder->enc, OPUS_SET_VBR(0));
        opus_encoder_ctl(encoder->enc, OPUS_SET_BITRATE(bitrate));

        encoder->sample_rate = sampleRate;
        encoder->channels = channels;
        encoder->application = application;
        encoder->duration = 20;
        encoder->opusBitrate = bitrate;
        encoder->pcmBitrate = sampleRate * channels * sizeof(opus_int16) * 8;
        encoder->pcmFrameSize = encoder->duration * sampleRate / 1000;
        encoder->opusFrameSize = sizeof(opus_int16) * encoder->pcmFrameSize * bitrate / encoder->pcmBitrate;
        return (long)encoder;
    }
}

long OpusCodec::native_opus_decoder_create(int sampleRate, int channels, int bitrate)
{
    int error;
    opus_st *decoder = (opus_st *)calloc(1, sizeof(opus_st));

    decoder->dec = opus_decoder_create(sampleRate, channels, &error);
    if (error != OPUS_OK)
    {
        RKLog("decoder create error %s\n", opus_strerror(error));
        free(decoder);
        return 0;
    }
    else
    {
        decoder->sample_rate = sampleRate;
        decoder->channels = channels;
        decoder->duration = 20;
        decoder->opusBitrate = bitrate;
        decoder->pcmBitrate = sampleRate * channels * sizeof(opus_int16) * 8;
        decoder->pcmFrameSize = decoder->duration * sampleRate / 1000;
        decoder->opusFrameSize = sizeof(opus_int16) * decoder->pcmFrameSize * bitrate / decoder->pcmBitrate;
        opus_decoder_ctl(decoder->dec, OPUS_SET_BITRATE(bitrate));
        opus_decoder_ctl(decoder->dec, OPUS_SET_VBR(0));
        return (long)decoder;
    }
}

uint32_t OpusCodec::native_opus_encode(long enc, const char *in, size_t length, unsigned char *&opus)
{
    opus_st *encoder = (opus_st *)enc;
    opus_int16 *pcm = (opus_int16 *)in;
    const uint32_t len = (uint32_t)length;
    uint32_t pcmFrameSize = encoder->pcmFrameSize;
    uint32_t opusFrameSize = encoder->opusFrameSize;
    uint32_t opusLength = len / pcmFrameSize / sizeof(opus_int16) * opusFrameSize;
    opus = new unsigned char[opusLength];
    RKLog("opus encode len(%d), pcmFrameSize(%d) opusFrameSize(%d)", len, pcmFrameSize, opusFrameSize);

    uint32_t totalLen = 0;
    int outLen = 0;
    unsigned char *opusBuf = opus;
    uint32_t encodedSize = 0;
    opus_int16 *pcmOrig = pcm;
    uint32_t roundCount = len / sizeof(opus_int16) / pcmFrameSize;
    while (encodedSize < roundCount)
    {
        outLen = opus_encode(encoder->enc, pcm, pcmFrameSize, opusBuf, opusFrameSize);

        if (outLen < 0)
        {
            RKLog("frame_size(%d) failed: %s", pcmFrameSize, opus_strerror(outLen));
            outLen = 0;
            break;
        }
        else if (outLen != (int)opusFrameSize)
        {
            RKLog(
                "Something abnormal happened outLen(%d) pcmFrameSize(%d), check "
                "it!!!",
                outLen, pcmFrameSize);
            delete[] opus;
            return 0;
        }
        pcm += pcmFrameSize;
        opusBuf += outLen;
        totalLen += outLen;
        encodedSize++;
    }
    // delete[] opus; //need release opus
    return opusLength;
}

uint32_t OpusCodec::native_opus_decode(long dec, const char *in, size_t length, char *&pcmOut)
{
    opus_st *decoder = (opus_st *)dec;
    unsigned char *opus = (unsigned char *)in;
    const int len = (const int)length;

    int opusFrameSize = decoder->opusFrameSize;
    int pcmFrameSize = decoder->pcmFrameSize;
    int compressRatio = sizeof(opus_int16) * decoder->pcmFrameSize / decoder->opusFrameSize;
    uint32_t pcmLength = len * compressRatio;
    opus_int16 *pcm = new int16_t[pcmLength];

    RKLog(
        "opus decode len(%d), compressRatio(%d), opusFrameSize(%d), "
        "pcmFrameSize(%d)",
        len, compressRatio, opusFrameSize, pcmFrameSize);

    int totalLen = 0;
    int decodedSize = 0;
    int outLen = 0;
    unsigned char *opus_orig = opus;
    opus_int16 *pcmBuf = pcm;
    while (decodedSize++ < (len / opusFrameSize))
    {
        outLen = opus_decode(decoder->dec, opus, opusFrameSize, pcmBuf, pcmFrameSize, 0);
        if (outLen < 0)
        {
            RKLog("opus decode len(%d) opus_len(%d) %s", len, opusFrameSize, opus_strerror(outLen));
            break;
        }
        else if (outLen != pcmFrameSize)
        {
            RKLog("VBS not support!! outLen(%d) pcmFrameSize(%d)", outLen, pcmFrameSize);
            break;
        }
        opus += opusFrameSize;
        pcmBuf += outLen;
        totalLen += outLen;
    }

    RKLog("opus decoded data total len = %d", totalLen);
    pcmOut = (char *)pcm;
    // return sizeof(opus_int16)*pcmLength;
    return pcmLength;
}

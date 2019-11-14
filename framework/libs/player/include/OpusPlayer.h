#ifndef OPUS_PLAYER_H
#define OPUS_PLAYER_H

#include "OpusCodec.h"
#include <pulse/error.h>
#include <pulse/simple.h>

class OpusPlayer
{
public:
    OpusPlayer();
    OpusPlayer(OpusSampleRate sampleRate, int channel, int bits, char *appName, char *streamType);
    ~OpusPlayer();

    void startOpusPlayer(const char *data, size_t length);
    void resetOpusPlayer();
    void drain(bool holdconnect = true);
    void encodePcm(const char *data, size_t length, char *outFileName);

private:
    void init();
    pa_simple *mPa;

    /* Default Sample format to use */
    pa_sample_spec mPaSS = {
        .format = PA_SAMPLE_S16LE,
        .rate = 24000,
        .channels = 1,
    };

    int mChannels = 1;
    int mBits = 16000;
    OpusSampleRate mSampleRate = OpusSampleRate::SR_24K;
    OpusApplication mApplication = OpusApplication::AUDIO;
    char *mAppName;
    char *mStreamType;
    OpusCodec *mOpus;
    size_t mSilentDataLen;
    uint8_t *mSilentData;
};

#endif

#include "OpusPlayer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <pulse/def.h>
#include <pulse/error.h>
#include <pulse/sample.h>
#include <pulse/simple.h>

#include "rklog/RKLog.h"

#include <mutex>  // std::mutex, std::unique_lock
#include <thread> // std::thread
#include <fstream>

static std::mutex gTtsPlayerMutex;
static char DEFAULT_APP_NAME[] = "ttsplayer";
static char DEFAULT_STREAM_TYPE[] = "tts";

OpusPlayer::OpusPlayer()
{
    std::lock_guard<std::mutex> lock(gTtsPlayerMutex);
    mAppName = DEFAULT_APP_NAME;
    mStreamType = DEFAULT_STREAM_TYPE;
    init();
}

OpusPlayer::OpusPlayer(OpusSampleRate sampleRate, int channel, int bits, char *appName, char *streamType)
{
    std::lock_guard<std::mutex> lock(gTtsPlayerMutex);

    mSampleRate = sampleRate;

    if (channel != 2 && channel != 1)
    {
        RKLog("channel number is wrong. Use default 1. %d");
        mChannels = 1;
    }
    else
    {
        mChannels = channel;
    }

    mBits = bits;

    if (!appName)
    {
        RKLog("App name is NULL! Use default name ttsplayer.");
        mAppName = DEFAULT_APP_NAME;
    }
    else
    {
        mAppName = appName;
    }

    if (!streamType)
    {
        RKLog("Stream type is NULL! Use default name tts.");
        mStreamType = DEFAULT_STREAM_TYPE;
    }
    else
    {
        mStreamType = streamType;
    }

    mPa = NULL;
    mOpus = NULL;
    init();
}

OpusPlayer::~OpusPlayer()
{
    if (mPa)
    {
        pa_simple_free(mPa);
        mPa = NULL;
    }
    if (mOpus)
    {
        delete mOpus;
        mOpus = NULL;
    }
}

void OpusPlayer::init()
{
    int error;

    mPaSS.rate = (int)mSampleRate;
    mPaSS.channels = mChannels;
    if (mPa)
    {
        pa_simple_free(mPa);
        mPa = NULL;
    }

    /* Create a new playback stream */
    if (!(mPa = pa_simple_new(NULL, mAppName, PA_STREAM_PLAYBACK, NULL, mStreamType, &mPaSS, NULL, NULL, &error)))
    {
        RKLog("pa_simple_new() failed: %s\n", pa_strerror(error));
        pa_simple_free(mPa);
        mPa = NULL;
    }

    if (mOpus)
    {
        delete mOpus;
        mOpus = NULL;
    }
    mOpus = new OpusCodec((int)mSampleRate, mChannels, mBits, (int)mApplication);
    mSilentDataLen = mPaSS.rate * mPaSS.channels * 2 / 100 * 4; // 40ms的安静声音
    RKLog("%s mSilentDataLen %ld", __func__, mSilentDataLen);
    mSilentData = new uint8_t[mSilentDataLen];
    memset(mSilentData, 0, mSilentDataLen);
}

void OpusPlayer::startOpusPlayer(const char *data, size_t length)
{
    std::lock_guard<std::mutex> lock(gTtsPlayerMutex);

    int error;
    char *pcmOut;
    uint32_t pcmOutLength;
    if (!mPa)
    {
        if (!(mPa = pa_simple_new(NULL, mAppName, PA_STREAM_PLAYBACK, NULL, mStreamType, &mPaSS, NULL, NULL, &error)))
        {
            RKLog("pa_simple_new() failed: %s\n", pa_strerror(error));
            pa_simple_free(mPa);
            mPa = NULL;
            return;
        }
    }
    if (!mOpus)
    {
        mOpus = new OpusCodec((int)mSampleRate, mChannels, mBits, (int)mApplication);
    }
    pcmOutLength = mOpus->native_opus_decode(mOpus->mDecoder, data, length, pcmOut);
    if (mPa)
    {
        pa_simple_write(mPa, pcmOut, pcmOutLength, &error);
    }

    delete[] pcmOut;
}

void OpusPlayer::encodePcm(const char *data, size_t length, char *outFileName)
{
    std::lock_guard<std::mutex> lock(gTtsPlayerMutex);

    if (data == NULL || length == 0)
    {
        RKLog("encodePcm parameter is invalid!");
        return;
    }
    std::ofstream ofs(outFileName, std::ifstream::out | std::ifstream::binary | std::ifstream::app);

    if (!ofs.good())
    {
        RKLog("Can't open output file!");
        return;
    }
    unsigned char *opusOut;

    int opusOutLength = mOpus->native_opus_encode(mOpus->mEncoder, data, length, opusOut);
    ofs.write(reinterpret_cast<char *>(opusOut), opusOutLength);
    ofs.close();

    delete[] opusOut;
}

void OpusPlayer::resetOpusPlayer()
{
    std::lock_guard<std::mutex> lock(gTtsPlayerMutex);

    if (mPa)
    {
        int err = 0;
        pa_simple_flush(mPa, &err);
        RKLog("pa_simple_flush err %d", err);
        pa_simple_write(mPa, mSilentData, mSilentDataLen, &err);
        pa_simple_drain(mPa, &err);
    }

    if (mOpus)
    {
        delete mOpus;
        mOpus = NULL;
    }
}

void OpusPlayer::drain(bool holdconnect)
{
    std::lock_guard<std::mutex> lock(gTtsPlayerMutex);

    if (mPa)
    {
        int err = 0;
        pa_simple_write(mPa, mSilentData, mSilentDataLen, &err);
        RKLog("pa_simple_drain start");
        pa_simple_drain(mPa, &err);
        RKLog("pa_simple_drain err %d", err);
    }
    if (!holdconnect && mPa)
    {
        pa_simple_free(mPa);
        mPa = NULL;
    }
    if (mOpus)
    {
        delete mOpus;
        mOpus = NULL;
    }
}

#include "SimplePlayer.h"

#include <errno.h>
#include <fcntl.h>
#include <pulse/def.h>
#include <pulse/sample.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rklog/RKLog.h"
#define printf RKLog
#undef LOG_TAG
#define LOG_TAG "SIMPLAYER"

static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;

SimplePlayer::SimplePlayer(const pa_sample_spec *sampleSpec, const char *tag)
{
    int error;
    mTag = new char[strlen(tag) + 1];
    strcpy(mTag, tag);
    pthread_mutex_lock(&sMutex);
    if (sampleSpec != NULL)
    {
        mSampleSpec.format = sampleSpec->format;
        mSampleSpec.rate = sampleSpec->rate;
        mSampleSpec.channels = sampleSpec->channels;
    }
    /** Create a new playback stream */
    if (!(mPaSimple
          = pa_simple_new(NULL, "simpleplay", PA_STREAM_PLAYBACK, NULL, mTag, &mSampleSpec, NULL, NULL, &error)))
    {
        ALOGW("pa_simple_new() failed: %s\n", pa_strerror(error));
        mPaSimple = NULL;
    }
    pthread_mutex_unlock(&sMutex);
}

SimplePlayer::SimplePlayer(const char *tag, const pa_sample_format_t format, const uint32_t rate, const uint8_t channel)
{
    int error;
    mTag = new char[strlen(tag) + 1];
    strcpy(mTag, tag);
    pthread_mutex_lock(&sMutex);
    mSampleSpec.format = format;
    mSampleSpec.rate = rate;
    mSampleSpec.channels = channel;
    /** Create a new playback stream */
    if (!(mPaSimple
          = pa_simple_new(NULL, "simpleplay", PA_STREAM_PLAYBACK, NULL, mTag, &mSampleSpec, NULL, NULL, &error)))
    {
        ALOGW("pa_simple_new() failed: %s\n", pa_strerror(error));
        mPaSimple = NULL;
    }
    pthread_mutex_unlock(&sMutex);
}

const char *SimplePlayer::getTag()
{
    return mTag;
}

const pa_sample_spec *SimplePlayer::getSample_spec()
{
    return &mSampleSpec;
}

SimplePlayer::~SimplePlayer()
{
    pthread_mutex_lock(&sMutex);
    if (mPaSimple)
    {
        pa_simple_free(mPaSimple);
        mPaSimple = NULL;
    }
    if (mTag)
    {
        delete[] mTag;
        mTag = NULL;
    }
    pthread_mutex_unlock(&sMutex);
}

void SimplePlayer::destory()
{
    pthread_mutex_lock(&sMutex);
    if (mPaSimple)
    {
        pa_simple_free(mPaSimple);
        mPaSimple = NULL;
    }
    if (mTag)
    {
        delete[] mTag;
        mTag = NULL;
    }
    pthread_mutex_unlock(&sMutex);
}

void SimplePlayer::reset()
{
    pthread_mutex_lock(&sMutex);
    if (mPaSimple)
    {
        int err = 0;
        pa_simple_flush(mPaSimple, &err);
        RKLog("pa_simple_flush err %d", err);
    }
    pthread_mutex_unlock(&sMutex);
}

void SimplePlayer::drain()
{
    pthread_mutex_lock(&sMutex);
    if (mPaSimple)
    {
        int err = 0;
        RKLog("pa_simple_drain start");
        pa_simple_drain(mPaSimple, &err);
        RKLog("pa_simple_drain err %d", err);
    }
    pthread_mutex_unlock(&sMutex);
}

void SimplePlayer::play(const void *data, const size_t length)
{
    pthread_mutex_lock(&sMutex);
    int error;
    if ((!data) || (length == 0))
    {
        RKLog("empty file");
        pthread_mutex_unlock(&sMutex);
        return;
    }
    if (mPaSimple == NULL)
    {
        if (!(mPaSimple
              = pa_simple_new(NULL, "simpleplayer", PA_STREAM_PLAYBACK, NULL, mTag, &mSampleSpec, NULL, NULL, &error)))
        {
            RKLog("pa_simple_new() failed: %s\n", pa_strerror(error));
            mPaSimple = NULL;
        }
    }
    if (mPaSimple)
    {
        RKLog("pa_simple_write");
        int ret = pa_simple_write(mPaSimple, data, length, &error);
        if (ret < 0)
        {
            RKLog("pa_simple_write failed: %s\n", pa_strerror(error));
        }
    }
    pthread_mutex_unlock(&sMutex);
}

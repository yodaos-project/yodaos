#include <fcntl.h>
#include <stdlib.h>
#include <sndfile.h>
#include <string.h>

#include <map>
#include <string>

#include <SimplePlayer.h>
#include "rklog/RKLog.h"
#define BLOCK_SZIE 4096
/** Maximum number of files that can be loaded */
#define FILE_COUNT_MAX 10
/** Maximum size of a single file that can be loaded */
#define FILE_SIZE_MAX 500000
static bool gLongLink = true;
static bool gStop = false;
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
SimplePlayer *gWavSimplePlayer = NULL;

class WavDecode
{
public:
    void *mData;
    size_t mLength;
    pa_sample_spec mSampleSpec;

    WavDecode()
        : mData(NULL)
        , mLength(0)
    {
    }

    WavDecode(const WavDecode &src)
    {
        mSampleSpec.rate = src.mSampleSpec.rate;
        mSampleSpec.channels = src.mSampleSpec.channels;
        mSampleSpec.format = src.mSampleSpec.format;
        mLength = src.mLength;
        if (src.mLength != 0)
        {
            mData = malloc(mLength);
            memcpy(mData, src.mData, mLength);
        }
    }

    ~WavDecode()
    {
        if (mData != NULL)
        {
            free(mData);
        }
        mData = NULL;
        RKLog("deconstruction WavDecode");
    }

    int decodeFile(const char *filePath)
    {
        RKLog("decodeFile %s", filePath);
        if (NULL == filePath)
        {
            RKLog("decodeFile file path empty");
            return -1;
        }
        SF_INFO sfinfo = { 0 };
        memset(&sfinfo, 0, sizeof(sfinfo));
        SNDFILE *sndfile = sf_open(filePath, SFM_READ, &sfinfo);
        if (!sndfile)
        {
            RKLog("sndfile failed to open file %s", filePath);
            return -1;
        }
        mSampleSpec.rate = (uint32_t)sfinfo.samplerate;
        mSampleSpec.channels = (uint8_t)sfinfo.channels;
        RKLog("channels：%d", mSampleSpec.channels);
        RKLog("rate：%d", mSampleSpec.rate);
        int items = sfinfo.channels * sfinfo.frames;
        sf_seek(sndfile, 0, SEEK_SET);
        switch (sfinfo.format & 0xFF)
        {
            case SF_FORMAT_PCM_16:
            case SF_FORMAT_PCM_U8:
            case SF_FORMAT_PCM_S8:
                mSampleSpec.format = PA_SAMPLE_S16NE;
                mData = (short *)malloc(items * sizeof(short));
                mLength = items * sizeof(short);
                sf_readf_short(sndfile, (short *)mData, sfinfo.frames);
                break;
            case SF_FORMAT_ULAW:
                mSampleSpec.format = PA_SAMPLE_ULAW;
                break;
            case SF_FORMAT_ALAW:
                mSampleSpec.format = PA_SAMPLE_ALAW;
                break;
            case SF_FORMAT_FLOAT:
            case SF_FORMAT_DOUBLE:
            default:
                mSampleSpec.format = PA_SAMPLE_FLOAT32NE;
                mData = (float *)malloc(items * sizeof(float));
                mLength = items * sizeof(float);
                sf_readf_float(sndfile, (float *)mData, sfinfo.frames);
                break;
        }
        sf_close(sndfile);
        return 0;
    }
};
typedef std::map<std::string, WavDecode *> FileMap;
FileMap gDatas;
static WavDecode *gNextData = NULL;
static WavDecode *gWav = NULL;

void clearCacheFile()
{
    pthread_mutex_lock(&gMutex);
    if (gDatas.empty())
    {
        pthread_mutex_unlock(&gMutex);
        return;
    }
    for (FileMap::iterator it = gDatas.begin(); it != gDatas.end(); it++)
    {
        if (it->second)
        {
            delete it->second;
            it->second = NULL;
        }
    }
    gDatas.clear();
    pthread_mutex_unlock(&gMutex);
    return;
}

int eraseCacheFile(const char *filePath = NULL)
{
    pthread_mutex_lock(&gMutex);
    int ret = 0;
    if (filePath != NULL)
    {
        FileMap::iterator it = gDatas.find(std::string(filePath));
        if (it != gDatas.end())
        {
            RKLog("erase begin");
            if (it->second)
            {
                delete it->second;
                it->second = NULL;
            }
            gDatas.erase(it);
        }
    }
    else
        ret = -1;
    pthread_mutex_unlock(&gMutex);
    return ret;
}
/** Add new file && update old, you can only update if the number of lists is more than FILE_COUNT_MAX,
    If file size more than FILE_SIZE_MAX, cound't preload into memory */
int prePrepareWavPlayer(const char *fileNames[], int num)
{
    pthread_mutex_lock(&gMutex);
    if (NULL == fileNames || num <= 0)
    {
        RKLog("filenames  list is NULL");
        pthread_mutex_unlock(&gMutex);
        return -1;
    }
    size_t size = gDatas.size();
    for (size_t i = 0; i < (unsigned int)num; i++)
    {
        if (fileNames[i] != NULL)
        {
            FileMap::iterator it = gDatas.find(std::string(fileNames[i]));
            if (it != gDatas.end())
            {
                if (it->second)
                {
                    delete it->second;
                    it->second = NULL;
                }
                gDatas.erase(it);
                size--;
            }

            if (size >= FILE_COUNT_MAX)
            {
                RKLog("%d files have to be loaded, and file list not have %s,coutinue,update other", size,
                      fileNames[i]);
                continue;
            }
            else
            {
                WavDecode *msg = new WavDecode();
                int result = msg->decodeFile(fileNames[i]);
                if (result < 0)
                {
                    RKLog("decodeFile %s fail!!", fileNames[i]);
                    delete msg;
                }
                else
                {
                    if (msg->mLength > FILE_SIZE_MAX)
                    {
                        delete msg;
                        RKLog("Sorry, file %s is too big to preload into memory.", fileNames[i]);
                    }
                    else
                    {
                        gDatas.insert(std::make_pair(std::string(fileNames[i]), msg));
                        size++;
                        RKLog("insert sucesss! size = %d", size);
                    }
                }
            }
        }
    }
#if NEED_WAVPLAY_PRE_LINK
    if (NULL == gWavSimplePlayer)
    {
        gWavSimplePlayer = new SimplePlayer();
    }
#endif
    pthread_mutex_unlock(&gMutex);
    return 0;
}

static void doStop()
{
    if (gStop)
    {
        gStop = false;
        if (gNextData && gWav)
        {
            delete gWav;
        }
        gWav = gNextData = NULL;
        if (gWavSimplePlayer)
        {
            gWavSimplePlayer->reset();
            RKLog("do stop gWavSimplePlayer");
        }
    }
}
int prepareWavPlayer(const char *filePath = NULL, const char *tag = NULL, bool holdConnect = false)
{
    if (NULL == filePath)
    {
        RKLog("file path empty");
        return -1;
    }

    if (gWav && gWavSimplePlayer)
    {
        gStop = true;
    }
    else
    {
        gStop = false;
    }
    pthread_mutex_lock(&gMutex);
    doStop();
    gLongLink = holdConnect;
    gWav = gDatas[std::string(filePath)];
    if (!gWav)
    {
        gNextData = new WavDecode();
        int result = gNextData->decodeFile(filePath);
        if (result < 0)
        {
            pthread_mutex_unlock(&gMutex);
            return -1;
        }
        gWav = gNextData;
    }
    if (NULL == gWavSimplePlayer)
    {
        tag = tag == NULL ? "system" : tag;
        gWavSimplePlayer = new SimplePlayer(&gWav->mSampleSpec, tag);
    }
    else
    {
        if (strcmp((tag == NULL ? "system" : tag), gWavSimplePlayer->getTag()) == 0
            && pa_sample_spec_equal(&gWav->mSampleSpec, gWavSimplePlayer->getSample_spec()))
        {
            RKLog("reuse connect");
        }
        else
        {
            RKLog("unavailable connect");
            delete gWavSimplePlayer;
            gWavSimplePlayer = NULL;
            tag = tag == NULL ? "system" : tag;
            gWavSimplePlayer = new SimplePlayer(&gWav->mSampleSpec, tag);
        }
    }
    doStop();
    pthread_mutex_unlock(&gMutex);
    RKLog("prepareWavPlayer over");
    return 0;
}

int startWavPlayer()
{
    RKLog("play begin");
    pthread_mutex_lock(&gMutex);
    int result = 0;
    if (gWavSimplePlayer && gWav)
    {
        void *ptr = gWav->mData;
        size_t size = gWav->mLength;
        size_t n = 0;
        while (size > 0)
        {
            if (gStop)
            {
                RKLog("stop");
                gWavSimplePlayer->reset();
                break;
            }
            n = (size < BLOCK_SZIE) ? size : BLOCK_SZIE;
            gWavSimplePlayer->play(ptr, n);
            ptr = (char *)ptr + n;
            size = size - n;
        }
        if (!gStop)
        {
            RKLog("drain");
            gWavSimplePlayer->drain();
        }
        gStop = false;
        RKLog("play over");
        if (gNextData && gWav)
        {
            delete gWav;
        }
        gNextData = gWav = NULL;
        if (!gLongLink && gWavSimplePlayer)
        {
            delete gWavSimplePlayer;
            gWavSimplePlayer = NULL;
        }
    }
    else
    {
        result = -1;
    }
    pthread_mutex_unlock(&gMutex);
    return result;
}

void stopWavPlayer()
{
    RKLog("stop wavPlay begin");
    gStop = true;
}

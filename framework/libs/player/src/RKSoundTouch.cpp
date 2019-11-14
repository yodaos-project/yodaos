/**************************************************************
 * Copyright (c) 2018-2020,Hangzhou Rokid Tech. Co., Ltd.
 * All rights reserved.
 *
 * FileName: RKSoundTouch.c
 * Description: soundtouch lib c++ caller from c packet
 *
 * Date: 2018.11.23
 * Author: dairen
 * Modification: creat
 **************************************************************/
#include "soundtouch/SoundTouch.h"
#include "RKSoundTouch.h"
using namespace soundtouch;

struct tagSoundTouch
{
    SoundTouch soundtouch;
    float tempoDelta;
    bool isTempo;
    enum SOUNDTOUCH_STATE state;
};

void Soundtouch_SetState(struct tagSoundTouch *p, enum SOUNDTOUCH_STATE state)
{
    p->state = state;
}

enum SOUNDTOUCH_STATE Soundtouch_GetState(struct tagSoundTouch *p)
{
    return p->state;
}

bool Soundtouch_IsChangeTempo(struct tagSoundTouch *p)
{
    return p->isTempo;
}

void Soundtouch_SetTempoDelta(struct tagSoundTouch *p, float tempo)
{
    if (tempo <= -99.9f)
    {
        return;
    }
    p->tempoDelta = tempo;
    if (tempo == 0)
    {
        p->isTempo = false;
    }
    else
    {
        p->isTempo = true;
    }
}

float Soundtouch_GetTempoDelta(struct tagSoundTouch *p)
{
    return p->tempoDelta;
}

struct tagSoundTouch *GetInstance(void)
{
    struct tagSoundTouch *p;
    p = new struct tagSoundTouch;
    p->tempoDelta = 0;
    p->isTempo = false;
    p->state = SOUNDTOUCH_OVER;
    return p;
}

void ReleaseInstance(struct tagSoundTouch **ppSoundtouch)
{
    delete *ppSoundtouch;
    *ppSoundtouch = NULL;
}

void Soundtouch_SetSampleRate(struct tagSoundTouch *p, uint rate)
{
    p->soundtouch.setSampleRate(rate);
}

void Soundtouch_SetChannels(struct tagSoundTouch *p, uint channel)
{
    p->soundtouch.setChannels(channel);
}

void Soundtouch_SetTempoChange(struct tagSoundTouch *p, float newTempo)
{
    p->soundtouch.setTempoChange(newTempo);
}

void Soundtouch_SetPitchSemiTones(struct tagSoundTouch *p, int newPitch)
{
    p->soundtouch.setPitchSemiTones(newPitch);
}

void Soundtouch_SetRateChange(struct tagSoundTouch *p, float newRate)
{
    p->soundtouch.setRateChange(newRate);
}

bool Soundtouch_SetSetting(struct tagSoundTouch *p, int settingId, int value)
{
    return p->soundtouch.setSetting(settingId, value);
}

void Soundtouch_PutSamples(struct tagSoundTouch *p, const CALLC_SAMPLETYPE *samples, uint numSamples)
{
    p->soundtouch.putSamples((const SAMPLETYPE *)samples, numSamples);
}

uint Soundtouch_ReceiveSamples(struct tagSoundTouch *p, CALLC_SAMPLETYPE *outBuffer, uint maxSamples)
{
    return p->soundtouch.receiveSamples(outBuffer, maxSamples);
}

uint Soundtouch_NumSamples(struct tagSoundTouch *p)
{
    return p->soundtouch.numSamples();
}

void Soundtouch_Flush(struct tagSoundTouch *p)
{
    p->soundtouch.flush();
}

/**************************************************************
 * Copyright (c) 2018-2020,Hangzhou Rokid Tech. Co., Ltd.
 * All rights reserved.
 *
 * FileName: RKSoundTouch.h
 * Description: soundtouch lib c++ caller from c packet
 *
 * Date: 2018.11.23
 * Author: dairen
 * Modification: creat
 **************************************************************/
#ifndef _SOUNDTOUCH_C_H__
#define _SOUNDTOUCH_C_H__

struct tagSoundTouch;
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
typedef unsigned int uint;

/* Enable/disable anti-alias filter in pitch transposer (0 = disable) */
#define SETTING_USE_AA_FILTER 0

/* Pitch transposer anti-alias filter length (8 .. 128 taps, default = 32) */
#define SETTING_AA_FILTER_LENGTH 1

/* Enable/disable quick seeking algorithm in tempo changer routine
 (enabling quick seeking lowers CPU utilization but causes a minor sound
  quality compromising) */
#define SETTING_USE_QUICKSEEK 2

/* Time-stretch algorithm single processing sequence length in milliseconds. This determines
to how long sequences the original sound is chopped in the time-stretch algorithm.
See "STTypes.h" or README for more information. */
#define SETTING_SEQUENCE_MS 3

/* Time-stretch algorithm seeking window length in milliseconds for algorithm that finds the
best possible overlapping location. This determines from how wide window the algorithm
may look for an optimal joining location when mixing the sound sequences back together.
See "STTypes.h" or README for more information.*/
#define SETTING_SEEKWINDOW_MS 4

/* Time-stretch algorithm overlap length in milliseconds. When the chopped sound sequences
 are mixed back together, to form a continuous sound stream, this parameter defines over
 how long period the two consecutive sequences are let to overlap each other.
 See "STTypes.h" or README for more information.*/
#define SETTING_OVERLAP_MS 5

    /* 16bit integer sample type */
    typedef short CALLC_SAMPLETYPE;

    enum SOUNDTOUCH_STATE
    {
        SOUNDTOUCH_INIT,
        SOUNDTOUCH_PLAYING,
        SOUNDTOUCH_OVER
    };

    struct tagSoundTouch *GetInstance(void);

    void Soundtouch_SetState(struct tagSoundTouch *p, enum SOUNDTOUCH_STATE state);

    enum SOUNDTOUCH_STATE Soundtouch_GetState(struct tagSoundTouch *p);

    bool Soundtouch_IsChangeTempo(struct tagSoundTouch *p);

    void Soundtouch_SetTempoDelta(struct tagSoundTouch *p, float tempo);

    float Soundtouch_GetTempoDelta(struct tagSoundTouch *p);

    void ReleaseInstance(struct tagSoundTouch **ppSoundtouch);

    void Soundtouch_SetSampleRate(struct tagSoundTouch *p, uint rate);

    void Soundtouch_SetChannels(struct tagSoundTouch *p, uint channel);

    void Soundtouch_SetTempoChange(struct tagSoundTouch *p, float newTempo);

    void Soundtouch_SetPitchSemiTones(struct tagSoundTouch *p, int newPitch);

    void Soundtouch_SetRateChange(struct tagSoundTouch *p, float newRate);

    bool Soundtouch_SetSetting(struct tagSoundTouch *p, int settingId, int value);

    void Soundtouch_PutSamples(struct tagSoundTouch *p, const CALLC_SAMPLETYPE *samples, uint numSamples);

    uint Soundtouch_NumSamples(struct tagSoundTouch *p);

    uint Soundtouch_ReceiveSamples(struct tagSoundTouch *p, CALLC_SAMPLETYPE *outBuffer, uint maxSamples);

    void Soundtouch_Flush(struct tagSoundTouch *p);

#ifdef __cplusplus
}
#endif
#endif

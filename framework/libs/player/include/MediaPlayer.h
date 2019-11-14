/*
 * FFmpegMediaPlayer: A unified interface for playing audio files and streams.
 *
 * Copyright 2016 William Seemann
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RMEDIAPLAYER_H_
#define _RMEDIAPLAYER_H_

#include <pthread.h>
#include "Utils.h"

#if ROKIDOS_FEATURES_HAS_EQ_CTRL
#include <eq_ctrl/eq_ctrl.h>
#else
typedef enum
{
    EQ_TYPE_INIT = 0, /* normal type*/
    EQ_TYPE_DANCE,
    EQ_TYPE_JAZZ,
    EQ_TYPE_POP,
    EQ_TYPE_ROCK,
    EQ_TYPE_BALLED,
    EQ_TYPE_NOEQ,
    EQ_TYPE_LIGHT,
    EQ_TYPE_CLASSIC,
    EQ_TYPE_LOUD,
    EQ_TYPE_TEST,
    EQ_TYPE_NUM,
}rk_eq_type_t;
#endif

struct RPlayer;
typedef int status_t;

/**
 * MediaPlayer listener.
 *
 */
class MediaPlayerListener
{
public:
    /**
     * notify from rplayer.
     *
     * @param msg
     *        message type, valid type is defined in enum media_event_type in rk_ffplay.h.
     *
     * @param ext1
     *        extra message, for MEDIA_BLOCK_PAUSE_MODE means status of block pause mode
     *        defined in enum block_pause_mode_status in rk_ffplay.h; for MEDIA_PLAYING_STATUS
     *        means time interval; for MEDIA_ERROR means error type in Util.h.
     *
     * @param ext2
     *        extra message, for MEDIA_PLAYING_STATUS means decoded frame time.
     *
     * @param fromThread
     *        from rplayer or not.
     *
     */
    virtual void notify(int msg, int ext1, int ext2, int fromThread) = 0;
};

class MediaPlayer
{
public:
    /**
     * Default MediaPlayer Construction.
     *
     */
    MediaPlayer();
    /**
     * Construct MediaPlayer with volume stream type name.
     *
     * @param mediaTag
     *        volume stream type name, valid name includes "audio", "tts", "ring", "voiceCall",
     *        "alarm", "playback", "system".
     */
    MediaPlayer(const char *mediaTag);

    /**
     * Construct MediaPlayer with volume stream type name and block pause mode setting.
     *
     * @param mediaTag
     *        volume stream type name, valid name includes "audio", "tts", "ring", "voiceCall",
     *        "alarm", "playback", "system".
     * @param cacheDuration
     *        the cache size under block pause mode, unit is second.
     *
     * @param cacheMode
     *        enable/disable the block pause mode, default is false.
     */
    MediaPlayer(const char *mediaTag, double cacheDuration, bool cacheMode = false);

    /**
     * Default MediaPlayer destruction.
     *
     */
    ~MediaPlayer();

    /**
     * Initialize rplayer.
     *
     * @deprecated
     */
    static void globalInit(void);

    /**
     * Deinitialize rplayer.
     *
     * @deprecated
     */
    static void globalUninit(void);

    /**
     * Stop rplayer.
     *
     * @deprecated
     */
    void disconnect();

    /**
     * Set data source for rplayer to play, should be called before prepare() or prepareAsync().
     *
     * @param url
     *        the data source path.
     *
     * @param headers
     *        deprecated.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setDataSource(const char *url, const char *headers);

    /**
     * Set data source for rplayer to play, should be called before prepare() or prepareAsync().
     *
     * @param url
     *        the data source path.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setDataSource(const char *url);

    /**
     * Set MediaPlayer listener.
     *
     * @param listener
     *        MediaPlayerListener realized by user.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setListener(MediaPlayerListener *listener);

    /**
     * Get MediaPlayer listener.
     *
     * @return MediaPlayerListener
     *         return MediaPlayerListener.
     */
    MediaPlayerListener *getListener();

    /**
     * Enable display.
     *
     * @deprecated
     *             unsupported.
     */
    void enableDisplay(int enabled);

    /**
     * Set loop play mode.
     *
     * @param loop
     *        > 0 : enable loop mode, <= 0 : disable loop mode.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setLooping(int loop);

    /**
     * Setup ffmpeg and sdl environment, synchronous interface.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t prepare();

    /**
     * Setup ffmpeg and sdl environment, asynchronous interface.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t prepareAsync();

    /**
     * Start to play.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t start();

    /**
     * Stop playing, synchronous interface.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t stop();

    /**
     * Stop playing, asynchronous interface.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t stopAsync();

    /**
     * Pause playing.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t pause();

    /**
     * Resume playing.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t resume();

    /**
     * Check the playing status.
     *
     * @return bool
     *         return the status, false : no playing, true : playing.
     */
    bool isPlaying();

    /**
     * Check the paused status.
     *
     * @return bool
     *         return the status, false : no paused, true : paused.
     */
    bool isPaused();

    /**
     * Seek to specified playing position to play.
     *
     * @param msec
     *        the new playing position, unit is ms.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t seekTo(int msec);

    /**
     * Get current playing position.
     *
     * @param msec
     *        current playing position, unit is ms.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t getCurrentPosition(int *msec);

    /**
     * Get current playing position.
     *
     * @return long
     *         return current playing position, unit is ms.
     */
    long getCurrentPosition(void);

    /**
     * Get the total duration of current playing item.
     *
     * @param msec
     *        duration, unit is ms.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t getDuration(int *msec);

    /**
     * Get the total duration of current playing item.
     *
     * @return long
     *         return duration, unit is ms.
     */
    long getDuration(void);

    /**
     * Reset the rplayer status, should be called after stop() or stopAsync().
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t reset();

    /**
     * Set audio stream type.
     *
     * @deprecated
     */
    status_t setAudioStreamType(int type);

    /**
     * Check the loop mode.
     *
     * @return bool
     *         return the status, false : loop mode is off, true : loop mode is on.
     */
    bool isLooping();

    /**
     * Set volume.
     *
     * @param leftVolume
     *        volume, 0~100.
     *
     * @param rightVolume
     *        volume, 0~100, useless.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setVolume(float leftVolume, float rightVolume);

    /**
     * Set volume.
     *
     * @param volume
     *        volume, 0~100.
     *
     * @return long
     *         return the volume.
     */
    int setVolume(int volume);

    /**
     * Get volume.
     *
     * @return int
     *         return the volume, 0~100.
     */
    int getVolume(void);

    /**
     * Set audio session id.
     *
     * @deprecated
     */
    status_t setAudioSessionId(int sessionId);

    /**
     * Get audio session id.
     *
     * @deprecated
     */
    int getAudioSessionId();

    /**
     * Set aux effect level.
     *
     * @deprecated
     */
    status_t setAuxEffectSendLevel(float level);

    /**
     * Attach aux effect.
     *
     * @deprecated
     */
    int attachAuxEffect(int effectId);

    /**
     * Set next MediaPlayer.
     *
     * @deprecated
     */
    status_t setNextMediaPlayer(const MediaPlayer *player);

    /**
     * Get video width.
     *
     * @deprecated
     */
    status_t getVideoWidth(int *w);

    /**
     * Get video height.
     *
     * @deprecated
     */
    status_t getVideoHeight(int *h);

    /**
     * notify from rplayer.
     *
     * @param msg
     *        message type, valid type is defined in enum media_event_type in rk_ffplay.h.
     *
     * @param ext1
     *        extra message, for MEDIA_BLOCK_PAUSE_MODE means status of block pause mode
     *        defined in enum block_pause_mode_status in rk_ffplay.h; for MEDIA_PLAYING_STATUS
     *        means time interval.
     *
     * @param ext2
     *        extra message, for MEDIA_PLAYING_STATUS means decoded frame time.
     *
     * @param fromThread
     *        from rplayer or not.
     *
     */
    void notify(int msg, int ext1, int ext, int fromThread);

    /**
     * Set the tcp receiving buffer size during playing online music,
     * should be called before prepare() or perpareAsync().
     *
     * @param size
     *        cache size, unit is Byte, default is 20480Byte.
     *
     */
    void setRecvBufSize(int size);

    /**
     * Enable block pause mode.
     *
     * @param enabled
     *        enable/disable block pause mode.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t enableCacheMode(bool enabled);

    /**
     * Set the playback speed.
     *
     * @param tempoDelta
     *        playback speed, -50.0~100.0 means 0.5~2 speed.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setTempoDelta(float tempoDelta);

    /**
     * Enable reconnection at end of playing item, should be called before prepare() or perpareAsync().
     *
     * @param enabled
     *        enable/disable reconnection.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t enableReconnectAtEOF(int enabled);

    /**
     * Set max delay time of reconnection, if delay time exceeds this time, rplayer will report media error. Should be
     * called before prepare() or perpareAsync().
     *
     * @param seconds
     *        max delay time of reconnection, unit is second, default is 120s.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setReconnectDelayMax(int seconds);

    /**
     * Set eq mode, defined in enum rk_eq_type_t in eq_ctrl.h.
     *
     * @param type
     *        eq mode.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t setEqMode(rk_eq_type_t type);

    /**
     * Get current eq mode, defined in enum rk_eq_type_t in eq_ctrl.h.
     *
     * @param type
     *        eq mode.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t getCurEqMode(rk_eq_type_t *type);

    /**
     * Get supported eq mode.
     *
     * @param atype
     *        supported eq array.
     *
     * @return int
     *         return supported eq count.
     */
    static int getSupporteqMode(rk_eq_type_t **atype);

private:
    /** Pointer to internal RPlayer */
    RPlayer *mPlayer;
    /** current eq mode */
    rk_eq_type_t mCurEQType;

    /**
     * Clear.
     *
     * @deprecated
     */
    void clear_l();

    /**
     * Seek to specified playing position to play.
     *
     * @param msec
     *        the new playing position, unit is ms.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t seekTo_l(int msec);

    /**
     * Setup ffmpeg and sdl environment, asynchronous interface.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t prepareAsync_l();

    /**
     * Get the total duration of current playing item.
     *
     * @param msec
     *        duration, unit is ms.
     *
     * @return status_t
     *         return the status, 0 : success, < 0 : failure.
     */
    status_t getDuration_l(int *msec);

    /**
     * Set data source for rplayer to play.
     *
     * @deprecated
     */
    status_t setDataSource(RPlayer *state);

    /** Pointer to MediaPlayer listener */
    MediaPlayerListener *mListener;

    /** Pointer to data source path */
    char *mUrl;
};

#endif // _RMEDIAPLAYER_H_

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

#define LOG_TAG "FFmpegMediaPlayer"

#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <MediaPlayer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rk_ffplay.h"

using namespace std;

// static function
void MediaPlayer::globalInit(void)
{
    rplayer_global_init();
}

void MediaPlayer::globalUninit(void)
{
    rplayer_global_deinit();
}

MediaPlayer::MediaPlayer()
{
    mPlayer = rplayer_create();

    mListener = NULL;
    mUrl = NULL;
}

MediaPlayer::MediaPlayer(const char *mediaTag)
{
    mPlayer = rplayer_create();

    mListener = NULL;
    mUrl = NULL;

    rplayer_set_media_tag(mPlayer, mediaTag);
}

#if BLOCK_PAUSE_MODE
MediaPlayer::MediaPlayer(const char *mediaTag, double cacheDuration, bool cacheMode)
{
    mPlayer = rplayer_create();

    mListener = NULL;
    mUrl = NULL;

    rplayer_set_media_tag(mPlayer, mediaTag);

    rplayer_set_cache_duration(mPlayer, cacheDuration, cacheMode);
}
#else
MediaPlayer::MediaPlayer(const char *mediaTag, double cacheDuration, bool cacheMode)
{
    mPlayer = rplayer_create();

    mListener = NULL;
    mUrl = NULL;

    rplayer_set_media_tag(mPlayer, mediaTag);
}
#endif

MediaPlayer::~MediaPlayer()
{
    rplayer_destroy(mPlayer);
    mPlayer = 0;
    if (mUrl)
        free(mUrl);
}

void MediaPlayer::disconnect()
{
    rplayer_stop(mPlayer);
}

// always call with lock held
void MediaPlayer::clear_l()
{
}

static void notifyListener(void *clazz, int msg, int ext1, int ext2, int fromThread)
{
    MediaPlayer *mp = (MediaPlayer *)clazz;
    mp->notify(msg, ext1, ext2, fromThread);
}

status_t MediaPlayer::setListener(MediaPlayerListener *listener)
{
    mListener = listener;
    if (mPlayer)
    {
        rplayer_set_event_listener(mPlayer, notifyListener, this);
    }
    return 0;
}

MediaPlayerListener *MediaPlayer::getListener()
{
    return mListener;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    return setDataSource(url, NULL);
}

status_t MediaPlayer::setDataSource(const char *url, const char *headers)
{
    if (mUrl)
        free(mUrl);
    mUrl = strdup(url);
    return 0;
}

#if defined(__ANDROID__) || defined(ANDROID)
status_t MediaPlayer::setVideoSurface(ANativeWindow *native_window)
{
    //__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "setVideoSurface");
    Mutex::Autolock _l(mLock);
    if (state == 0)
        return NO_INIT;
    if (native_window != NULL)
        return ::setVideoSurface(&state, native_window);
    else
        return ::setVideoSurface(&state, NULL);
}
#endif

// must call with lock held
status_t MediaPlayer::prepareAsync_l()
{
    return rplayer_prepare_async(mPlayer, mUrl);
}

status_t MediaPlayer::prepare()
{
    return rplayer_prepare(mPlayer, mUrl);
}

status_t MediaPlayer::prepareAsync()
{
    return rplayer_prepare_async(mPlayer, mUrl);
}

status_t MediaPlayer::start()
{
    return rplayer_start(mPlayer);
}

status_t MediaPlayer::stop()
{
    setEqMode(EQ_TYPE_INIT);
    rplayer_stop(mPlayer);
    return 0;
}

status_t MediaPlayer::stopAsync()
{
    setEqMode(EQ_TYPE_INIT);
    rplayer_stop_async(mPlayer);
    return 0;
}

status_t MediaPlayer::pause()
{
    getCurEqMode(&mCurEQType);
    setEqMode(EQ_TYPE_INIT);
    rplayer_pause(mPlayer);
    return 0;
}

status_t MediaPlayer::resume()
{
    setEqMode(mCurEQType);
    rplayer_resume(mPlayer);
    return 0;
}

bool MediaPlayer::isPlaying()
{
    return rplayer_is_playing(mPlayer);
}

bool MediaPlayer::isPaused()
{
    return rplayer_is_paused(mPlayer);
}

void MediaPlayer::setRecvBufSize(int size)
{
    rplayer_set_recv_buf_size(mPlayer, size);
}
status_t MediaPlayer::getVideoWidth(int *w)
{
    // TODO
    if (w)
        w = 0;
    return 0;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
    // TODO
    if (h)
        h = 0;
    return 0;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
    int r = rplayer_current_position(mPlayer);
    if (msec)
        *msec = r;
    return 0;
}

long MediaPlayer::getCurrentPosition(void)
{
    return rplayer_current_position(mPlayer);
}

long MediaPlayer::getDuration(void)
{
    return rplayer_get_duration(mPlayer);
}

status_t MediaPlayer::getDuration_l(int *msec)
{
    int r = rplayer_get_duration(mPlayer);
    if (msec)
        *msec = r;
    return 0;
}

status_t MediaPlayer::getDuration(int *msec)
{
    return getDuration_l(msec);
}

status_t MediaPlayer::seekTo_l(int msec)
{
    return rplayer_seek_to(mPlayer, msec);
}

status_t MediaPlayer::setTempoDelta(float tempoDelta)
{
#if ROKIDOS_FEATURES_HAS_SOUNDTOUCH
    return rplayer_set_tempo(mPlayer, tempoDelta);
#else
    return -1;
#endif
}

status_t MediaPlayer::seekTo(int msec)
{
    return seekTo_l(msec);
}

status_t MediaPlayer::reset()
{
    rplayer_reset(mPlayer);
    return 0;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
    // TODO
    return 0;
}

status_t MediaPlayer::setLooping(int loop)
{
    rplayer_set_loop(mPlayer, loop);
    return 0;
}

bool MediaPlayer::isLooping()
{
    int r = rplayer_get_loop(mPlayer);
    return r > 1;
}

status_t MediaPlayer::setVolume(float leftVolume, float rightVolume)
{
    return rplayer_set_volume(mPlayer, leftVolume);
}

status_t MediaPlayer::setAudioSessionId(int sessionId)
{
    // TODO
    return 0;
}

int MediaPlayer::getAudioSessionId()
{
    return 0;
}

status_t MediaPlayer::setAuxEffectSendLevel(float level)
{
    // TODO
    return 0;
}

status_t MediaPlayer::attachAuxEffect(int effectId)
{
    // TODO
    return 0;
}

void MediaPlayer::notify(int msg, int ext1, int ext2, int fromThread)
{
    if (mListener)
        mListener->notify(msg, ext1, ext2, fromThread);
}

status_t MediaPlayer::setNextMediaPlayer(const MediaPlayer *next)
{
    // TODO
    return 0;
}

int MediaPlayer::setVolume(int volume)
{
    return rplayer_set_volume(mPlayer, volume);
}

void MediaPlayer::enableDisplay(int enabled)
{
    rplayer_enable_display(mPlayer, enabled);
}

int MediaPlayer::getVolume(void)
{
    return rplayer_get_volume(mPlayer);
}
// ranges from 0 - 128

#if BLOCK_PAUSE_MODE
status_t MediaPlayer::enableCacheMode(bool enabled)
{
    rplayer_enable_cache_mode(mPlayer, enabled);
    return 0;
}
#else
status_t MediaPlayer::enableCacheMode(bool enabled)
{
    return -1;
}
#endif

status_t MediaPlayer::enableReconnectAtEOF(int enabled)
{
    return rplayer_set_reconnect_at_eof(mPlayer, enabled);
}

status_t MediaPlayer::setReconnectDelayMax(int seconds)
{
    return rplayer_set_reconnect_delay_max(mPlayer, seconds);
}

status_t MediaPlayer::setEqMode(rk_eq_type_t type)
{
#if ROKIDOS_FEATURES_HAS_EQ_CTRL
    return rkSetEqMode(type);
#else
    return -1;
#endif
}

status_t MediaPlayer::getCurEqMode(rk_eq_type_t *type)
{
#if ROKIDOS_FEATURES_HAS_EQ_CTRL
    return rkGetEqMode(type);
#else
    return -1;
#endif
}

status_t MediaPlayer::getSupporteqMode(rk_eq_type_t **atype)
{
#if ROKIDOS_FEATURES_HAS_EQ_CTRL
    return rkGetSupportEqMode(atype);
#else
    return -1;
#endif
}

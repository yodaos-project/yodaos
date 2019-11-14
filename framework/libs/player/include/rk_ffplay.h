#ifndef _FFPLAY_H_
#define _FFPLAY_H_

#include "rklog/RKLog.h"
#include "Utils.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef unsigned int uint;
    typedef void (*RPlayerEventListener)(void *userdata, int event, int arg1, int arg2, int from_thread);
    void rplayer_global_init();
    void rplayer_global_deinit();

    typedef struct RPlayer RPlayer;

    RPlayer *rplayer_create();
    void rplayer_destroy(RPlayer *player);
    void rplayer_reset(RPlayer *player);

    /* call these functions before prepare*() */
    void rplayer_enable_display(RPlayer *is, int display_enabled);
    void rplayer_set_event_listener(RPlayer *player, RPlayerEventListener listener, void *opaque);
    void rplayer_set_loop(RPlayer *player, int loop);

    int rplayer_prepare(RPlayer *player, const char *file_name);
    int rplayer_prepare_async(RPlayer *player, const char *file_name);
    int rplayer_start(RPlayer *player);
    void rplayer_pause(RPlayer *player);
    void rplayer_resume(RPlayer *player);
    int rplayer_is_paused(RPlayer *player);
    int rplayer_is_playing(RPlayer *player);
    void rplayer_stop(RPlayer *player);
    void rplayer_stop_async(RPlayer *is);
    long rplayer_get_volume(RPlayer *is);
    long rplayer_set_volume(RPlayer *is, long volume);
    // void rplayer_wait_stop(RPlayer *player);
    void rplayer_set_recv_buf_size(RPlayer *is, int size);
    int rplayer_seek_to(RPlayer *player, int msecs);
    long rplayer_get_duration(RPlayer *player);
    long rplayer_get_playable_duration(RPlayer *player);
    long rplayer_current_position(RPlayer *player);
    int rplayer_get_loop(RPlayer *player);
    void rplayer_set_media_tag(RPlayer *player, const char *media_tag);
#if BLOCK_PAUSE_MODE
    void rplayer_set_cache_duration(RPlayer *is, double duration, int blockPauseMode);
    void rplayer_enable_cache_mode(RPlayer *is, int enabled);
#endif
    int rplayer_set_tempo(RPlayer *is, float tempo);
    int rplayer_set_reconnect_at_eof(RPlayer *is, int enabled);
    int rplayer_set_reconnect_delay_max(RPlayer *is, int seconds);
#ifdef __cplusplus
}
#endif
#endif

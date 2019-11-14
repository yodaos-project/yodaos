#ifndef VOLUMECONTROL_H
#define VOLUMECONTROL_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        STREAM_AUDIO = 0,
        STREAM_TTS,
        STREAM_RING,
        STREAM_VOICE_CALL,
        STREAM_ALARM,
        STREAM_PLAYBACK,
        STREAM_SYSTEM,
    } rk_stream_type_t;

    /*exclude alarm*/
    extern int rk_set_volume(int vol);
    extern int rk_get_volume();
    /* 0 unmute,1 mute */
    extern int rk_set_mute(bool mute);
    extern bool rk_is_mute();

    /* tag interfaces */

    extern int rk_set_tag_volume(const char *tag, int vol);
    extern int rk_get_tag_volume(const char *tag);
    /* 0 unmute,1 mute */
    extern int rk_set_tag_mute(const char *tag, bool mute);
    extern bool rk_is_tag_mute(const char *tag);

    extern int rk_get_tag_playing_status(const char *tag);

    /* stream interfaces */

    extern int rk_set_stream_volume(rk_stream_type_t stream_type, int vol);
    extern int rk_get_stream_volume(rk_stream_type_t stream_type);
    /* 0 unmute,1 mute */
    extern int rk_set_stream_mute(rk_stream_type_t stream_type, bool mute);
    extern bool rk_is_stream_mute(rk_stream_type_t stream_type);

    extern int rk_get_stream_playing_status(rk_stream_type_t stream_type);

    extern int rk_set_all_volume(int vol);
    extern int rk_get_all_volume();

#define set_all_volume rk_set_all_volume
#define get_all_volume rk_get_all_volume

    /* old interface */
    extern int set_app_volume(char *name, int vol);
    extern int get_app_volume(char *name);
    extern int set_sink_mute(int mute);

    extern int rk_changeto_pactl_volume(int vol);
    extern int rk_changeForm_pactl_volume(int vol);
    extern int rk_setCustomVolumeCurve(int cnt, int *VolArray);

    extern int rk_initCustomVolumeCurve();
#ifdef __cplusplus
}
#endif

#endif

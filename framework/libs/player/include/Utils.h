#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum RplayerMediaErrorType{

    MEDIA_ERROR_OUT_OF_MEMORY = 0,

    MEDIA_ERROR_IO = 1,

    MEDIA_ERROR_SERVER_DIED = 2,

    MEDIA_ERROR_TIMED_OUT = 3,

    MEDIA_ERROR_UNKNOWN = 4,

    MEDIA_ERROR_UNSUPPORTED = 5,

    MEDIA_INFO_NOT_SEEKABLE = 6,

    MEDIA_ERROR_MALFORMED = 7,

    }RplayerMediaErrorType;

    typedef enum block_pause_mode_status
    {
        BLOCK_PAUSE_ON = 0,
        BLOCK_PAUSE_OFF = 1,
    } block_pause_mode_type;

    typedef enum quit_block_pause_type
    {
        STREAM_END = 0,
        STREAM_RESUME = 1,
        STREAM_SEEK = 2,
        PACKET_COME = 3,
    } quit_block_pause_type;

    typedef enum media_event_type
    {
        MEDIA_NOP = 0, // interface test message
        MEDIA_PREPARED = 1,
        MEDIA_PLAYBACK_COMPLETE = 2,
        MEDIA_BUFFERING_UPDATE = 3,
        MEDIA_SEEK_COMPLETE = 4,
        MEDIA_POSITION = 5,
        MEDIA_PAUSE = 6,
        MEDIA_STOPED = 7,
        MEDIA_PLAYING = 8,
        MEDIA_BLOCK_PAUSE_MODE = 9,
        MEDIA_PLAYING_STATUS = 10,
        MEDIA_ERROR = 100,
        MEDIA_INFO = 200,
    } media_event_type;
#ifdef __cplusplus
}
#endif

#endif

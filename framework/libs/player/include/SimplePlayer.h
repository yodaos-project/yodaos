#ifndef _SIMPLEPLAYER_H_
#define _SIMPLEPLAYER_H_
#include <pulse/simple.h>
#include <pulse/error.h>

/**
 * Class Simpleplayer uses pulseaudio simple API and provides basic playback interfaces for applications.
 * It can only play music with PCM format.
 */
class SimplePlayer
{
public:
    /**
     * SimplePlayer construction, create a new playback stream and connect with pulseaudio service.
     *
     * @param tag
     *        the volume stream type of the player
     *
     * @param format
     *        pulseaudio sample format.
     *        The default value is PA_SAMPLE_S16LE(Signed 16 Bit PCM, little endian).
     *
     * @param rate
     *        sample rate, the default value is 16000.
     *
     * @param channels
     *        audio channels , the default value is 1 channel.
     */
    SimplePlayer(const char *tag = "system", const pa_sample_format_t format = PA_SAMPLE_S16LE,
                 const uint32_t rate = 16000, const uint8_t channels = 1);

    /**
     * SimplePlayer construction with pa_sample_spec(A sample format and attribute specification).
     *
     * @param sampleSpec
     *        sampleSpec contains sample format, sample rate and audio channels.
     *
     * @param tag
     *        the volume stream type of the player.
     */
    SimplePlayer(const pa_sample_spec *sampleSpec, const char *tag = "system");

    /**
     * SimplePlayer destruction.
     */
    ~SimplePlayer();

    /**
     * Play PCM audio.
     *
     * @param data
     *        pcm audio data.
     *
     * @param length
     *        The size of PCM audio data.
     */
    void play(const void *data, const size_t length);

    /**
     *  Reset the player, and throw away all data in buffers.
     */
    void reset();

    /**
     *  Drain the player to send data of buffers to codec in order to finish playing.
     */
    void drain();

    /**
     * Destroy the player and break connection from pulseaudio and free the memory.
     */
    void destory();

    /**
     * Get volume stream tag.
     *
     * @return const char*
     *         return the stream tag.
     */
    const char *getTag();

    /**
     * Get sample format.
     *
     * @return const pa_sample_spec*
     *         return the stream sample format.
     */
    const pa_sample_spec *getSample_spec();

private:
    /** Volume stream type. */
    char *mTag;
    /** Simple connection object. */
    pa_simple *mPaSimple;
    /** Sample spec. */
    pa_sample_spec mSampleSpec;
};
#endif

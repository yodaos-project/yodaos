#include "pulseaudio.hh"
#include <cmath>
#include <list>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <string>
#include <fstream>

using namespace std;

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
        STREAM_TYPE_NUM,
    } rk_stream_type_t;

    const char* g_stream_type[STREAM_TYPE_NUM] = { "audio", "tts", "ring", "voiceCall", "alarm", "playback", "system" };
    static Pulseaudio pulse("rokid");

    static int bUseCustomVolumeCurve = 0;
    static int bLoadCustomVolumeCurve = 0;
    static int CustomVolArray[101] = {};

    static const char* VOLUME_CURVE_FILE = "/etc/volume/volumecurve.txt";

    int rk_custom_changeto_pactlVolume(int vol)
    {
        return CustomVolArray[vol];
    }

    int rk_custom_changeForm_pactl_volume(int vol)
    {
        for (int i = 0; i <= 100; i++)
        {
            if (CustomVolArray[i] == vol)
                return i;
        }
        return -1;
    }

    int rk_initCustomVolumeCurve();

    int rk_changeto_pactl_volume(int vol)
    {
        int volume = 0;

        if (!bUseCustomVolumeCurve && !bLoadCustomVolumeCurve)
        {
            int ret = rk_initCustomVolumeCurve();
            if (ret >= 0)
            {
                bUseCustomVolumeCurve = 1;
            }
            bLoadCustomVolumeCurve = 1;
        }
        if (bUseCustomVolumeCurve)
        {
            volume = rk_custom_changeto_pactlVolume(vol);
        }
        else
        {
            // new_value = pa_sw_volume_from_linear(vol / 100.0);
            // new_value = (int) round (sqrt((double) vol / 100.0) * (double) PA_VOLUME_NORM );
            // new_value = (int) round ((double) vol / 100.0 * (double) PA_VOLUME_NORM );
            if (vol != 0)
                volume = (vol + 156) * (vol + 156);
            else
                volume = 0;
        }
        // printf("%s vol[%d] volume[%d]\n",__FUNCTION__,vol,volume);
        return volume;
    }

    int rk_changeForm_pactl_volume(int vol)
    {
        int volume = 0;

        if (bUseCustomVolumeCurve)
        {
            volume = rk_custom_changeForm_pactl_volume(vol);
        }
        else
        {
            if (vol != 0)
                volume = (int)round(sqrt(vol)) - 156;
            else
                volume = 0;
        }
        // printf("%s vol[%d] volume[%d]\n",__FUNCTION__,vol,volume);
        return volume;
    }

    int rk_setCustomVolumeCurve(int cnt, int* VolArray)
    {
        printf("rk_setCustomVolumeCurve:%d\n", cnt);
        if (cnt != 101)
            return -1;
        bUseCustomVolumeCurve = 1;
        for (int i = 0; i < cnt; i++)
        {
            CustomVolArray[i] = VolArray[i];
            // printf("CustomVolArray[%d]:%d\n",i,CustomVolArray[i]);
        }
        return 0;
    }

    int rk_initCustomVolumeCurve()
    {
        ifstream fin(VOLUME_CURVE_FILE);
        string str;
        int count = 0;
        int volArray[101] = { 0 };
        if (!fin.is_open())
        {
            printf("Failed to open %s!\n", VOLUME_CURVE_FILE);
            return -1;
        }
        while (!fin.eof() && (count < 101))
        {
            getline(fin, str);
            stringstream ss;
            ss << str;
            ss >> volArray[count];
            count++;
        }
        int ret = rk_setCustomVolumeCurve(count, volArray);
        if (ret < 0)
        {
            return -1;
        }
        return 0;
    }

    int rk_set_all_volume(int vol)
    {
        pa_volume_t new_value = 0;
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();

        new_value = rk_changeto_pactl_volume(vol);
        if (new_value > PA_VOLUME_MAX)
            new_value = PA_VOLUME_MAX;
        pulse.set_volume(device, new_value);
        pthread_mutex_unlock(&pulse.lock);
        return 0;
    }

    int rk_get_all_volume()
    {
        pa_volume_t avgvol;
        int vol;
        double tmp;
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();
        pthread_mutex_unlock(&pulse.lock);
        avgvol = pa_cvolume_avg(&device.volume);
        vol = rk_changeForm_pactl_volume(avgvol);
        // vol = (int) round (tmp * tmp * 100.0);
        // vol = (int) round ((double) avgvol / (double) PA_VOLUME_NORM * 100.0);
        return vol;
    }

    int rk_set_mute(bool mute)
    {
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();
        pulse.set_mute(device, mute);
        pthread_mutex_unlock(&pulse.lock);
        return 0;
    }

    bool rk_is_mute()
    {
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();
        pthread_mutex_unlock(&pulse.lock);
        return device.mute;
    }

    int rk_set_volume(int vol)
    {
        pa_volume_t new_value = 0;
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();
        new_value = rk_changeto_pactl_volume(vol);

        if (new_value > PA_VOLUME_MAX)
            new_value = PA_VOLUME_MAX;
        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if (strcmp(g_stream_type[STREAM_ALARM], (const char*)sinkinput.name.c_str())
                && device.index == sinkinput.sink)
                pulse.set_volume(sinkinput, new_value);
        }
        pthread_mutex_unlock(&pulse.lock);
        return 0;
    }

    int rk_get_volume()
    {
        pa_volume_t avgvol;
        int vol = -1;
        double tmp;
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();

        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if ((!strcmp(g_stream_type[STREAM_AUDIO], (const char*)sinkinput.name.c_str())
                 || !strcmp(g_stream_type[STREAM_TTS], (const char*)sinkinput.name.c_str()))
                && device.index == sinkinput.sink)
            {
                // vol = (int) round(pa_sw_volume_to_linear(sinkinput.volume_avg) * 100.0)
                tmp = (double)sinkinput.volume_avg / (double)PA_VOLUME_NORM;
                // vol = (int) round (tmp * tmp * 100.0);
                vol = (int)round(tmp * 100.0);
                break;
            }
        }
        pthread_mutex_unlock(&pulse.lock);
        return vol;
    }

    int rk_set_tag_volume(const char* tag, int vol)
    {
        if (!tag || strlen(tag) == 0)
        {
            return -1;
        }
        bool hit = false;
        pa_volume_t new_value = 0;
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();

        new_value = rk_changeto_pactl_volume(vol);

        if (new_value > PA_VOLUME_MAX)
            new_value = PA_VOLUME_MAX;
        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if (!strcmp(tag, (const char*)sinkinput.name.c_str()) && device.index == sinkinput.sink)
            {
                hit = true;
                pulse.set_volume(sinkinput, new_value);
            }
        }
        if (!hit)
        {
            pulse.set_volume_store(tag, new_value);
            printf("set volume at the stream not new,media name=%s\n", tag);
        }
        pthread_mutex_unlock(&pulse.lock);
        return 0;
    }

    int rk_get_tag_volume(const char* tag)
    {
        if (!tag || strlen(tag) == 0)
        {
            return -1;
        }

        pa_volume_t avgvol;
        int vol = -1;
        double tmp;
        pthread_mutex_lock(&pulse.lock);

        Device device = pulse.get_default_sink();

        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if (!strcmp(tag, (const char*)sinkinput.name.c_str()) && device.index == sinkinput.sink)
            {
                // vol = (int) round(pa_sw_volume_to_linear(sinkinput.volume_avg) * 100.0);
                // tmp = (double) sinkinput.volume_avg / (double) PA_VOLUME_NORM;
                // vol = (int) round (tmp * tmp * 100.0);
                vol = rk_changeForm_pactl_volume(sinkinput.volume_avg);
                break;
            }
        }
        pthread_mutex_unlock(&pulse.lock);
        return vol;
    }

    int rk_set_tag_mute(const char* tag, bool mute)
    {
        if (!tag || strlen(tag) == 0)
        {
            return -1;
        }
        pthread_mutex_lock(&pulse.lock);

        Device device = pulse.get_default_sink();

        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if (!strcmp(tag, (const char*)sinkinput.name.c_str()) && device.index == sinkinput.sink)
                pulse.set_mute(sinkinput, mute);
        }
        pthread_mutex_unlock(&pulse.lock);
        return 0;
    }

    bool rk_is_tag_mute(const char* tag)
    {
        if (!tag || strlen(tag) == 0)
        {
            return -1;
        }
        pthread_mutex_lock(&pulse.lock);
        Device device = pulse.get_default_sink();

        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if (!strcmp(tag, (const char*)sinkinput.name.c_str()) && device.index == sinkinput.sink)
            {
                pthread_mutex_unlock(&pulse.lock);
                return sinkinput.mute;
            }
        }
        pthread_mutex_unlock(&pulse.lock);
        return -1;
    }

    int rk_get_tag_playing_status(const char* tag)
    {
        if (!tag || strlen(tag) == 0)
        {
            return -1;
        }
        pthread_mutex_lock(&pulse.lock);

        Device device = pulse.get_default_sink();

        for (Device& sinkinput : pulse.get_sinkinputs())
        {
            if (!strcmp(tag, (const char*)sinkinput.name.c_str()))
            {
                pthread_mutex_unlock(&pulse.lock);
                return 1;
            }
        }
        pthread_mutex_unlock(&pulse.lock);
        return 0;
    }

    int rk_set_stream_volume(rk_stream_type_t stream_type, int vol)
    {
        if (stream_type < 0 || stream_type >= STREAM_TYPE_NUM)
            return -1;

        return rk_set_tag_volume(g_stream_type[stream_type], vol);
    }

    int rk_get_stream_volume(rk_stream_type_t stream_type)
    {
        if (stream_type < 0 || stream_type >= STREAM_TYPE_NUM)
            return -1;

        return rk_get_tag_volume(g_stream_type[stream_type]);
    }

    int rk_set_stream_mute(rk_stream_type_t stream_type, bool mute)
    {
        if (stream_type < 0 || stream_type >= STREAM_TYPE_NUM)
            return -1;

        return rk_set_tag_mute(g_stream_type[stream_type], mute);
    }

    bool rk_is_stream_mute(rk_stream_type_t stream_type)
    {
        if (stream_type < 0 || stream_type >= STREAM_TYPE_NUM)
            return -1;

        return rk_is_tag_mute(g_stream_type[stream_type]);
    }

    int rk_get_stream_playing_status(rk_stream_type_t stream_type)
    {
        if (stream_type < 0 || stream_type >= STREAM_TYPE_NUM)
            return 0;

        return rk_get_tag_playing_status(g_stream_type[stream_type]);
    }

    /* old interface */
    int set_app_volume(char* name, int vol)
    {
        return -1;
    }

    int get_app_volume(char* name)
    {
        return -1;
    }
    int set_sink_mute(int mute)
    {
        return -1;
    }
#ifdef __cplusplus
}
#endif

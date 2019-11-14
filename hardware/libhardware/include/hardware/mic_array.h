/*************************************************************************
	> File Name: mic_array.h
	> Author: 
	> Mail: 
	> Created Time: Mon May  4 14:19:38 2015
 ************************************************************************/

#ifndef ANDROID_MIC_ARRAY_H
#define ANDROID_MIC_ARRAY_H

#include <stdint.h>
#include <hardware/hardware.h>

__BEGIN_DECLS
#define MIC_ARRAY_HARDWARE_MODULE_ID "mic_array"
//ENABLE_DSP_MULTI_BF means the data get from vsp supports mic_stream ref_stream and multi_bf_stream.
//#define ENABLE_DSP_ALL_EXT
//#define ENABLE_DSP_MULTI_BF

struct mic_array_module_t {
    struct hw_module_t common;
};

struct mic_array_device_t {
    struct hw_device_t common;
    int channels;
    int sample_rate;
    int bit;
    unsigned int frame_cnt;
#ifdef MIC_ARRAY_HAL_VSP
    unsigned int ext_cnt;
    int dspMultiBf;
    int dspBfNum;
#endif
    struct pcm *pcm;

    int (*get_stream_buff_size) (struct mic_array_device_t *dev);
    int (*start_stream) (struct mic_array_device_t *dev);
    int (*stop_stream) (struct mic_array_device_t *dev);
    int (*finish_stream) (struct mic_array_device_t * dev);
    int (*resume_stream) (struct mic_array_device_t *dev);
    #ifdef ENABLE_DSP_ALL_EXT
    int (*read_stream) (struct mic_array_device_t* dev, char* ext_buff, int* ext_cnt);
    #elif ENABLE_DSP_MULTI_BF
    int (*read_stream) (struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt, float* ext_buff, unsigned int ext_cnt);
    #else
    int (*read_stream) (struct mic_array_device_t *dev, char *buff, unsigned int frame_cnt);
    #endif
    int (*config_stream) (struct mic_array_device_t *dev, int cmd, char *cmd_buff);
	int (*find_card) (const char *snd);
};
__END_DECLS
#endif

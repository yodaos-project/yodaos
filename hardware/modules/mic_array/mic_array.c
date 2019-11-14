/*************************************************************************
        > File Name: mic_array.c
        > Author: Li.Zhang
        > Mail: li.zhang@rokid.com
        > Created Time: Mon Jan 16 11:11:11 2018
        > Note: This file demostrate the way to read data from VSP
        >       with ioctl method rather than ALSA
 ************************************************************************/
#define LOG_TAG "mic_array"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <hardware/hardware.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>

#include <time.h>
#include <tinyalsa/asoundlib.h>

#include "mic/mic_array.h"
#include <vsp_ioctl.h>

#define VSP_DEV_PATH "/dev/"VSP_DEVICE_NAME

//#define ENABLE_MIC_STREAM
//#define ENABLE_REF_STREAM
//#define ENABLE_OUT_STREAM

#define MODULE_NAME "mic_array"
#define MODULE_AUTHOR "li.zhang@rokid.com"

struct mic_array_device_ex {
    struct mic_array_device_t mic_array;

    int vsp_fd;
    VSP_IOC_INFO vsp_info;
    VSP_IOC_CONTEXT vsp_context;
    int vsp_sample_left;                // 没有取走的样本点；
    short *vsp_buffer;                  // 这是为了交织前的Buffer；
    char *ext_buffer;                 //data of multi bf from dsp
};

static int mic_array_device_open(const struct hw_module_t* module,const char* name, struct hw_device_t** device);

static int mic_array_device_close(struct hw_device_t* device);

static int mic_array_device_start_stream(struct mic_array_device_t* dev);

static int mic_array_device_stop_stream(struct mic_array_device_t* dev);

static int mic_array_device_finish_stream(struct mic_array_device_t* dev);

#ifdef ENABLE_DSP_ALL_EXT
static int mic_array_device_read_stream(struct mic_array_device_t* dev, char* ext_buff, int* ext_cnt);
#elif ENABLE_DSP_MULTI_BF
static int mic_array_device_read_stream(struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt, float* ext_buff, unsigned int ext_cnt);
#else
static int mic_array_device_read_stream(struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt);
#endif

static int mic_array_device_config_stream(struct mic_array_device_t* dev, int cmd, char* cmd_buff);

static int mic_array_device_get_stream_buff_size(struct mic_array_device_t* dev);

static int mic_array_device_resume_stream(struct mic_array_device_t* dev);

static int mic_array_device_find_card(const char* snd);

static struct hw_module_methods_t mic_array_module_methods = {
    .open = mic_array_device_open,
};

struct mic_array_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = MIC_ARRAY_HARDWARE_MODULE_ID,
        .name = MODULE_NAME,
        .author = MODULE_AUTHOR,
        .methods = &mic_array_module_methods,
    },
};


static int mic_array_device_find_card(const char* snd)
{
    if (snd == NULL)
        return -1;
    return 0;
}

static int mic_array_device_open(const struct hw_module_t* module,
    const char* name, struct hw_device_t** device)
{
    struct mic_array_device_ex* dev_ex = NULL;
    struct mic_array_device_t* dev = NULL;

    dev_ex = (struct mic_array_device_ex*)malloc(sizeof(struct mic_array_device_ex));
    if (!dev_ex) {
        ALOGE("MIC_ARRAY: FAILED TO ALLOC SPACE");
        return -1;
    }

    memset(dev_ex, 0, sizeof(struct mic_array_device_ex));

    // open vsp device
    dev_ex->vsp_fd = open(VSP_DEV_PATH, O_RDONLY);
    if (dev_ex->vsp_fd < 0) {
        ALOGE("Failed to open vsp device\n");
        free(dev_ex);
        return -1;
    }

    if (ioctl(dev_ex->vsp_fd, VSP_IOC_GET_INFO, &dev_ex->vsp_info)) {
        ALOGE("Failed to get system info!\n");
        close(dev_ex->vsp_fd);
        free(dev_ex);
        return -1;
    }

    printf("==========================================================\n");
    printf(" Message Version:               %x\n", dev_ex->vsp_info.msg_version);
    printf(" Kernel's IOC Version:          %x\n", dev_ex->vsp_info.ioc_version);
    printf(" Our IOC Version:               %x\n", VSP_IOC_VERSION);
    printf(" Sample Rate:                   %d (Hz)\n", dev_ex->vsp_info.sample_rate);
    printf(" MIC Channel Number:            %d (channels)\n", dev_ex->vsp_info.mic_num);
    printf(" REF Channel Number:            %d (channels)\n", dev_ex->vsp_info.ref_num);
    printf(" OUT Channel Number:            %d (channels)\n", dev_ex->vsp_info.out_num);
    printf(" OUT Interlaced:                %d\n", dev_ex->vsp_info.out_interlaced);
    printf(" Audio Frame Length:            %d (ms)\n", dev_ex->vsp_info.frame_length);
    printf(" Audio Frame Num in Context:    %d (frames)\n", dev_ex->vsp_info.frame_num);
    printf(" Features Dimension:            %d (dims)\n", dev_ex->vsp_info.features_dim);
    printf(" Extra Buffer Size:             %d (bytes)\n", dev_ex->vsp_info.ext_buffer_size);
    printf(" LED Number:                    %d\n", dev_ex->vsp_info.led_num);
    printf("==========================================================\n");

    dev = (struct mic_array_device_t*)dev_ex;
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = mic_array_device_close;

    dev->start_stream = mic_array_device_start_stream;
    dev->stop_stream = mic_array_device_stop_stream;
    dev->finish_stream = mic_array_device_finish_stream;
    dev->resume_stream = mic_array_device_resume_stream;
    dev->read_stream = mic_array_device_read_stream;
    dev->config_stream = mic_array_device_config_stream;
    dev->get_stream_buff_size = mic_array_device_get_stream_buff_size;
    dev->find_card = mic_array_device_find_card;

    dev->sample_rate = dev_ex->vsp_info.sample_rate;
    dev->bit = 16;
    dev->pcm = NULL;

    *device = &(dev->common);
    return 0;
}

//static void resetBuffer(struct mic_array_device_ex* dev) { dev->pts = 0; }

static int mic_array_device_close(struct hw_device_t* device)
{
    ALOGI("DEVICE CLOSE");

    struct mic_array_device_t* mic_array_device = (struct mic_array_device_t*)device;
    struct mic_array_device_ex *dev_ex = (struct mic_array_device_ex *)mic_array_device;

    if (dev_ex->vsp_fd >= 0)
        close(dev_ex->vsp_fd);

    if (mic_array_device != NULL) {
        free(mic_array_device);
        mic_array_device = NULL;
    }
    return 0;
}

static int mic_array_device_start_stream(struct mic_array_device_t* dev)
{
    int result = 0;
    int channel_bytes_size = 0;
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_ex*)dev;

    printf("mic_array_device_start_stream, dspMultiBf=%d, dspBfNum=%d\n", dev->dspMultiBf, dev->dspBfNum);
    dev->channels = 0;
    if(dev->dspMultiBf) {
        #ifdef ENABLE_DSP_MULTI_BF
            //dev->channels = dev_ex->vsp_info.out_num;
            dev->channels = dev->dspBfNum;
        #else
            dev->channels += dev_ex->vsp_info.mic_num;
            dev->channels += 1;
        #endif
        //printf("mic_array_device_start_stream, we are using OUT_STREAM, dev->channels=%d\n", dev->channels);
    } else {
        dev->channels = dev_ex->vsp_info.mic_num;
        //printf("mic_array_device_start_stream, we are using MIC_STREAM, dev->channels=%d\n", dev->channels);
    }

    printf("mic_array_device_start_stream, frame_length=%d, sample_rate=%d, frame_num=%d, channels=%d, vsp_info.out_num=%d\n", dev_ex->vsp_info.frame_length, dev_ex->vsp_info.sample_rate, dev_ex->vsp_info.frame_num, dev->channels, dev_ex->vsp_info.out_num);
    channel_bytes_size = dev_ex->vsp_info.frame_length * dev_ex->vsp_info.sample_rate * dev_ex->vsp_info.frame_num * sizeof(float) / 1000;
    printf("mic_array_device_start_stream, channel_bytes_size=%d\n", channel_bytes_size);

    //dev->frame_cnt = channel_bytes_size * dev->channels;
    dev->frame_cnt = channel_bytes_size * dev_ex->vsp_info.out_num;
    dev->ext_cnt = dev_ex->vsp_info.ext_buffer_size;// aec + freq_spectrum + energy
    printf("mic_array_device_start_stream, Allocate vsp buffer with size = %d, ext buffer with size=%d\n", dev->frame_cnt, dev->ext_cnt);

    #ifndef ENABLE_DSP_ALL_EXT
    dev_ex->vsp_buffer = (float *)malloc(dev->frame_cnt);
    if (!dev_ex->vsp_buffer) {
        ALOGE("Failed to allocate vsp buffer!");
        return -1;
    }

    void *vsp_buffer = (void *)dev_ex->vsp_buffer;
    if(dev->dspMultiBf) {
        // multi beamforming buffer
        #ifdef ENABLE_DSP_MULTI_BF
        dev_ex->vsp_context.out_buffer.addr = vsp_buffer;
        dev_ex->vsp_context.out_buffer.size = channel_bytes_size * dev_ex->vsp_info.out_num;
        //dev_ex->vsp_context.out_buffer.size = channel_bytes_size * dev->channels;
        vsp_buffer += dev_ex->vsp_context.out_buffer.size;

        //vsp_buffer += 4 * 72 * sizeof(float);
        //vsp_buffer += 4 * channel_bytes_size;
        #else
        // Fill the Mic Buffer
        for (int i = 0; i < dev_ex->vsp_info.mic_num; i++) {
            dev_ex->vsp_context.mic_buffer[i].addr = vsp_buffer;
            dev_ex->vsp_context.mic_buffer[i].size = channel_bytes_size;
            vsp_buffer += channel_bytes_size;
        }
        dev_ex->vsp_context.out_buffer.addr = vsp_buffer;
        dev_ex->vsp_context.out_buffer.size = channel_bytes_size;
        #endif
    } else {
        // Fill the Mic Buffer
        for (int i = 0; i < dev_ex->vsp_info.mic_num; i++) {
            dev_ex->vsp_context.mic_buffer[i].addr = vsp_buffer;
            dev_ex->vsp_context.mic_buffer[i].size = channel_bytes_size;
            vsp_buffer += channel_bytes_size;
        }
    }
    #endif

    dev_ex->ext_buffer = (char *)malloc(dev->ext_cnt);
    if (!dev_ex->ext_buffer) {
        ALOGE("Failed to allocate ext buffer!");
        return -1;
    }

    //todo: currently, we always output the ext_bffer because the mcu supports, and convenient contrast
    void *ext_buffer = (void *)dev_ex->ext_buffer;
    //dev_ex->vsp_context.ext_buffer.addr = ext_buffer;
    dev_ex->vsp_context.ext_buffer.size = dev_ex->vsp_info.ext_buffer_size;

    //printf("start stream, dev_ex->vsp_context.ext_buffer.addr=%x\n", dev_ex->vsp_context.ext_buffer.addr);

    // 切换到Active模式
    #ifdef KENOBI_MODE
    VSP_IOC_MODE_TYPE work_mode = VSP_IOC_MODE_KENOBI;
    result = ioctl(dev_ex->vsp_fd, VSP_IOC_SWITCH_MODE, work_mode);
    if (result || work_mode != VSP_IOC_MODE_KENOBI) {
        printf("Failed to switch to KEMOBI mode!\n");
        return -1;
    }
    #else
    VSP_IOC_MODE_TYPE work_mode = VSP_IOC_MODE_ACTIVE;
    result = ioctl(dev_ex->vsp_fd, VSP_IOC_SWITCH_MODE, work_mode);
    if (result || work_mode != VSP_IOC_MODE_ACTIVE) {
        printf("Failed to switch to ACTIVE mode!\n");
        return -1;
    }
    #endif

    return 0;
}

static int mic_array_device_stop_stream(struct mic_array_device_t* dev)
{
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_ex *)dev;

    #ifdef ENABLE_DSP_ALL_EXT
    dev->ext_cnt = 0;
    if (dev_ex->ext_buffer) {
        free(dev_ex->ext_buffer);
        dev_ex->ext_buffer = NULL;
    }
    dev_ex->vsp_sample_left = 0;
    #else
    dev->frame_cnt = 0;
    if (dev_ex->vsp_buffer) {
        free(dev_ex->vsp_buffer);
        dev_ex->vsp_buffer = NULL;
    }
    dev_ex->vsp_sample_left = 0;
    #endif

    return 0;
}

static int mic_array_device_finish_stream(struct mic_array_device_t* dev)
{
    ALOGE("finish stream is no use");
    return -1;
}

#ifdef ENABLE_DSP_ALL_EXT
static int mic_array_device_read_stream(
    struct mic_array_device_t* dev, char* ext_buff, int* ext_cnt)
{
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_ex*)dev;

    if (dev_ex->vsp_fd < 0 || !dev_ex->ext_buffer) {
        ALOGE("not initialized!");
        return -1;
    }

    if (ext_buff == NULL) {
        ALOGE("null buffer");
        return -1;
    }

	dev_ex->vsp_context.ext_buffer.addr = ext_buff;

//    printf("mic_array_device_read_stream, ext_buffer.size=%d, dev_ex->vsp_context.ext_buffer.addr=%p, ext_cnt=%d, ext_buff addr=%p\n", dev_ex->vsp_context.ext_buffer.size, dev_ex->vsp_context.ext_buffer.addr, *ext_cnt, ext_buff);
    if (ioctl(dev_ex->vsp_fd, VSP_IOC_GET_CONTEXT, &dev_ex->vsp_context)) {
        ALOGE("Failed to get context!\n");
        return -1;
    }
    //*ext_cnt = ((int *)dev_ex->vsp_context.ext_buffer.addr)[0]; 
    *ext_cnt = dev->ext_cnt; 
  //  printf("after ioctl, read stream, dev_ex->vsp_context.ext_buffer.addr=%p, dsp ext_cnt=%d, ext_cnt addr=%p, ext_buff addr=%p\n", dev_ex->vsp_context.ext_buffer.addr, *ext_cnt, ext_cnt, ext_buff);
    //for test
    //*ext_cnt = 10240;
    //memcpy(ext_buff, (char*)((char *)dev_ex->vsp_context.ext_buffer.addr), *ext_cnt);
//    memcpy(ext_buff, (dev_ex->vsp_context.ext_buffer.addr), *ext_cnt);//test cpu
//    *ext_buff = *(dev_ex->vsp_context.ext_buffer.addr);
//    printf("read stream, memcpy end, ext_buff addr=%p\n", ext_buff);

    return 0;
}
#elif ENABLE_DSP_MULTI_BF
static int mic_array_device_read_stream(
    struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt, float* ext_buff, unsigned int ext_cnt)
{
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_ex*)dev;

    if (dev_ex->vsp_fd < 0 || !dev_ex->vsp_buffer) {
        ALOGE("not initialized!");
        return -1;
    }

    if (buff == NULL || ext_buff == NULL) {
        ALOGE("null buffer");
        return -1;
    }

    short *out_buffer = (short *)buff;
    unsigned int out_sample_num = frame_cnt / sizeof(short) / dev->channels;
    unsigned int channel_sample_num = dev_ex->vsp_info.frame_length * dev_ex->vsp_info.sample_rate * dev_ex->vsp_info.frame_num / 1000;
    //printf("mic_array_device_read_stream, out_sample_num=%d, channel_sample_num=%d, dev->channels=%d, ext_buffer.size=%d, dev_ex->vsp_context.ext_buffer.addr=%x, ext_cnt=%d\n", out_sample_num, channel_sample_num, dev->channels, dev_ex->vsp_context.ext_buffer.size, dev_ex->vsp_context.ext_buffer.addr, ext_cnt);
    unsigned int count = 0;

    while (out_sample_num) {
        if (dev_ex->vsp_sample_left) {
            // 读取长度
            unsigned int read_sample_num = out_sample_num < dev_ex->vsp_sample_left ? out_sample_num : dev_ex->vsp_sample_left;
            // 准备源数据
            short *vsp_buffer[16];
            int j = 0;

            //printf("mic_array_device_read_stream, dspMultiBf=%d\n", dev->dspMultiBf);
            if(dev->dspMultiBf) {
                for (int i = 0; i < dev->channels; i++) {
                    vsp_buffer[j] = (short *)(dev_ex->vsp_context.out_buffer.addr + (channel_sample_num - dev_ex->vsp_sample_left) * sizeof(short) + i * channel_sample_num * sizeof(short));
                    j++;
                }
            } else {
                for (int i = 0; i < dev_ex->vsp_info.mic_num; i++) {
                    vsp_buffer[j] = (short *)(dev_ex->vsp_context.mic_buffer[i].addr + (channel_sample_num - dev_ex->vsp_sample_left) * sizeof(short));
                    j++;
                }
            }

            // 交织过程
            for (int j = 0; j < read_sample_num; j++) {
                for (int i = 0; i < dev->channels; i++) {
                    *out_buffer = *vsp_buffer[i];
                    out_buffer++;
                    vsp_buffer[i]++;
                    //printf("data print, j=%d, i=%d, vsp_buffer[j][i]=%d\n", j, i, vsp_buffer[i][j]);
                }
            }

            out_sample_num -= read_sample_num;
            dev_ex->vsp_sample_left -= read_sample_num;

            memcpy(ext_buff, (float*)((float *)dev_ex->vsp_context.ext_buffer.addr), ext_cnt * sizeof(float));
        } else {
            if (ioctl(dev_ex->vsp_fd, VSP_IOC_GET_CONTEXT, &dev_ex->vsp_context)) {
                ALOGE("Failed to get context!\n");
                return -1;
            }
            dev_ex->vsp_sample_left = channel_sample_num;
            count++;
            //printf("after ioctl, read stream, dev_ex->vsp_context.out_buffer.addr=%x, dev_ex->vsp_context.ext_buffer.addr=%x\n", dev_ex->vsp_context.out_buffer.addr, dev_ex->vsp_context.ext_buffer.addr);
        }
    }

    return 0;
}
#else
static int mic_array_device_read_stream(
    struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt)
{
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_ex*)dev;

    if (dev_ex->vsp_fd < 0 || !dev_ex->vsp_buffer) {
        ALOGE("not initialized!");
        return -1;
    }

    if (buff == NULL) {
        ALOGE("null buffer");
        return -1;
    }

    short *out_buffer = (short *)buff;
    unsigned int out_sample_num = frame_cnt / sizeof(short) / dev->channels;
    unsigned int channel_sample_num = dev_ex->vsp_info.frame_length * dev_ex->vsp_info.sample_rate * dev_ex->vsp_info.frame_num / 1000;
    //printf("mic_array_device_read_stream, out_sample_num=%d, channel_sample_num=%d\n", out_sample_num, channel_sample_num);

    while (out_sample_num) {
        if (dev_ex->vsp_sample_left) {
            // 读取长度
            unsigned int read_sample_num = out_sample_num < dev_ex->vsp_sample_left ? out_sample_num : dev_ex->vsp_sample_left;
            // 准备源数据
            short *vsp_buffer[16];
            int j = 0;
            #ifdef ENABLE_MIC_STREAM
            for (int i = 0; i < dev_ex->vsp_info.mic_num; i++) {
                vsp_buffer[j] = (short *)(dev_ex->vsp_context.mic_buffer[i].addr + (channel_sample_num - dev_ex->vsp_sample_left) * sizeof(short));
                j++;
            }
            #endif
            #ifdef ENABLE_REF_STREAM
            for (int i = 0; i < dev_ex->vsp_info.ref_num; i++) {
                vsp_buffer[j] = (short *)(dev_ex->vsp_context.ref_buffer[i].addr + (channel_sample_num - dev_ex->vsp_sample_left) * sizeof(short));
                j++;
            }
            #endif
            #ifdef ENABLE_OUT_STREAM
            vsp_buffer[j] = (short *)(dev_ex->vsp_context.out_buffer.addr + (channel_sample_num - dev_ex->vsp_sample_left) * sizeof(short));
            #endif
            // 交织过程
            for (int j = 0; j < read_sample_num; j++) {
                for (int i = 0; i < dev->channels; i++) {
                    *out_buffer = *vsp_buffer[i];
                    out_buffer++;
                    vsp_buffer[i]++;
                }
            }

            out_sample_num -= read_sample_num;
            dev_ex->vsp_sample_left -= read_sample_num;

        } else {
            if (ioctl(dev_ex->vsp_fd, VSP_IOC_GET_CONTEXT, &dev_ex->vsp_context)) {
                ALOGE("Failed to get context!\n");
                return -1;
            }
            dev_ex->vsp_sample_left = channel_sample_num;
        }
    }
    return 0;
}
#endif

static int mic_array_device_config_stream(struct mic_array_device_t* dev, int cmd, char* cmd_buff)
{
    return -1;
}

static int mic_array_device_get_stream_buff_size(struct mic_array_device_t* dev)
{
    #ifdef ENABLE_DSP_ALL_EXT
    return dev->ext_cnt;
    #else
    return dev->frame_cnt;
    #endif
}

static int mic_array_device_resume_stream(struct mic_array_device_t* dev)
{
    ALOGI("not implmentation");
    return -1;
}

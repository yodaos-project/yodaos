#define LOG_TAG "leds_hw"

#include <hardware/hardware.h>
#include <rokidware/leds.h>
#include <cutils/log.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <vsp_ioctl.h> //vsp device header file

#define VSP_DEV_PATH    ("/dev/" VSP_DEVICE_NAME)
#define MAX_LED_GROUP_NUM LED_NUMS //defined in defconfig
#define LED_REVERSE TRUE
#define LED_SEQUENCE TRUE
#define MAX_BRIGHTNESS    255

const int LED_OFFSET = 0;
static int brightness = MAX_BRIGHTNESS;
static unsigned char api_enabled = 1;
static int s_led_fd = 0;

#define CHK_IF_ENABLED() \
	do { \
		if (!api_enabled) { \
			ALOGW("disabled\n"); \
			return (-EPERM); \
		} \
	} while(0)

#define CHK_IF_OUT_OF_RANGE(br) \
	do { \
		if (br < 0 || br > 255) { \
			ALOGE("out of range\n"); \
			return (-EINVAL); \
		} \
	} while(0)

#define CHK_IF_BUSY() \
	do { \
		if (led_is_busy()) { \
			ALOGE("busy\n"); \
			return (-EBUSY); \
		} \
	} while(0)


static int close_vsp(int *vsp_fd)
{
    if((vsp_fd) && (*vsp_fd > 0))
    {
        close(*vsp_fd);
        *vsp_fd = 0;

        return 0;
    }

    return -1;
}

static int open_vsp(int *vsp_fd)
{
    int fd = 0;
    VSP_IOC_MODE_TYPE work_mode = VSP_IOC_MODE_ACTIVE;

    fd = open(VSP_DEV_PATH, O_RDONLY);
    if(fd > 0)
    {
        if(ioctl(fd, VSP_IOC_SWITCH_MODE, work_mode) == 0)
        {
            VSP_IOC_INFO vspInfo;
            ioctl(fd, VSP_IOC_GET_INFO, &vspInfo);
            ALOGV("vsp led num: %d\n", vspInfo.led_num);
        }
        else
        {
            close_vsp(&fd);
            ALOGE("switch vsp mode failed");
        }
    }
    else
    {
        ALOGE("open vsp device failed");
    }

    if(vsp_fd)
        *vsp_fd = fd;

    return fd;
}

static int write_pixls(unsigned char *buf, int len)
{
    VSP_IOC_LED_FRAME led_frame;
    int ret;

    if(s_led_fd <= 0)
        return -1;

    led_frame.transition = 0; //transition time, in ms, should <= duration
    led_frame.duration = 10; //hold duration time, in ms
    led_frame.pixels.addr = (void *)buf;
    led_frame.pixels.size = len;

    if((ret = ioctl(s_led_fd, VSP_IOC_PUT_LED_FRAME, &led_frame)) != 0)
    {
        ALOGE("%s", strerror(errno));
        return -errno;
    }

    return 0;
}

static int led_is_busy()
{
    int busy = 0;
    return busy ? 1 : 0;
}

static int led_current_set(struct ledarray_device_t *dev __unused,
			   unsigned char level)
{
    return -1;
}

static int led_enable_set(struct ledarray_device_t *dev __unused, int cmd)
{
    ALOGV("%s: cmd(%d)\n", __func__, cmd);

    api_enabled = cmd;
    return 0;
}

static int led_brightness_set(struct ledarray_device_t *dev __unused,
			      unsigned char br)
{
    CHK_IF_ENABLED();

    ALOGV("%s: br(%d)\n", __func__, br);

    brightness = br;
    return 0;
}

static int led_draw_one_inner(struct ledarray_device_t *dev __unused,
			      int idx, int br __unused, int r, int g,
			      int b)
{
    int ret = 0;
    unsigned char led_buf[MAX_LED_GROUP_NUM * 4] = {0};
    unsigned char *p = NULL;

    if(idx >= MAX_LED_GROUP_NUM)
    {
        return -1;
    }

    p = led_buf + idx * 4;
    p[0] = (unsigned char)r;
    p[1] = (unsigned char)b;
    p[2] = (unsigned char)g;
    p[3] = 0xff;

    return write_pixls(led_buf, sizeof(led_buf));
}

static inline int led_index_relocate(int index)
{
#ifdef LED_REVERSE
    index = ((MAX_LED_GROUP_NUM - index)%MAX_LED_GROUP_NUM); // reverse.
#endif
    index = ((index + LED_OFFSET) % MAX_LED_GROUP_NUM); // first led pos need offset LED_OFFSET.
#ifdef LED_SEQUENCE
    switch(index) {
        case 0:
            index = 1;
            break;
        case 1:
            index = 7;
            break;
        case 2:
            index = 2;
            break;
        case 3:
            index = 8;
            break;
        case 4:
            index = 3;
            break;
        case 5:
            index = 9;
            break;
        case 6:
            index = 4;
            break;
        case 7:
            index = 10;
            break;
        case 8:
            index = 5;
            break;
        case 9:
            index = 11;
            break;
        case 10:
            index = 0;
            break;
        case 11:
            index = 6;
            break;
        default:
            break;
    }
#endif
    return index;
}

static void led_reverse_mem(unsigned char *dst, const unsigned char *src, size_t n)
{
    size_t src_idx;
    size_t dst_idx;
    const size_t led_nums = n/3;

    for (src_idx = 0; src_idx < led_nums; ++src_idx) {
        dst_idx = led_index_relocate(src_idx);
	const size_t src_offset = src_idx * 3;
	const size_t dst_offset = dst_idx * 3;
        dst[dst_offset] = src[src_offset];
        dst[dst_offset+1] = src[src_offset+1];
        dst[dst_offset+2] = src[src_offset+2];
    }
}

static int led_draw_one(struct ledarray_device_t *dev, int idx,
			int br, int r, int g, int b)
{
    int ret = 0;

    CHK_IF_ENABLED();
    CHK_IF_BUSY();

    CHK_IF_OUT_OF_RANGE(b);
    CHK_IF_OUT_OF_RANGE(g);
    CHK_IF_OUT_OF_RANGE(r);

    ALOGV("%s: idx(%d), br(%d), r(%d), g(%d), b(%d)\n", __func__, idx, br,
	  r, g, b);

    ret = led_draw_one_inner(dev, idx, br, r, g, b);
    if (ret)
	return ret;
    return 0;
}

static void color_trans(unsigned char *tmp_buf, int len, unsigned char *output)
{
    int i, n;

    for(i = 0, n = 0; i < len; i += 3)
    {
#ifdef COLOR_RGB2RBG
        output[n++] = tmp_buf[i];
        output[n++] = tmp_buf[i + 2];
        output[n++] = tmp_buf[i + 1];
        output[n++] = 0xff;
#elif COLOR_RGB2BGR
	output[n++] = tmp_buf[i + 2];
	output[n++] = tmp_buf[i + 1];
	output[n++] = tmp_buf[i];
	output[n++] = 0xff;
#else
	output[n++] = tmp_buf[i];
	output[n++] = tmp_buf[i + 1];
	output[n++] = tmp_buf[i + 2];
	output[n++] = 0xff;
#endif
    }
}

static int led_draw(struct ledarray_device_t *dev __unused,
		    unsigned char *buf, int len)
{
    unsigned char tmp_buf[MAX_LED_GROUP_NUM * PXL_FMT_RGB];
    unsigned char led_buf[MAX_LED_GROUP_NUM * 4];

    CHK_IF_ENABLED();
    CHK_IF_BUSY();

    if (len < 0 || len > MAX_LED_GROUP_NUM * PXL_FMT_RGB || (len % 3)) {
	ALOGE("invalid argument: len(%d)\n", len);
	return -EINVAL;
    }

    ALOGV("%s: buf(%p), len(%d)\n", __func__, buf, len);
    led_reverse_mem(tmp_buf, buf, len);
    color_trans(tmp_buf, len, led_buf);

    return write_pixls(led_buf, sizeof(led_buf));
}

static int led_dev_close(struct ledarray_device_t *device)
{
    ALOGV("%s\n", __func__);
    close_vsp(&s_led_fd);
    free(device);
    return 0;
}

static int led_dev_open(const hw_module_t * module,
			const char *name __unused, hw_device_t ** device)
{
    struct ledarray_device_t *ledadev = NULL;

    if(open_vsp(&s_led_fd) <= 0)
    {
        return -ENODEV;
    }

    ledadev = calloc(1, sizeof(ledarray_device_t));
    if (!ledadev) {
        close_vsp(&s_led_fd);
        ALOGE("Can not allocate memory for the leds device");
        return -ENOMEM;
    }

    ALOGV("%s\n", __func__);

    ledadev->common.tag = HARDWARE_DEVICE_TAG;
    ledadev->common.module = (hw_module_t *) module;
    ledadev->common.version = HARDWARE_DEVICE_API_VERSION(1, 0);

    ledadev->led_count = MAX_LED_GROUP_NUM;
    ledadev->pxl_fmt = PXL_FMT_RGB;

    ledadev->dev_close = led_dev_close;
    ledadev->draw = led_draw;
    ledadev->draw_one = led_draw_one;
    ledadev->set_brightness = led_brightness_set;
    ledadev->set_enable = led_enable_set;
    ledadev->set_current_reating = led_current_set;

    *device = (struct hw_device_t *) ledadev;
    return 0;
}

static struct hw_module_methods_t led_module_methods = {
    .open = led_dev_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = LEDS_API_VERSION,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = LED_ARRAY_HW_ID,
    .name = "ROKID LEDS HAL: The Coral solution.",
    .author = "The Linux Foundation",
    .methods = &led_module_methods,
};

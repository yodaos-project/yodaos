#define LOG_TAG "leds_pwm_hw"
//#define LOG_NDEBUG 0

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

#define PWM_EXPORT_PATH	"/sys/class/pwm/pwmchip0/export"
#define PWM_UNEXPORT_PATH	"/sys/class/pwm/pwmchip0/unexport"
#define PWM_PERIOD 50000    /* customed,nanoseconds */
#define PWM_OFFSET 13
#define PWM_NUMS    LED_NUMS
#define MAX_LED_GROUP_NUM LED_NUMS
#define MAX_BRIGHTNESS    255

static int brightness = MAX_BRIGHTNESS;
static unsigned char api_enabled = 1;
static int s_led_fd = 0;
static int pwm_export_finished = 0;

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

static int rgb_to_gray(int red, int green, int blue)
{
    int gray;
    gray = (red*3 + green*6 + blue*1) /10;

    return (gray > 255) ? 255 : gray;
}

static int pwm_export (int port)
{
    int fd = 0;
    int len = 0;
    char buffer[64];

    fd = open(PWM_EXPORT_PATH,O_WRONLY);
    if (fd < 0) {
        ALOGE("open pwm export failed. \n");
        return fd;
    }

    len = snprintf(buffer, sizeof(buffer), "%d", port);
    if (write(fd, buffer, len) < 0) {
        ALOGE("write pwm export %d failed. \n",port);
        return -1;
    }
    close(fd);
    return 0;
}

static int pwm_unexport (int port)
{
    int fd = 0;
    int len = 0;
    char buffer[64];

    fd = open(PWM_UNEXPORT_PATH,O_WRONLY);
    if (fd < 0) {
        ALOGE("open pwm unexport failed. \n");
        return fd;
    }

    len = snprintf(buffer, sizeof(buffer), "%d", port);
    if (write(fd, buffer, len) < 0) {
        ALOGE("write pwm unexport %d failed. \n",port);
        return -1;
    }
    close(fd);
    return 0;
}

static int pwm_port_init (int port)
{
    int fd = 0;
    int len = 0;
    char buffer[64];
    char path[64];

    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip0/pwm%d/period", port);
    fd = open(path,O_WRONLY);
    if (fd < 0) {
        ALOGE("open pwm%d period failed. \n",port);
        return fd;
    }

    len = snprintf(buffer, sizeof(buffer), "%d", PWM_PERIOD);
    if (write(fd, buffer, len) < 0) {
        ALOGE("pwm port%d period write failed. \n",port);
        return -1;
    }
    close(fd);

    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", port);
    fd = open(path,O_WRONLY);
    if (fd < 0) {
        ALOGE("open pwm%d duty_cycle failed. \n",port);
        return fd;
    }

    len = snprintf(buffer, sizeof(buffer), "%d", PWM_PERIOD);
    if (write(fd, buffer, len) < 0) {
        ALOGE("pwm port%d duty_cycle write failed. \n",port);
        return -1;
    }
    close(fd);

    return 0;

}

static int pwm_led_set (int num, int brightness)
{
    int fd = 0;
    int len = 0;
    int level = 0;
    char buffer[64];
    char path[64];

    if (brightness == 0){
        snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip0/pwm%d/enable", num + PWM_OFFSET);
        fd = open(path,O_WRONLY);
        if (fd < 0) {
            printf("open pwm enable port failed. \n");
            return fd;
        }

        len = snprintf(buffer, sizeof(buffer), "%d", brightness);
        if (write(fd, buffer, len) < 0) {
            ALOGE("pwm port enable write failed. \n");
            return -1;
        }
        close(fd);
    } else {
        snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip0/pwm%d/enable", num + PWM_OFFSET);
        fd = open(path,O_WRONLY);
        if (fd < 0) {
            ALOGE("open pwm enable port failed. \n");
            return fd;
        }

        len = snprintf(buffer, sizeof(buffer), "%d", 1);
        if (write(fd, buffer, len) < 0) {
            ALOGE("pwm port enable write failed. \n");
            return -1;
        }
        close(fd);
        level = 1 + brightness / 26; /* 10 level 's brightness is enough for single-color  led*/
        snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", num + PWM_OFFSET);
        fd = open(path,O_WRONLY);
        if (fd < 0) {
            ALOGE("open pwm duty_cycle sys failed. \n");
            return fd;
        }

        len = snprintf(buffer, sizeof(buffer), "%d", level*PWM_PERIOD/10); /* pwm control brightness */
        if (write(fd, buffer, len) < 0) {
            ALOGE("pwm duty_cycle write failed. \n");
            return -1;
        }
        close(fd);
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
			      int idx, int br __unused, int r, int g, int b)
{
    int ret = 0;
    int gray = 0;
    unsigned char led_buf[MAX_LED_GROUP_NUM * 4] = {0};
    unsigned char *p = NULL;
    if(idx >= MAX_LED_GROUP_NUM)
    {
        return -1;
    }
    gray = rgb_to_gray(r, g, b);
    return pwm_led_set(idx, gray);
}

static int led_parse_data(const unsigned char *src, size_t n)
{
    size_t src_idx;
    int gray;
    const size_t led_nums = n/3;

    for (src_idx = 0; src_idx < led_nums; ++src_idx) {
        const size_t src_offset = src_idx * 3;
        gray = rgb_to_gray(src[src_offset], src[src_offset+1], src[src_offset+2]);
        pwm_led_set(src_idx, gray);
    }
    return 0;
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

static int led_draw(struct ledarray_device_t *dev __unused,
		    unsigned char *buf, int len)
{
    unsigned char led_buf[MAX_LED_GROUP_NUM * 4];

    CHK_IF_ENABLED();
    CHK_IF_BUSY();
    if (len < 0 || len > MAX_LED_GROUP_NUM * PXL_FMT_RGB || (len % 3)) {
	ALOGE("invalid argument: len(%d)\n", len);
	return -EINVAL;
    }

   return led_parse_data(buf, len);
}

static int led_dev_close(struct ledarray_device_t *device)
{
    ALOGV("%s\n", __func__);
    if (pwm_export_finished) {
        for (int i = 0; i < PWM_NUMS; i++) {
            pwm_unexport(i + PWM_OFFSET);
        }
        pwm_export_finished = 0;
    }
    free(device);
    return 0;
}

static int led_dev_open(const hw_module_t * module,
			const char *name __unused, hw_device_t ** device)
{
    struct ledarray_device_t *ledadev = NULL;

    ALOGV("%s\n", __func__);
    ledadev = calloc(1, sizeof(ledarray_device_t));
    if (!ledadev) {
        ALOGE("Can not allocate memory for the leds device");
        return -ENOMEM;
    }


    ALOGV("%s\n", __func__);
    if( !pwm_export_finished ){
        for (int i = 0; i < PWM_NUMS; i++ ){
            pwm_export(i + PWM_OFFSET);
            pwm_port_init(i + PWM_OFFSET);
        }
        pwm_export_finished = 1;
    }
    ledadev->common.tag = HARDWARE_DEVICE_TAG;
    ledadev->common.module = (hw_module_t *) module;
    ledadev->common.version = HARDWARE_DEVICE_API_VERSION(1, 0);

    //ledadev->dev_code = DEVCODE;
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
    .name = "ROKID PWM LEDS HAL: The Coral solution.",
    .author = "The Linux Foundation",
    .methods = &led_module_methods,
};

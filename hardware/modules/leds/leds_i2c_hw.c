#define LOG_TAG "leds_hw"
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

#define LED_PATH "/sys/class/leds"

#define DEVNAME ROKIDOS_BOARDCONFIG_PRODUCT_MODULE

#define ROKIDOS_BOARDCONFIG_LED_NUMS LED_NUMS //defined in defconfig

#define MAX_LED_GROUP_NUM ROKIDOS_BOARDCONFIG_LED_NUMS
#define MAX_BRIGHTNESS    255
#define MAX_PATH_LEN      128

unsigned char LED_TABLE[MAX_LED_GROUP_NUM * PXL_FMT_RGB][12] = {
    "rgb00_blue", "rgb00_green", "rgb00_red",
    "rgb01_blue", "rgb01_green", "rgb01_red",
    "rgb02_blue", "rgb02_green", "rgb02_red",
    "rgb03_blue", "rgb03_green", "rgb03_red",
    "rgb04_blue", "rgb04_green", "rgb04_red",
    "rgb05_blue", "rgb05_green", "rgb05_red",
    "rgb06_blue", "rgb06_green", "rgb06_red",
    "rgb07_blue", "rgb07_green", "rgb07_red",
    "rgb08_blue", "rgb08_green", "rgb08_red",
    "rgb09_blue", "rgb09_green", "rgb09_red",
    "rgb10_blue", "rgb10_green", "rgb10_red",
    "rgb11_blue", "rgb11_green", "rgb11_red",
#if (ROKIDOS_BOARDCONFIG_LED_NUMS == 13)
    "rgb12_blue", "rgb12_green", "rgb12_red",
#elif (ROKIDOS_BOARDCONFIG_LED_NUMS > 13)
  #error "Throw an error?";
#endif
};

unsigned char LED_GLOBAL[] = "aw9523_led";

static int brightness = MAX_BRIGHTNESS;
static unsigned char api_enabled = 1;

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

static int write_int(char const *path, int value)
{
    int fd;
    char buffer[20];
    int bytes;
    ssize_t ret;

    ALOGV("path(%s), value(%d)\n", path, value);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
	ALOGE("%s failed to open (%s)\n", __func__, strerror(errno));
	return -errno;
    }

    bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
    ret = write(fd, buffer, (size_t) bytes);
    close(fd);

    if (ret < 0) {
	ALOGE("%s", strerror(errno));
	return -errno;
    }

    return 0;
}

static int read_int(char const *path, int *value)
{
    int fd;
    char buffer[20];
    ssize_t ret;

    ALOGV("path(%s)\n", path);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
	ALOGE("%s failed to open (%s)\n", __func__, strerror(errno));
	return -errno;
    }

    ret = read(fd, buffer, sizeof(buffer));
    close(fd);
    if (ret < 0) {
	ALOGE("%s", strerror(errno));
	return -errno;
    }

    errno = 0;
    (*value) = strtol(buffer, NULL, 0);
    if (errno) {
	ALOGE("%s failed to strtol (%s)\n", __func__, strerror(errno));
	return -errno;
    }

    return 0;
}

static int write_pixls(unsigned char *buf, int len)
{
    int fd;
    char path[MAX_PATH_LEN];
    ssize_t ret;

    ALOGV("%s\n", __func__);

    sprintf(path, "%s/%s/pixel", LED_PATH, LED_GLOBAL);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
	ALOGE("%s failed to open (%s)\n", __func__, strerror(errno));
	return -errno;
    }

    ret = write(fd, buf, len);
    close(fd);

    if (ret < 0) {
	ALOGE("%s", strerror(errno));
	return -errno;
    }

    return 0;
}

static int led_is_busy()
{
    char path[MAX_PATH_LEN];
    int busy = 0;
    int ret;

    sprintf(path, "%s/%s/busy", LED_PATH, LED_GLOBAL);
    ret = read_int(path, &busy);
    if (ret) {
	ALOGE("%s failed to read_int\n", __func__);
	return 0;
    }

#if 0
    return busy ? 1 : 0;
#else
    return 0;
#endif
}

#if 0
static int led_current_set(struct ledarray_device_t *dev __unused,
			   unsigned char level)
{
    int ret;
    char path[MAX_PATH_LEN];

    CHK_IF_ENABLED();
    CHK_IF_BUSY();

    ALOGV("%s: level(%d)\n", __func__, level);

    sprintf(path, "%s/%s/current_level", LED_PATH, LED_GLOBAL);
    ret = write_int(path, (int) level);
    if (ret)
	return ret;

    return 0;
}
#endif

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

#if 0
static int led_draw_one_inner(struct ledarray_device_t *dev __unused,
			      int idx, int br __unused, int r, int g,
			      int b)
{
    int ret = 0;
    char path[MAX_PATH_LEN];

    b = (unsigned char) (b & 0xFF) * brightness / MAX_BRIGHTNESS;
    g = (unsigned char) (g & 0xFF) * brightness / MAX_BRIGHTNESS;
    r = (unsigned char) (r & 0xFF) * brightness / MAX_BRIGHTNESS;

    sprintf(path, "%s/%s/brightness", LED_PATH, LED_TABLE[idx * 3]);
    ret += write_int(path, b);
    sprintf(path, "%s/%s/brightness", LED_PATH, LED_TABLE[idx * 3 + 1]);
    ret += write_int(path, g);
    sprintf(path, "%s/%s/brightness", LED_PATH, LED_TABLE[idx * 3 + 2]);
    ret += write_int(path, r);

    return ret;
}
#endif

#ifdef BOARD_ROKID_LED_PATCH_FOR_ERR_DIRECTION_1STPOS
static inline int led_index_relocate(int index)
{
    index = ((MAX_LED_GROUP_NUM - index)%MAX_LED_GROUP_NUM); // reverse.
    index = ((index + 3) % MAX_LED_GROUP_NUM); // first led pos need offset 9.
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
#endif

#if 0
static int led_draw_one(struct ledarray_device_t *dev, int idx,
			int br, int r, int g, int b)
{
    int ret = 0;

    CHK_IF_ENABLED();
    CHK_IF_BUSY();

    //ALOGW("!! argument (br) has been discarded, all pixels share the same br value\n");

    CHK_IF_OUT_OF_RANGE(b);
    CHK_IF_OUT_OF_RANGE(g);
    CHK_IF_OUT_OF_RANGE(r);

#ifdef BOARD_ROKID_LED_PATCH_FOR_ERR_DIRECTION_1STPOS
    idx = led_index_relocate(idx);
#endif

    ALOGV("%s: idx(%d), br(%d), r(%d), g(%d), b(%d)\n", __func__, idx, br,
	  r, g, b);

    ret = led_draw_one_inner(dev, idx, br, r, g, b);
    if (ret)
	return ret;
    return 0;
}
#endif

static void rgb2bgr(unsigned char *tmp_buf, int len)
{
    int i;
    unsigned char swap;

    for (i = 0; i < len; i += 3) {
	swap = tmp_buf[i];
	tmp_buf[i] = tmp_buf[i + 2];
	tmp_buf[i + 2] = swap;
    }
}

static int led_draw(struct ledarray_device_t *dev __unused,
		    unsigned char *buf, int len)
{
    int ret = 0;
    char path[MAX_PATH_LEN];
    unsigned char tmp_buf[MAX_LED_GROUP_NUM * PXL_FMT_RGB] = {0};

    CHK_IF_ENABLED();
    CHK_IF_BUSY();

    if (len < 0 || len > MAX_LED_GROUP_NUM * PXL_FMT_RGB || (len % 3)) {
	ALOGE("invalid argument: len(%d)\n", len);
	return -EINVAL;
    }

    ALOGV("%s: buf(%p), len(%d)\n", __func__, buf, len);

#ifdef BOARD_ROKID_LED_PATCH_FOR_ERR_DIRECTION_1STPOS
    led_reverse_mem(tmp_buf, buf, len);
#else
    memcpy(tmp_buf, buf, len);
#endif
    rgb2bgr(tmp_buf, len);
    ret = write_pixls(tmp_buf, len);
    if (ret)
	return ret;
    return 0;
}

static int led_dev_close(struct ledarray_device_t *device)
{
    ALOGV("%s\n", __func__);

    free(device);
    return 0;
}

static int led_dev_open(const hw_module_t * module,
			const char *name __unused, hw_device_t ** device)
{
    struct ledarray_device_t *ledadev =
	calloc(1, sizeof(ledarray_device_t));

    if (!ledadev) {
	ALOGE("Can not allocate memory for the leds device");
	return -ENOMEM;
    }

    ALOGV("%s\n", __func__);

    ledadev->common.tag = HARDWARE_DEVICE_TAG;
    ledadev->common.module = (hw_module_t *) module;
    ledadev->common.version = HARDWARE_DEVICE_API_VERSION(1, 0);


    ledadev->dev_code = NANA;
    ledadev->led_count = MAX_LED_GROUP_NUM;
    ledadev->pxl_fmt = PXL_FMT_RGB;

    ledadev->dev_close = led_dev_close;
    ledadev->draw = led_draw;
    //ledadev->draw_one = led_draw_one;
    ledadev->set_brightness = led_brightness_set;
    ledadev->set_enable = led_enable_set;
    //ledadev->set_current_reating = led_current_set;

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

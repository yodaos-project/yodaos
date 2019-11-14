#ifndef LEDARRAY_H
#define LEDARRAY_H

#include <hardware/hardware.h>

__BEGIN_DECLS;

#define LED_ARRAY_HW_ID "led_array"
#define LEDS_API_VERSION HARDWARE_MODULE_API_VERSION(1,0)

#ifndef __unused
#define __unused __attribute__((unused))
#endif
/*
#define EPERM 0x1
#define ENOENT 0x2
#define EIO 0x5
#define EBADF 0x9
#define EAGAIN 0xb
#define EACCES 0xd
#define EBUSY 0x10
#define ENODEV 0x13
#define EINVAL 0X16
*/
typedef enum {
    ALIEN,
    PEBBLE,
    MINI,
    NANA,
    SSD,
    NANO,
    NABOO,
    UNIVERSAL,
    K18_DEV33,
    K18_CUS360,
    COMMON
} ledarray_dev_code_t;

typedef enum {
    PXL_FMT_RGB = 3,
    PXL_FMT_ARGB = 4
} ledarray_pxl_fmt_t;

typedef enum {
    CMD_HAL_DISABLE = 0,
    CMD_HAL_ENABLE = 1
} ledarray_cmd_t;

struct led_module_t {
    struct hw_module_t common;
};

typedef struct ledarray_device_t {
    struct hw_device_t common;

    // led array device type for reference
    // system will fetch the device type info from other way
    ledarray_dev_code_t dev_code;

    // led number
    int led_count;

    // `pixel' format: rgb & argb
    // common we use rgb or we could use `a' channel to control the brightness
    // in this case, the frame size = led_count * pxl_fmt
    // led array application will alloc the frame buffer according to
    // these two parameters
    ledarray_pxl_fmt_t pxl_fmt;

    // FPS: frame per second
    int fps;

    // led array panel number
    int pannel_count;

    // header offset array in frame buffer for each led array panel
    // e.g.: ALIEN has three led array panel: face / suspension / headback
    int *pannel_offset_list;

    int fd;

    // this func ptr is useless
    // open_dev func should be given in struct hw_module_methods_t
    // led_open_dev
    int (*dev_close) (struct ledarray_device_t *dev); // led_close_dev
    int (*draw) (struct ledarray_device_t *dev,
                 unsigned char *buf,
                 int len); // led_flush_all
    int (*set_brightness) (struct ledarray_device_t *dev,
                           unsigned char br); // led_change_bright
    int (*draw_one) (struct ledarray_device_t *dev,
                     int idx, int br, int r, int g, int b); // led_flush_one
    int (*set_enable) (struct ledarray_device_t *dev,
                       int cmd); // led_hal_interface
    int (*set_current_reating) (struct ledarray_device_t *dev,
                                unsigned char level); // 1~4:Imax/4~Imax
} ledarray_device_t;

__END_DECLS;

#endif


/*
* author: tian.fan@rokid.com
*/

#include "LumenLight.h"
#include <assert.h>

LumenLight::LumenLight () {
    printf ("LumenLight ctor\n");
    initialize();
}

LumenLight::~LumenLight () {
    printf ("LumenLight dtor\n");
}

void LumenLight::initialize () {
    printf("%s\n", __func__);
    m_leds = chkDev();
    m_frameSize = m_leds->led_count * m_leds->pxl_fmt;
    m_ledCount = m_leds->led_count;
    m_fps = m_leds->fps;
    m_pixelFormat = m_leds->pxl_fmt;
    printf ("=== led light info ===\nf size: %d\nled count: %d\nfps: %d\npxl format: %d\n=====\n",
        m_frameSize,
        m_ledCount,
        m_fps,
        m_pixelFormat);
    return;
}

int LumenLight::lumen_draw(unsigned char* buf, int len) {
    if(len > m_frameSize){
        len = m_frameSize;
    }
    int ret = m_leds->draw(m_leds, buf, len);
    return ret;
}
ledarray_device_t *LumenLight::chkDev () {

    //ALOGV ("%s", __FUNCTION__);
    struct hw_module_t* hw_mod;
    ledarray_device_t *leds = NULL;

    int ret = hw_get_module (LED_ARRAY_HW_ID, (const struct hw_module_t **) &hw_mod);
    if (ret == 0)
        ret = hw_mod->methods->open (hw_mod, NULL, (struct hw_device_t **) &leds);
    else {
        printf ("failed to fetch hw module: %s\n", LED_ARRAY_HW_ID);
        goto lumen_dev_init_fail;
    }
    return leds;

lumen_dev_init_fail:

    if (leds)
        leds->dev_close (leds);
    return NULL;
}

void LumenLight::lumen_set_enable(bool cmd) {
    if (cmd)
        m_leds->set_enable(m_leds,1);
    else
        m_leds->set_enable(m_leds,0);
}
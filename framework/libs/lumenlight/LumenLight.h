#ifndef LUMEN_LIGHT_H
#define LUMEN_LIGHT_H
#include <stdio.h>
#include <rokidware/leds.h>

class LumenLight /*: public virtual RefBase */{
public:
    LumenLight ();
    virtual ~LumenLight ();

    /* set the interface lumen_draw be usrful or not */
    void lumen_set_enable(bool cmd);

    /* draw all led to show the color buff you set
     * buf[m_ledCount * m_pixelFormat]; len = sizeof(buf);
     * if m_pixelFormat == 3, means RGB
     * you can set buf[ledNum] = 0xff;  for the red component
     * you can set buf[ledNum + 1] = 0xff  for the green component
     * you can set buf[ledNum + 2] = 0xff  for the blue component
     * ledNum: from 0 to  (m_ledCount - 1)*/
    int lumen_draw(unsigned char* buf, int len);

    /* read framesize
     * framesize = m_ledCount *  m_pixelFormat */
    inline int getFrameSize()
    {
        return m_frameSize;
    }

    /* read led count */
    inline int getLedCount()
    {
        return m_ledCount;
    }

    /* read pixel format */
    inline int getPixelFormat()
    {
        return m_pixelFormat;
    }

    /* read the max fps that the lumenlight suggests */
    inline int getFps()
    {
        return m_fps;
    }

private:

    int m_frameSize;
    int m_ledCount;
    int m_pixelFormat;
    int m_fps;

    ledarray_device_t *m_leds = NULL;
    ledarray_device_t *chkDev (void);
    void initialize ();
};

#endif

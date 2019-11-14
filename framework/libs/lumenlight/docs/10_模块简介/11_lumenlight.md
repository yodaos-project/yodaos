## LumenLight

### 1. 简介

* LumenLight模块为应用层提供灯光绘制方法和LED信息。

### 2. API说明

#### 2.1 函数说明

##### 2.1.1  void lumen_set_enable(bool cmd)

* 设置 *lumen_draw* 是否生效。

##### 2.1.2  int lumen_draw(unsigned char* buf, int len)

* 写入LED像素值。

* buf：像素值数据。

* len：像素值数据长度。

##### 2.1.3  inline int getLedCount()

* 读取LED数目。

##### 2.1.4 inline int getPixelFormat()

* 读取像素格式，如：3代表RGB格式。

##### 2.1.5 inline int getFrameSize()

* 读取像素值总长度，*m_frameSzie =  m_ledCount *  m_pixelFormat*。

##### 2.1.6 inline int getFps()

* 读取建议使用的最大帧率，实际帧率大于该值时可能导致丢帧。
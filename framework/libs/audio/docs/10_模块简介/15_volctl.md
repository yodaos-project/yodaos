# vol\_ctrl模块简介

vol\_ctrl模块可以视为HAL层的功能，是对底层pulseaudio功能的封装，对上提供设置音量、静音、获取音量、获取当前播放状态、设置音量曲线等功能。

vol\_ctrl对上提供动态库和头文件"volumecontrol.h"

## rk\_stream\_type\_t

对上提供一个枚举类型，定义了不同的播放stream，原型如下：

	typedef enum
	{
	    STREAM_AUDIO = 0,
	    STREAM_TTS,
	    STREAM_RING,
	    STREAM_VOICE_CALL,
	    STREAM_ALARM,
	    STREAM_PLAYBACK,
	    STREAM_SYSTEM,
	}rk_stream_type_t;

每个stream就代表一个pulseaudio连接，含义如字面意思。

## 主要接口函数

### `extern int rk_set_volume(int vol);`

`vol`：整形，取值范围0~100，表示音量值；  
返回值：0；  
功能：设置**除alarm外**的其他所有stream的音量。

### `extern int rk_get_volume();`

功能：返回AUDIO或者TTS stream的音量值。  

**注**：另外还有`rk_set_all_volume`和`rk_get_all_volume`接口函数，是用来设置和获取设备音量的接口，建议不要使用，以免造成状态混乱。

### `extern int rk_set_mute(bool mute);`

功能：设置静音（true）或者取消静音（false）；  
返回值：0。

### `extern bool rk_is_mute();`

功能：获取静音状态，静音返回true，非静音返回false。

### `extern int rk_set_stream_volume(rk_stream_type_t stream_type, int vol);`

`stream_type`：rk\_stream\_type\_t 枚举类型；  
`vol`：整形，取值范围0~100，表示音量值；  
功能：设置指定stream的音量；  
返回值：成功返回0，失败返回-1。

### `extern int rk_get_stream_volume(rk_stream_type_t stream_type);`

功能：获取指定stream的音量值。

### `extern int rk_set_stream_mute(rk_stream_type_t stream_type, bool mute);`

功能：设置指定stream的静音状态，true：设置静音；false：取消静音。

### `extern bool rk_is_stream_mute(rk_stream_type_t stream_type);`

功能：获取指定stream的静音状态。

### `extern int rk_get_stream_playing_status(rk_stream_type_t stream_type);`

功能：获取指定stream的播放状态；  
返回值：正在播放返回1，传入参数错误或者未在播放状态返回0.  
**注**：只要有pulseaudio连接，就认为处在播放状态，常连接，就算不在播放状态，也会返回1。

### `extern int rk_setCustomVolumeCurve(int cnt,int* VolArray);`

功能：设置自定义音量曲线；  
`cnt`：音量曲线数组大小，要求为固定值：101；  
`VolArray`：音量曲线数组。

**注**：我们传入的音量值取值范围为0-100，实际设置到pulseaudio的值为0-65536，默认的音量转换公式为：

`(vol+156)^2`  

用户也可以通过此函数设置自己的音量曲线，但要求传入的音量曲线数组大小必须为101.
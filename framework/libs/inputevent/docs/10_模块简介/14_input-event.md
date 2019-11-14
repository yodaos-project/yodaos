# Input-event模块简介

input-event模块可以视为HAL层的功能，是对底层input事件的封装和处理，对上提供处理过的input时间，例如单击、双击、长按、滑动等事件。

input-event对上层提供动态库和头文件“input-event.h”。

## 主要结构体

input-event对上提供的事件，主要是两个结构体，`struct gesture`和`struct keyevent`，原型如下：

```
struct gesture {
    int key_code;
    int action;
    int slide_value;
    int click_count;
    bool new_action;
    long long_press_time;
};

struct keyevent {
    int action;
    int key_code;
    long key_time;
    struct timeval key_timeval;
    int value;
    bool new_action;
};
```

* `new_action`：表示是否有按键事件；  
* `key_code`：linux input.h中定义的标准key code，例如 114是KEY\_VOLUMEDOWN，115是KEY\_VOLUMEUP等；  
* `action`：表示按键事件，目前有如下几种：
	
	```
	#define ACTION_ORDINARY_EVENT  0
	#define ACTION_SINGLE_CLICK 1
	#define ACTION_DOUBLE_CLICK 2
	#define ACTION_LONG_CLICK 3
	#define ACTION_SLIDE 4
	```
* `slide_value`:slide事件使用，区分slide up和slide down两种不同操作；  
* `click_count`：暂未使用；  
* `long_press_time`：长按事件的时间；  
* `key_time`：按键的时间，单位ms；  
* `key_timeavl`：timeval格式的时间；  
* `value`：表示按下还是抬起。

## 主要接口函数

对上主要提供两个接口函数：

### `bool init_input_key(bool has_slide_event,int select_time_out,int double_click_timeout,int slide_timeout);`

* `has_slide_event `：bool型，表示是否支持滑动事件；
* `select_time_out `：select函数超时时间，即单次监测input事件的超时时间；
* `double_click_timeout `：判断是否是双击事件的超时时间，按键间隔超过这个时间即认为不是双击事件；
* `slide_timeout `：滑动事件的超时时间；
* init成功返回true，失败返回false。

### `void daemon_start_listener(struct keyevent * down_up,struct gesture * gest);`

* `down_up`：struct keyevent 指针类型，一个监测周期内的keyevent事件将填入指针指向的结构体；
* `gest`： struct gesture 指针类型，一个监测周期内的gesture事件将填入指针指向的结构体。

## 配置文件

配置文件位于openwrt/package/rokid/input-event/files目录，编译时copy到设备的/etc目录，命名为`input-event.conf`，具体的配置项参考以下解释：

	*[Global]*::
	Specifies all devices files to listen to. This option may be used more than
	once.
	
	*[Keys]*::
	All commands in this section are executed when the specified shortcut occurred.
	Modifiers are separated by the plus sign. A shortcut may be defined only once.
	
	*[Switches]*::
	This section defines commands which are executed when a specified switch is set
	to the defined value. Switch name and value are separated by a colon.
	
	*[Idle]*::
	The commands defined in this section are executed after all input devices did
	not send any events in the specified amount of time. The special key 'RESET'
	is triggered when returning from idle.
	
Global项下为需要监测的文件节点，最大支持32个；Keys Switches Idle等项下为具体的key event可以执行的shell命令，目前我们没有使用。

## 此模块的使用

首先修改配置文件，添加需要监测的input事件的文件节点，添加具体的Keys和Switches事件需要执行的命令(可选)；  
源码中添加头文件依赖：`#include <input-event/input-event.h>`；  
首先调用`init_input_key`函数初始化，根据设备具体情况设置是否支持slide，各种超时时间，返回true后，循环调用`daemon_start_listener`函数获取具体的输入事件，并进行相应业务处理。


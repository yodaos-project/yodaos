# BSP GUIDE
## RTC GUIDE

### 编程接口
rtc 作为一个硬件设备在文件系统中呈现，一般是 /dev/rtc0

操作这个设备过程是 open ioctl close

ioctl 常用的命令有：
```
RTC_RD_TIME，RTC_SET_TIME，RTC_ALM_READ，RTC_ALM_SET，RTC_AIE_ON
```

### linux命令
linux 自带一个 hwclock 的命令
```
hwclock –r        显示硬件时钟与日期
hwclock –s        将系统时钟调整为与目前的硬件时钟一致。
hwclock –w        将硬件时钟调整为与目前的系统时钟一致。
```

### Note

- RTC_ALM_SET 不会将时间立即设置到硬件，而是缓存起来，等调用RTC_AIE_ON的时候才会将时间设置到硬件。
- linux 里的 struct rtc_time 这个结构一般不是真正的时间， 年份需要加上1900，月份需要加上1
- rtc硬件模块使用的是ck的硬件模块，所有不能两边同时注册中断和写寄存器。操作要保持互斥性。
- alarm的任何设置需要开启RTC_CON的alarm_enable位才可写。
- time的任何设置需要关闭RTC_CON的moudle_enable位才可写。
- time 计时是一个时钟周期加10us，所以分频后的频率应该是 100000Hz

### 示例
示例代码见文件rtctest.c


## Reboot进入烧录功能

## 实现流程
### A7
1. 通过ioctl向ck发送进入下载请求
2. poweroff

### CK
1. 接收到A7端请求后，进入下载模式
2. 等待A7shutdown完成
3. 做重新载入下载固件的前期准备，包括关外围设备，关cache，关中断等
4. 从flash中读取固件到SRAM
5. 跳转到新的固件执行,准备接收新固件烧录

### 使用方法
首先boot.bin需要烧录到启动音乐分区(0x200000)
1. 通过adb shell登录到命令行
2. 输入burn_fw.sh，并等待adb shell自动退出
3. 在主机端输入以下命令升级
```
sudo ./bootx -m leo -t nnu -c downlaod <addr> <bin_file>
```

## 呼吸灯

呼吸灯的头文件为`vsp/mcu/include/driver/led_sgm31324.h`
接口如下：
```
int LedSGM31324Init(int i2c_bus, int i2c_chip);
int LedSGM31324SetColor(LED_COLOR rbg);
int LedSGM31324SetConfig(SGM31324_CONFIG *config);
int LedSGM31324Done(void);
```
### 示例代码
示例代码在vsp/cpu/test/led_test_13.c
编译vsp后会生成vsp/cpu/test/output/led_test_13文件，adb push到文件系统，执行该命令，会有跑马灯效果


## 给uboot传参

uboot下获得ck传递的参数通过通用寄存器来实现：
### 通用寄存器的分配情况
```
#define REG_OFFSET_COMMON_REG_DATA00    (0x0280)        // A7 -> CK
#define REG_OFFSET_COMMON_REG_DATA01    (0x0284)
#define REG_OFFSET_COMMON_REG_DATA02    (0x0288)
#define REG_OFFSET_COMMON_REG_DATA03    (0x028c)
#define REG_OFFSET_COMMON_REG_DATA04    (0x0290)
#define REG_OFFSET_COMMON_REG_DATA05    (0x0294)
#define REG_OFFSET_COMMON_REG_DATA06    (0x0298)
#define REG_OFFSET_COMMON_REG_DATA07    (0x029c)

#define REG_OFFSET_COMMON_REG_DATA08    (0x02a0)        // CK -> A7
#define REG_OFFSET_COMMON_REG_DATA09    (0x02a4)
#define REG_OFFSET_COMMON_REG_DATA10    (0x02a8)
#define REG_OFFSET_COMMON_REG_DATA11    (0x02ac)
#define REG_OFFSET_COMMON_REG_DATA12    (0x02b0)
#define REG_OFFSET_COMMON_REG_DATA13    (0x02b4)
#define REG_OFFSET_COMMON_REG_DATA14    (0x02b8)
#define REG_OFFSET_COMMON_REG_DATA15    (0x02bc)

#define REG_OFFSET_COMMON_REG_DATA16    (0x02c0)
#define REG_OFFSET_COMMON_REG_DATA17    (0x02c4)
#define REG_OFFSET_COMMON_REG_DATA18    (0x02c8)
#define REG_OFFSET_COMMON_REG_DATA19    (0x02cc)
#define REG_OFFSET_COMMON_REG_DATA20    (0x02d0)
#define REG_OFFSET_COMMON_REG_DATA21    (0x02d4)
#define REG_OFFSET_COMMON_REG_DATA22    (0x02d8)
#define REG_OFFSET_COMMON_REG_DATA23    (0x02dc)

#define REG_OFFSET_COMMON_REG_DATA24    (0x02e0)
#define REG_OFFSET_COMMON_REG_DATA25    (0x02e4)
#define REG_OFFSET_COMMON_REG_DATA26    (0x02e8)
#define REG_OFFSET_COMMON_REG_DATA27    (0x02ec)
#define REG_OFFSET_COMMON_REG_DATA28    (0x02f0)
#define REG_OFFSET_COMMON_REG_DATA29    (0x02f4)
#define REG_OFFSET_COMMON_REG_DATA30    (0x02f8)
#define REG_OFFSET_COMMON_REG_DATA31    (0x02fc)        // WAKEUP_REASON 用于CK向A7传递唤醒方式
#define REG_OFFSET_COMMON_REG_DATA32    (0x00c4)        // 用于休眠时kernel向uboot传递唤醒地址
#define REG_OFFSET_COMMON_REG_DATA33    (0x0174)        // 用于休眠/重启/关机kernel向mcu传递flag
#define REG_OFFSET_COMMON_REG_DATA34    (0x017c)
```

### 获取唤醒reason

唤醒Reason设置在 (MCU_REG_BASE_CONFIG + 0x02fc)中，可选值有
```
typedef enum {
	VSP_CPU_WAKEUP_REASON_COLD = 0,
	VSP_CPU_WAKEUP_REASON_BUTTON,   // User press button
	VSP_CPU_WAKEUP_REASON_KEYWORD,  // User Speak a keyword
	VSP_CPU_WAKEUP_REASON_WIFI,     // WiFi Module
	VSP_CPU_WAKEUP_REASON_RTC,      // RTC
	VSP_CPU_WAKEUP_REASON_CHARGER,  // Wall plug

} VSP_CPU_WAKE_UP_REASON;
```

## 低功耗接口

### 进入低功耗的前提条件
需要vsp支持standby模式才能进入低功耗,开启standby的方法如下：
make menuconfig
```
 VSP settings  --->
  [*] Has STANDBY workmode
    [ ]   Enable Wakeup by RTC
    [*]   Enable Wakeup by Button
    [ ]   Enable Wakeup by Wi-Fi Module
```
### 进入低功耗
示例代码在vsp/cpu/test/standby_test.c
编译完后会生成vsp/cpu/output/standby_test
把standby_test程序拷贝到文件系统，在vsp.ko加载起来后执行该命令就能进入低功耗模式

### 退出低功耗
按下GPIO8 volup-gpio 退出低功耗
该按钮是可配的，配置未知在mcu/board/nationalchip/nre_rkd_3v/include/board_config.h
宏#define CONFIG_BOARD_WAKEUP_BUTTON_GPIO 8

## 开关机管理MCU(HC18P110L)
- 关机命令
```
poweroff
```
- 重启命令
```
reboot
```

## ChargeIc和电量计
### 示例代码
```
vsp/cpu/test/pmu_test.c
```
### 使用方法
- 编译vsp工程，生成vsp/cpu/test/output/pmu_test工具
- 把pmu_test拷贝到文件系统，并执行
### 获取
```
./pmu_test get
```
### 设置
```
./pmu_test set
```

## 管脚复用配置
### 配置表

```
static const struct pin_config pin_table[] = {
/*   id| func // function0  | function1 | function2   | function3  | function4 */
    { 1, 0},  // POWERDOWN  | PD1PORT01 |
    { 2, 0},  // UART0RX    | PD1PORT02 |
    { 3, 0},  // UART0TX    | PD1PORT03 |
    { 4, 0},  // OTPAVDDEN  | PD1PORT04 |
    { 5, 0},  // SDBGTDI    | DDBGTDI   | SNDBGTDI    | PD1PORT05
    { 6, 0},  // SDBGTDO    | DDBGTDO   | SNDBGTDO    | PD1PORT06
    { 7, 0},  // SDBGTMS    | DDBGTMS   | SNDBGTMS    | PCM1INBCLK | PD1PORT07
    { 8, 0},  // SDBGTCK    | DDBGTCK   | SNDBGTCK    | PCM1INLRCK | PD1PORT08
    { 9, 0},  // SDBGTRST   | DDBGTRST  | SNBGTRST    | PCM1INDAT0 | PD1PORT09
    {11, 0},  // PCM1INBCLK | PD1PORT11
    {12, 0},  // PCM1INLRCK | PD1PORT12
    {13, 0},  // PCM1INDAT0 | PD1PORT13
    {14, 0},  // PCMOUTMCLK | DUARTTX   | SNUARTTX    | PD1PORT14
    {15, 0},  // PCMOUTDAT0 | SPDIF     | PD1PORT15
    {16, 0},  // PCMOUTLRCK | PD1PORT16
    {17, 0},  // PCMOUTBCLK | PD1PORT17
    {18, 0},  // UART1RX    | PD1PORT18
    {19, 0},  // UART1TX    | PD1PORT19
    {20, 0},  // DDBGTDI    | SNDBGTDI  | PD1PORT20
    {21, 0},  // DDBGTDO    | SNDBGTDO  | PD1PORT21
    {22, 0},  // DDBGTMS    | SNDBGTMS  | PD1PORT22
    {23, 0},  // DDBGTCK    | SNDBGTCK  | PD1PORT23
    {24, 0},  // DDBGTRST   | SNDBGTRST | PD1PORT24
    {25, 0},  // DUARTTX    | SNUARTTX  | PD1PORT25
    {26, 0},  // SDA0       | PD1PORT26
    {27, 0},  // SCL0       | PD1PORT27
    {28, 0},  // SDA1       | PD1PORT28
    {29, 0},  // SCL1       | PD1PORT29
    {30, 1},  // PCM0INDAT1 | PDMDAT3   | PD1PORT30
    {31, 1},  // PCM0INDAT0 | PDMDAT2   | PD1PORT31
    {32, 1},  // PCM0INMCLK | PDMDAT1   | PD1PORT32
    {33, 1},  // PCM0INLRCK | PDMDAT0   | PCM0OUTLRCK | PD1PORT33
    {34, 1},  // PCM0INBCLK | PDMCLK    | PCM0OUTBCLK | PD1PORT34
    {35, 0},  // IR         | PD1PORT35
};

```

### 配置方法
- 配置表中的第一列表示芯片的port口，第二列表示port口的功能
- 根据硬件给出的管脚复用表，对应修改每个port口的功能即可

### 代码位置

- ck域管脚复用
```
vsp/mcu/board/nationalchip/<板级>/misc_board.c
```
- a7域管脚复用
```
uboot/board/nationalchip/<板级>/pinmux.c
```

## LEO GPIO

### A7端使用

#### GPIO 规划
PD1开头的port对应ck域的port
PD2开头的port对应A7域的port

PD2PORT0 - PD2PORT31 对应 gpa0 组
PD2PORT32 - PD2PORT51 对应 gpa1 组

PD1PORT0 - PD1PORT31 对应ck_gpa0 组
PD1PORT32 - PD1PORT35 对应ck_gpa1 组

目前不提供ck的gpio中断在A7的支持
所有的GPIO都可以配置为中断模式，但要注意管脚复用配置正确，为GPIO模式

#### [应用层使用方法](https://github.com/torvalds/linux/blob/v4.4/Documentation/gpio/sysfs.txt)
echo 进 export 的数字有一个换算关系：
A7域的port echo 进 export 的数字就是PD1PORTXX的数字
ck域的port echo 进 export 的数字是PD1PORTXX的数字加上64
例子:
控制PD2PORT42脚
```
echo 42 > export
cd gpio42
echo out > direction  //设置管脚为输出
echo 1 > value        //设置管脚为高电平
```
控制PD1PORT10脚
```
echo 74 > export
cd gpio74
echo out > direction  //设置管脚为输出
echo 1 > value        //设置管脚为高电平
```

#### [驱动层使用方法](https://github.com/torvalds/linux/blob/v4.4/Documentation/gpio/consumer.txt)
port换算关系:
port 的数字就是PDXPORTXX的数字 % 32

例子:
```
控制PD2PORT1脚
设备树节点
    test:test{
        compatible = "nationalchip,LEO_A7-test";
        test1-gpio = <&gpa0 1 GPIO_ACTIVE_HIGH>;
    };

```

```
控制PD2PORT42脚
设备树节点
    test:test{
        compatible = "nationalchip,LEO_A7-test";
        test1-gpio = <&gpa1 10 GPIO_ACTIVE_HIGH>;
    };
```

```
控制PD1PORT10脚
设备树节点
    test:test{
        compatible = "nationalchip,LEO_A7-test";
        test1-gpio = <&ck_gpa0 10 GPIO_ACTIVE_HIGH>;
    };
```

驱动代码
```
desc = devm_gpiod_get(&pdev->dev, "test1", GPIOD_OUT_LOW);  //申请test1-gpio这个属性对应的GPIO口描述符，并设置为输出模式，输出低电平
gpiod_direction_input(desc);                                //设置desc描述符对应的GPIO口为输入模式
gpiod_to_irq(desc)                                          //获取此gpio对应的中断号，中断号要注意检查，大于0为有效的
gpiod_direction_output(desc,1);                             //设置desc描述符对应的GPIO口为输出模式，输出高电平
gpiod_set_value(desc,0);                                    //设置desc描述符对应的GPIO口输出值为0
gpio_level = gpiod_get_value(desc);                         //得到desc描述符对应的GPIO口的电平
devm_gpiod_put(&test_pdev->dev, desc);                      //释放desc描述符对应的GPIO口，这样别人才能申请这个GPIO口
```

### CK端使用


ck端只提供ck的gpio支持

在 include/driver/gpio.h 文件中有gpio的相关接口，相应功能如函数名所示

```
GPIO_DIRECTION GpioGetDirection(unsigned int port);
int GpioSetDirection(unsigned int port, GPIO_DIRECTION direction);
GPIO_LEVEL GpioGetLevel(unsigned int port);
int GpioSetLevel(unsigned int port, GPIO_LEVEL level);
int GpioEnableTrigger(unsigned int port, GPIO_TRIGGER_EDGE edge, GPIO_CALLBACK callback, void *pdata);
int GpioDisableTrigger(unsigned int port);
int GpioEnablePWM(unsigned int port, unsigned int freq, unsigned int duty);
int GpioDisablePWM(unsigned int port);
```

例子:
```
GpioSetDirection(10, GPIO_DIRECTION_OUTPUT) //设置ck的PD1PORT10为输出
GpioSetLevel(33, GPIO_LEVEL_HIGH)           //设置ck的PD1PORT33为高电平
```

## PWM
在kernel里只提供A7gpio的pwm功能，ck的gpio不支持

[官方Documentation](https://github.com/torvalds/linux/blob/v4.4/Documentation/pwm.txt)
[第三方博客](http://www.wowotech.net/comm/pwm_overview.html)

### 应用层
pwmchip0 对应gpa0组
pwmchip32对应gpa1组

```
控制PD2PORT26脚

echo 26 > /sys/class/pwm/pwmchip0/export
cd /sys/class/pwm/pwmchip0/pwm26
echo 0 > enable          //关闭此port的pwm
echo 100000 > period     //周期设置为100 us
echo  50000 > duty_cycle //高电平持续时间设置为 50 us
echo 1 > enable
```

```
控制PD2PORT33脚

echo 1 > /sys/class/pwm/pwmchip32/export
cd /sys/class/pwm/pwmchip32/pwm1
echo 0 > enable          //关闭此port的pwm
echo 100000 > period     //周期设置为100 us
echo  50000 > duty_cycle //高电平持续时间设置为 50 us
echo 1 > enable
```
#### 按键
按键驱动通过轮询来实现，通过input子系统上报事件。


### 驱动层

pwm0对应gpa0组,pwm1对应gpa1组
port换算关系:
port 的数字就是PDXPORTXX的数字 % 32

####接口

```
struct pwm_device *devm_pwm_get(struct device *dev, const char *con_id)  //获取pwm句柄
int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)       //配置pwn
int pwm_enable(struct pwm_device *pwm)                                   //开启pwm
oid pwm_disable(struct pwm_device *pwm)                                  //关闭pwm
```
例子：

```
控制PD2PORT26脚
设备树节点
test:test{
   compatible = "nationalchip,LEO_A7-test";
   pwms = <&pwm0 26 5000000>;
}
```

## pstore

pstore官方文档kernel源码
```
kernel/Documentation/ramoops.txt
```

### 开启办法

#### kernel
内核中需要开启pstore支持
```
File systems  --->
  <**>   Persistent store support
  [*]     Log kernel console messages
  [*]     Log user space messages
  <*>     Log panic/oops to a RAM buffer
```

#### uboot
uboot commandline传入以下参数

```
ramoops.mem_address=0x17000000 ramoops.mem_size=0x10000 ramoops.ecc=1
```
- ramoops.mem_address：pstore保存日志的地址
- ramoops.mem_size ：pstore使用内存大小

*注意*
uboot源码中修改环境变量后，需要格式化环境变量分区才能生效。可以在uboot命令中执行以下命令：
```
env default -f -a
save
```

### 测试
把内核源码kernel/tools/testing/selftests/pstore 目录下的内容拷贝到板子
1. 检查pstore功能是否正常
```
./pstore_tests
```
2. 制造panic重启
```
./pstore_crash_test
```
3. 检查crash文件是否生成
```
./pstore_post_reboot_tests
```

### 获取panic日志文件

crash重启后，挂载pstore文件系统
```
root@OpenWrt:~# mount -t pstore pstore /mnt
root@OpenWrt:~# ls -l /mnt
-r--r--r--    1 root     root          3283 Jan  1 00:07 console-ramoops-0
-r--r--r--    1 root     root          3607 Jan  1 00:07 dmesg-ramoops-0
-r--r--r--    1 root     root          9086 Jan  1 00:07 dmesg-ramoops-1
```

## 板级添加
### vsp
1. mcu/board/Kconfig添加板级入口项
2. mcu/board/Makefile添加spl_board.o、audio_board.o、misc_board.o编译入口
3. 拷贝mcu/board/nationalchip/<板级>,按规则重命名,修改文件里面的板级名字
4. 按硬件修改audio_board.c
5. 按硬件修改misc_board.c中的管脚复用

### uboot
1. arch/arm/dts/Makefile中添加设备树编译入口
2. 拷贝arch/arm/dts/leo_gx8010_dev_1v.dts文件，按规则重命名
3. arch/arm/mach-leo/Kconfig中添加板级的入口项
4. 拷贝board/nationalchip/leo_gx8010_dev_1v文件夹到当前目录，按规则重命名。命名规则为leo_<板级>
5. 遍历拷贝过后的文件，修改板级名字。如LEO_GX8010_DEV_V1、leo_gx8010_ssd_1v
6. 修改管脚复用，打开pinmux.c文件，按管脚配置修改pin_table结构体
7. 拷贝configs/leo_gx8010_dev_1v_defconfig,按规则重命名,修改文件里面的板级名字
8. 拷贝include/configs/leo_gx8010_dev_1v.h,按规则重命名,修改文件里面的板级名字和环境变量
9. 注意：sdram_config.h需要和使用封装一样的板级文件

### kernel
1. arch/arm/boot/dts/Makefile文件中添加设备树生成入口
2. arch/arm/boot/dts/ 目录中拷贝一个相似板级设备树文件

### openwrt
1. gackage/boot/uboot-leo/Makefile中添加uboot包
2. target/linux/leo/Makefile中添加板级入口
3. 拷贝target/linux/leo/<板级>目录，按规则重命名，遍历其中的文件，更改板级名。(如果存储器为spinand,拷贝target/linux/leo/gx8010-ssd-1v)
4. 更新configs/默认配置

## 单独编译内核
1. 设置环境变量(重新打开终端，只需执行一次)
```
source env.sh
```
2. cd kernel/
3. make ARCH=arm CROSS_COMPILE=arm-openwrt-linux- LOCALVERSION= leo_defconfig
4. make ARCH=arm CROSS_COMPILE=arm-openwrt-linux- LOCALVERSION= zImage dtbs

## 单独编译uboot
1. 设置环境变量(重新打开终端，只需执行一次)
```
source env.sh
```
2. cd uboot
3. make ARCH=arm CROSS_COMPILE=arm-openwrt-linux- <板级默认配置>
4. make ARCH=arm CROSS_COMPILE=arm-openwrt-linux-

## 单独编译vsp
1. 设置环境变量(重新打开终端，只需执行一次)
```
source env.sh
```

2. 生成.config
```
make <默认配置>
```
默认配置在configs/目录下：
```
rokid_me_defconfig
rokid_mini_defconfig
rokid_nano_defconfig
ssd_defconfig
```
例：
```
make rokid_me_defconfig
```
3. 编译


- 完整编译


```
make
```
- 编译mcu
```
make mcu
```
- 编译cpu(vsp.ko)
```
make cpu
```
- 编译dsp
```
make dsp
```


## perf工具使用

### 开启perf功能
make menuconfig
```
Development  --->
	<*> perf................................... Linux performance monitoring tool
```

### 使用方法
1. 使用perf record命令采样event
```
perf record -F 99 -a -g -- sleep 60
perf script > out.perf
```
2. 下载FlameGraph工具
```
git clone https://github.com/brendangregg/FlameGraph.git
```
使用其中的`stackcollapse-perf.pl`，`flamegraph.pl`脚本

3. 使用stackcollapse程序将堆栈样本折叠为单行
```
./stackcollapse-perf.pl out.perf > out.folded
```
4. 使用flamegraph.pl生成火焰图
```
./flamegraph.pl out.folded > out.svg
```
5. 把生成的svg文件拖到浏览器中观察


## 文件系统切换

切换根文件系统只需修改内涵command line

### 切换成ubifs
```
editenv mtdargs
ubi.mtd=9,2048 root=ubi0:rootfs rootwait rootfstype=ubifs rw loglevel=6
```

### 切换成squashfs
```
editenv mtdargs
root=/dev/mtdblock9 loglevel=6 rootwait
```

其中ubi.mtd=9,2048、root=/dev/mtdblock9中的9是根文件系统所在分区

## ubifs文件系统制作-c参数选择
使用mkfs.squashfs制作ubifs文件系统时，需要使用`-c`参数制定卷大小，卷的大小依据分区大小而定，公式如下：
分区大小(M)*1024/128
修改方法：
```
vi target/linux/leo/<板级>/profiles/*.mk
修改*_UBIFS_OPTS=".... -c
```

## OTP

OTP的使用请参考
```
rokidos/kernel/drivers/misc/nationalchip/otp_test.c
```

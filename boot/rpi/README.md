# 树莓派移植U-boot

## 编译U-boot

### 获取Uboot源码

渠道1.从GitHub或Uboot官网可以获取Uboot源码。这里提供官网FTP的地址。Uboot版本选择Uboot-2018.07.

[]: ftp://ftp.denx.de/pub/u-boot/u-boot-2018.07.tar.bz2

渠道2.从openai.rokid-inc.com获取经过修改的Uboot源码。(Gerrit:kamino_yodaos/gx8010/rpi-uboot)

### 获取ToolChain

渠道1：从Linaro.org 上获取交叉编译工具链 aarch64-linux-gnu。

[]: https://www.linaro.org/downloads/

渠道2: 从openai.rokid-inc.com获取编译工具链aarch-linux-gnu.(Gerrit:kamino_yodaos/gx8010/toolchains)

### 编译Uboot源码

编译环境：Ubuntu16.04/Ubuntu14.04

1.解压缩Uboot源码以及工具链。

3.进入Uboot目录中,执行以下命令。

```shell

1.配置环境变量
CROSS_COMPILE=/opt/gcc-linaro-6.2.1-2016.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-        												(替换为实际的交叉编译工具链目录)
ARCH=arm 												（替换为实际的被编译平台）

2.配置默认config
make rpi_3_defconfig ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

3.查看booti功能是否开启
make menuconfig ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} 
进入Menuconfig界面
进入Boot Commands选项 打开booti命令支持

4.开始编译
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j4 
```

编译过程中可能会出现依赖缺失的情况。请自行安装。

编译成功后当前目录下生成u-boot.bin文件。



## 编写 U-boot启动脚本

```shell
setenv bootargs "8250.nr_uarts=1 bcm2708_fb.fbwidth=1920 bcm2708_fb.fbheight=1080 bcm2708_fb.fbswap=1 vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000 dwc_otg.lpm_enable=0 console=ttyS0,115200 kgdboc=ttyS0,115200 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 rootwait
rw androidboot.rokidseed=${rokidseed} androidboot.serialno=${serialno} androidboot.devicetypeid=${devicetypeid}"

saveenv

fatload mmc 0:1 ${kernel_addr_r} Image
booti ${kernel_addr_r} - ${fdt_addr}
```

脚本首先通过fatload将Image(镜像文件名)加载到指定的内存地址上，通过booti启动内核。需要注意的的是，fdt_addr指向的内存上装载了由bootcode.bin加载的设备树。、

将上述代码保存在rpi3_bootscript.txt中。

### 编译 U-boot 启动脚本

安装u-boots-tools

```shell
sudo apt-get install u-boot-tools
```

编译U-boot启动脚本

```shell
sudo mkimage -A arm64 -O linux -T script -d ./rpi3-bootscript.txt ./boot.scr
```

编译成功后，当前目录下产生boot.scr文件。

## 烧录TF卡

将openwrt镜像烧录至TF卡中。烧录完成后，TF卡中有两个分区。(Mac可以通过Etcher 软件)

---boot

---rootfs

1.将TF卡挂在至Linux／Mac系统上，打开Boot分区。

2.将u-boot.bin&boot.scr拷入Boot分区。将kernel8.img重命名为Image（此处与U-boot脚本对应）。

3.修改config.txt,添加以下配置。

```shell
arm_contorl=0x200
kernel=u-boot.bin
```

4.将config.txt末尾的dtoverlay选项前的“#”去除。打开所有的设备树展开文件。

## 添加Key分区

sudo fdisk -l列出所有分区

sudo fdisk /dev/sdx进入fdisk命令界面准备格式化设备sdc

在fdisk界面输入n创建新分区，输入p创建逻辑primary分区，默认创建为primary分区，输入e创建为扩展分区，分区号选择使用默认值即可 起始位置选择2048 大小选择+512K

输入w完成所有分区保存

退出至控制台输入sudo mkfs.vfat /dev/sdxx格式化刚刚创建的分区。



Author：xinyu.zhu<xinyu.zhu@rokid.com>
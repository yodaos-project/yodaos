# OpenWrt用户手册

## 简介
通过openwrt可编译生成gxrobotos所需的运行环境及固件，它集成了gxdnn，vsp等驱动及api，并将sdk所需的linux kernel，uboot，mpd，senseflow等工程整合到一个编译环境

## 安装编译依赖工具
### Ubuntu16.04
```
sudo apt-get install build-essential subversion libncurses5-dev zlib1g-dev gawk gcc-multilib flex git-core gettext libssl-dev unzip texinfo device-tree-compiler dosfstools

```

### CentOS7
```
yum install -y unzip bzip2 dosfstools wget gcc gcc-c++ git ncurses-devel zlib-static openssl-devel svn patch perl-Module-Install.noarch perl-Thread-Queue
```
CentOS7需手动安装device-tree-compiler：
```
wget http://www.rpmfind.net/linux/epel/6/x86_64/Packages/d/dtc-1.4.0-1.el6.x86_64.rpm
rpm -i dtc-1.4.0-1.el6.x86_64.rpm
```

## 下载sdk代码
下载nationalchip的5个工程：openwrt, senseflow, kernel, uboot, mpd，放在同一目录下

## 编译
1. 进入openwrt目录
```
cd openwrt
```

2. 拷贝板级defconfig配置到.config
```
cp configs/<板级>_defconfig .config
```
3. 生成.config配置文件
```
make defconfig
```
4. make
```
make
```
如果想加快编译速度，请加-j参数
如果想查看详细编译信息，请加V=s参数

## FAQ
### 生成的文件有那些
openwrt编译生成的文件在openwrt/bin/<板级>/ 目录下：
```
-rwxrwxr-x  1 xukai xukai 1.7M May 17 15:19 bootx
drwxrwxr-x  5 xukai xukai 4.0K May 17 15:27 bootx_win
-rwxrwxr-x  1 xukai xukai 1.6K May 17 15:20 download.sh
-rwxrw-r--  1 xukai xukai  212 May 16 10:10 gen_swupdate_img.sh
-rw-rw-r--  1 xukai xukai 230K May 24 15:23 mcu.bin
-rw-r--r--  1 xukai xukai  930 May 24 16:53 md5sums
-rw-r--r--  1 xukai xukai  22K May 24 16:50 openwrt-leo-gx8010-rkd-3v.dtb
-rw-r--r--  1 xukai xukai 192M May 24 16:53 openwrt-leo-gx8010-rkd-3v-ext4.img
-rw-r--r--  1 xukai xukai 4.3M May 24 16:53 openwrt-leo-gx8010-rkd-3v-fit-uImage.itb
-rw-r--r--  1 xukai xukai  71M May 24 16:53 openwrt-leo-gx8010-rkd-3v-rootfs.tar.gz
-rw-r--r--  1 xukai xukai 197M May 24 16:53 openwrt-leo-gx8010-rkd-3v-sdcard-vfat-ext4.img
-rw-r--r--  1 xukai xukai  87M May 24 16:53 openwrt-leo-gx8010-rkd-3v.swu
-rw-r--r--  1 xukai xukai  83M May 24 16:53 openwrt-leo-gx8010-rkd-3v-ubifs.img
-rw-r--r--  1 xukai xukai  86M May 24 16:53 openwrt-leo-gx8010-rkd-3v-ubi.img
-rw-r--r--  1 xukai xukai 4.3M May 24 16:53 openwrt-leo-gx8010-rkd-3v-uImage
-rwxr-xr-x  1 xukai xukai 4.3M May 24 16:53 openwrt-leo-gx8010-rkd-3v-zImage
drwxr-xr-x 10 xukai xukai 4.0K May 24 16:54 packages
-rw-r--r--  1 xukai xukai 1.6K May 24 16:54 sha256sums
-rw-rw-r--  1 xukai xukai  523 May 16 10:10 sw-description
drwxr-xr-x  2 xukai xukai 4.0K May 24 16:10 uboot-leo-gx8010-rkd-3v
```
包括mcu, uboot，kernel，dtb，rootfs，ubifs，tf卡镜像等

### 怎样生成默认配置
1. 使用make menuconfig完成配置修改
2. 使用diffconfig.sh脚本生成配置
```
./scripts/diffconfig.sh > configs/<板级>_defconfig
```

### 怎样编译单个包
- 清理、编译、安装单个包
```
make package/<name>/{clean,compile,install}
```
- 编译清理, 编译, 安装工具链
```
make toolchain/{clean,compile,install}
```

### 如何单独编译uboot
1. 编译uboot
```
make package/uboot-leo/compile
```
2. install到openwrt/bin/目录下
```
make package/uboot-leo/install
```

### 如何单独编译kernel
1. 编译module
```
make target/linux/compile
```
2. 编译zImage、dts。install到openwrt/bin/目录下
```
make target/linux/install
```

### 如何修改openwrt package配置
```
make menuconfig
```

### 如何修改kernel配置
#### 修改所有板级
```
make kernel_menuconfig
```
#### 修改一个板级
```
make kernel_menuconfig CONFIG_TARGET=subtarget
```
当修改了内核的默认配置时，需要在openwrt中执行该命令，把配置同步到openwrt的内核配置中

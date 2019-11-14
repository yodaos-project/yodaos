.PHONY : linux win version clean

VERSION=v1.3.1
BUILD_DIR=${shell pwd}
CHANGELOG_H=${BUILD_DIR}/changelog.h
HELP_H=${BUILD_DIR}/help.h

LINUX_CPP=g++
WIN_CPP=i586-mingw32msvc-g++
ARMLINUX_CPP=arm-linux-gnueabi-g++
ARMLINUXANDROID_CPP=arm-linux-androideabi-g++

LINUX_CFLAGS= -m32 -Wall -O2 -g -Wno-unused-result
WIN_CFLAGS= -O2 -D__MINGW
ARMLINUX_CFLAGS= -Wall -O2 -g -Wno-unused-result
ARMLINUXANDROID_CFLAGS= -Wall -O2 -g -Wno-unused-result

WIN_INCLUDE=-I./libusb/output/windows_lib/include/libusb-1.0
WIN_LD_FLAGS=-L./libusb/output/windows_lib/MinGW32/static -lusb-1.0 -lmingw32 -lwsock32

LINUX_INCLUDE=-I./libusb/output/linux_lib/include/libusb-1.0
LINUX_LD_FLAGS=-L./libusb/output/linux_lib/lib -lusb-1.0 -lpthread

ARMLINUX_INCLUDE=-I./libusb/output/arm-linux_lib/include/libusb-1.0
ARMLINUX_LD_FLAGS=-L./libusb/output/arm-linux_lib/lib -lusb-1.0 -lpthread

ARMLINUXANDROID_INCLUDE=-I./libusb/output/arm-linux-android_lib/include/libusb-1.0
ARMLINUXANDROID_LD_FLAGS=-L./libusb/output/arm-linux-android_lib/lib -lusb-1.0 -lpthread

WIN_OUTPUT=bootx.exe
LINUX_OUTPUT=bootx
ARMLINUX_OUTPUT=bootx
ARMLINUXANDROID_OUTPUT=bootx

SRC=$(wildcard *.c *.cpp ./bootfile/*.c)

linux: start prepare
	@$(LINUX_CPP) -static $(LINUX_CFLAGS) ${LINUX_INCLUDE} $(SRC) ${LINUX_LD_FLAGS} -o $(LINUX_OUTPUT)
	@chmod +x $(LINUX_OUTPUT)
	@strip $(LINUX_OUTPUT)
	@rm ${CHANGELOG_H}
	@rm ${HELP_H}
	@echo "output : $(LINUX_OUTPUT)"

win: start prepare
	@$(WIN_CPP) -static $(WIN_CFLAGS) $(WIN_INCLUDE) $(SRC) ${WIN_LD_FLAGS} -o $(WIN_OUTPUT)
	@rm ${CHANGELOG_H}
	@rm ${HELP_H}
	@echo "output : $(WIN_OUTPUT)"

arm-linux: start prepare
	@$(ARMLINUX_CPP) -static $(ARMLINUX_CFLAGS) ${ARMLINUX_INCLUDE} $(SRC) ${ARMLINUX_LD_FLAGS} -o $(ARMLINUX_OUTPUT)
	@chmod +x $(ARMLINUX_OUTPUT)
	@strip $(ARMLINUX_OUTPUT)
	@rm ${CHANGELOG_H}
	@rm ${HELP_H}
	@echo "output : $(ARMLINUX_OUTPUT)"

arm-linux-android: start prepare
	@$(ARMLINUXANDROID_CPP) -static $(ARMLINUXANDROID_CFLAGS) ${ARMLINUXANDROID_INCLUDE} $(SRC) ${ARMLINUXANDROID_LD_FLAGS} -o $(ARMLINUXANDROID_OUTPUT)
	@chmod +x $(ARMLINUXANDROID_OUTPUT)
	@strip $(ARMLINUXANDROID_OUTPUT)
	@rm ${CHANGELOG_H}
	@rm ${HELP_H}
	@echo "output : $(ARMLINUXANDROID_OUTPUT)"

# 手动编译 Linux libusb 库，并安装至 ./libusb/output 下
libusb-linux:
	@cd libusb/; rm -rf output/linux_lib; mkdir output/linux_lib; ./configure --prefix=`pwd`/output/linux_lib/ --enable-static --enable-udev=no CC="gcc -m32"
	@make -C libusb && make -C libusb install

# 手动编译 Arm linux libusb 库，并安装至 ./libusb/output 下
libusb-armlinux:
	@cd libusb/; rm -rf output/arm-linux_lib; mkdir output/arm-linux_lib;./configure --host=arm-linux-gnueabi --prefix=`pwd`/output/arm-linux_lib CC=arm-linux-gnueabi-gcc --enable-udev=no
	@make -C libusb && make -C libusb install

clean:
	rm -f $(WIN_OUTPUT) $(LINUX_OUTPUT)

# ----------------------------- 其他准备工作 --------------------------------------
changelog:
	@echo "#ifndef __CHANGELOG_H__" >  ${CHANGELOG_H}
	@echo "#define __CHANGELOG_H__" >> ${CHANGELOG_H}
	@echo "#define CHANGELOG \"\\"  >> ${CHANGELOG_H}
	@sed 's/$$/& \\n\\/g' ${BUILD_DIR}/changelog.txt >>  ${CHANGELOG_H}
	@echo "\"" >> ${CHANGELOG_H}
	@echo "#endif" >> ${CHANGELOG_H}

bootx_help:
	@echo "#ifndef __HELP_H__" >  ${HELP_H}
	@echo "#define __HELP_H__" >> ${HELP_H}
	@echo "#define BOOTX_HELP \"\\"  >> ${HELP_H}
	@sed 's/$$/& \\n\\/g' ${BUILD_DIR}/help.txt >>  ${HELP_H}
	@echo "\"" >> ${HELP_H}
	@echo "#endif" >> ${HELP_H}

version:
	@sed -i "s/#define BOOTX_VERSION.*\(".*"\)/#define BOOTX_VERSION \"Version : $(VERSION) ($(shell date "+%Y%m%d"))\"/" main.c

prepare:changelog bootx_help version

start:
	@echo "Compiling ..."

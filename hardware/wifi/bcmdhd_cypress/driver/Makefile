# cywdhd
#################
# Usage and porting guide for this Makefile
#################
# 1 Default to the module external build, that is, no need to copy the driver into the kernel source tree
#  Build with:
#       make LINUX_DIR=/path/to/your/kernel
# 2 Default to x86 build: Build the x86 driver without any modification outside of kernel
# 3 Just change the 'DHD_PLATFORM=imx' and the IMx OOB version can work!
# 4 Copy DHD_PLATFORM=imx as the start point and create your own name

#####################
# SDIO Basic feature
#####################

##########################
# driver type
# m: module type driver
# y: built-in type driver
##########################
DRIVER_TYPE ?= m

DHDOFILES := dhd_pno.o dhd_common.o dhd_ip.o dhd_custom_gpio.o \
	dhd_linux.o dhd_linux_sched.o dhd_cfg80211.o dhd_linux_wq.o aiutils.o bcmevent.o \
	bcmutils.o bcmwifi_channels.o hndpmu.o linux_osl.o sbutils.o siutils.o \
	wl_android.o wl_cfg80211.o wl_cfgp2p.o wl_cfg_btcoex.o wldev_common.o wl_linux_mon.o  \
	dhd_linux_platdev.o dhd_pno.o dhd_linux_wq.o wl_cfg_btcoex.o \
        hnd_pktq.o hnd_pktpool.o wl_cfgvendor.o dhd_debug_linux.o bcmxtlv.o dhd_debug.o bcm_app_utils.o \
	wl_cfgnan.o dhd_gpio.o

ifneq ($(CONFIG_BCMDHD_SDIO),)
  DHDOFILES += bcmsdh.o bcmsdh_linux.o bcmsdh_sdmmc.o bcmsdh_sdmmc_linux.o
  DHDOFILES += dhd_cdc.o dhd_wlfc.o dhd_sdio.o
endif

ifneq ($(CONFIG_BCMDHD_PCIE),)
  DHDOFILES += dhd_pcie.o dhd_pcie_linux.o dhd_msgbuf.o dhd_flowring.o
  DHDOFILES += pcie_core.o
endif

cywdhd-objs := $(DHDOFILES)
obj-$(DRIVER_TYPE)   += cywdhd.o

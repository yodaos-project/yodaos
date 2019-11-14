# bcmdhd
# 1. WL_IFACE_COMB_NUM_CHANNELS must be added if Android version is 4.4 with Kernel version 3.0~3.4,
#    otherwise please remove it.

#CONFIG_BCMDHD := m
CONFIG_BCMDHD_SDIO := y
#CONFIG_BCMDHD_PCIE := y
#CONFIG_BCMDHD_USB := y

#CONFIG_BCMDHD_OOB := y
#CONFIG_BCMDHD_CUSB := y
CONFIG_BCMDHD_PROPTXSTATUS := y
#CONFIG_DHD_USE_STATIC_BUF := y
#CONFIG_VTS_SUPPORT := y

CONFIG_MACH_PLATFORM := y
#CONFIG_BCMDHD_DTS := y

DHDOFILES = aiutils.o siutils.o sbutils.o bcmutils.o bcmwifi_channels.o   \
	dhd_linux.o dhd_linux_platdev.o dhd_linux_sched.o dhd_pno.o           \
	dhd_common.o dhd_ip.o dhd_linux_wq.o dhd_custom_gpio.o                \
	bcmevent.o hndpmu.o linux_osl.o wldev_common.o wl_android.o           \
	dhd_debug_linux.o dhd_debug.o dhd_mschdbg.o                           \
	hnd_pktq.o hnd_pktpool.o dhd_config.o wl_android_ext.o

#BCMDHD_SDIO
ifneq ($(CONFIG_BCMDHD_SDIO),)
DHDOFILES += bcmsdh.o bcmsdh_linux.o bcmsdh_sdmmc.o bcmsdh_sdmmc_linux.o  \
	dhd_sdio.o dhd_cdc.o dhd_wlfc.o
endif

#BCMDHD_PCIE
ifneq ($(CONFIG_BCMDHD_PCIE),)
DHDOFILES += dhd_pcie.o dhd_pcie_linux.o pcie_core.o dhd_flowring.o       \
	dhd_msgbuf.o
endif

#BCMDHD_USB
ifneq ($(CONFIG_BCMDHD_USB),)
DHDOFILES += dbus.o dbus_usb.o dbus_usb_linux.o dhd_cdc.o dhd_wlfc.o
endif

#VTS_SUPPORT
ifeq ($(CONFIG_VTS_SUPPORT),y)
DHDOFILES += bcmxtlv.o dhd_rtt.o bcm_app_utils.o
endif

ifeq ($(CONFIG_MACH_PLATFORM),y)
	DHDOFILES += dhd_gpio.o
endif

ifeq ($(CONFIG_DHD_USE_STATIC_BUF),y)
	obj-{CONFIG_DHD_USE_STATIC_BUF} += dhd_static_buf.o
endif

ifneq ($(CONFIG_WIRELESS_EXT),)
	DHDOFILES += wl_iw.o wl_escan.o
endif
ifneq ($(CONFIG_CFG80211),)
	DHDOFILES += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o wl_cfg_btcoex.o wl_cfgvendor.o
	DHDOFILES += dhd_cfg80211.o
endif

obj-${CONFIG_BCMDHD} += bcmdhd.o
bcmdhd-objs += $(DHDOFILES)

#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bcmdhd_cypress
PKG_VERSION:=1.363.125.12
PKG_RELEASE:=2
USE_SOURCE_DIR := $(ROKID_DIR)/hardware/wifi/bcmdhd_cypress/driver

include $(INCLUDE_DIR)/package.mk

define KernelPackage/bcmdhd_cypress
  SUBMENU:=WIFI
  SECTION:=WIFI
  CATEGORY:=rokid
  TITLE:=Cypress common drivers
  FILES:=$(PKG_BUILD_DIR)/cywdhd.ko
endef

define KernelPackage/bcmdhd_cypress/description
 Kernel module for register a custom bcmdhd_cypress platform device.
endef

EXTRA_KCONFIG:= \
    CONFIG_BCMDHD=m CONFIG_BCMDHD_SDIO=y

ifeq ($(findstring y, $(CONFIG_TARGET_amlogic) $(CONFIG_TARGET_a113l)), y)
    EXTRA_KCONFIG += CONFIG_BCMDHD_OOB=y
endif

EXTRA_CFLAGS:= \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG))))

DHD_CFLAGS:= -Wall -Wstrict-prototypes -Dlinux -DLINUX -DBCMDRIVER            \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE            \
	-DSHOW_EVENTS -DBCMDBG -DWLP2P                                        \
	-DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT                                \
	-DKEEP_ALIVE -DCSCAN -DPKT_FILTER_SUPPORT                             \
	-DEMBEDDED_PLATFORM -DPNO_SUPPORT                                     \
	-DDHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT                           \
	-DCUSTOMER_HW2
DHD_CFLAGS += -DDHD_OF_SUPPORT -DCUSTOM_SDIO_F2_BLKSIZE=256

ifeq ($(findstring y, $(CONFIG_TARGET_amlogic) $(CONFIG_TARGET_a113l)), y)
DHD_CFLAGS += -DCUSTOMER_HW_AMLOGIC -DCUSTOMER_HW
DHD_CFLAGS += -DCUSTOMER_OOB -DHW_OOB
endif


DHD_CFLAGS += -DWL_CFG80211
# Print out kernel panic point of file and line info when assertion happened
DHD_CFLAGS += -DBCMASSERT_LOG

# keepalive
DHD_CFLAGS += -DCUSTOM_KEEP_ALIVE_SETTING=28000

DHD_CFLAGS += -DVSDB

# Fix __DATE__ & __TIME__ error on gcc version 4.9 or later
#GCCVERSION = $(shell $(CC) -dumpversion | cut -f1-2 -d.)
#ifeq ($(shell expr $(GCCVERSION) \>= 4.9),1)
  DHD_CFLAGS += -Wno-error=date-time -Wno-date-time
#endif

# For p2p connection issue
DHD_CFLAGS += -DWL_SCB_TIMEOUT=10

# TDLS enable
DHD_CFLAGS += -DWLTDLS
# For TDLS tear down inactive time 40 sec
DHD_CFLAGS += -DCUSTOM_TDLS_IDLE_MODE_SETTING=40000
# for TDLS RSSI HIGH for establishing TDLS link
DHD_CFLAGS += -DCUSTOM_TDLS_RSSI_THRESHOLD_HIGH=-60
# for TDLS RSSI HIGH for tearing down TDLS link
DHD_CFLAGS += -DCUSTOM_TDLS_RSSI_THRESHOLD_LOW=-70

# Roaming/
DHD_CFLAGS += -DROAM_AP_ENV_DETECTION
DHD_CFLAGS += -DROAM_ENABLE -DROAM_CHANNEL_CACHE -DROAM_API
DHD_CFLAGS += -DENABLE_FW_ROAM_SUSPEND
# Roaming trigger
DHD_CFLAGS += -DCUSTOM_ROAM_TRIGGER_SETTING=-75
DHD_CFLAGS += -DCUSTOM_ROAM_DELTA_SETTING=10
# Voice Enterprise
DHD_CFLAGS += -DWLFBT
# Set PM 2 always regardless suspend/resume
DHD_CFLAGS += -DSUPPORT_PM2_ONLY

# For special PNO Event keep wake lock for 10sec
DHD_CFLAGS += -DCUSTOM_PNO_EVENT_LOCK_xTIME=10
DHD_CFLAGS += -DMIRACAST_AMPDU_SIZE=8
#Vendor Extension
DHD_CFLAGS += -DWL_VENDOR_EXT_SUPPORT
#Gscan
# DHD_CFLAGS += -DGSCAN_SUPPORT
#RTT
#DHD_CFLAGS += -DRTT_SUPPORT -DRTT_DEBUG


#Link Statistics
DHD_CFLAGS += -DLINKSTAT_SUPPORT

# Early suspend
DHD_CFLAGS += -DDHD_USE_EARLYSUSPEND

# For Scan result patch
DHD_CFLAGS += -DESCAN_RESULT_PATCH

# Ioctl timeout 5000ms
DHD_CFLAGS += -DIOCTL_RESP_TIMEOUT=5000

# Prevent rx thread monopolize
DHD_CFLAGS += -DWAIT_DEQUEUE

# idle count
DHD_CFLAGS += -DDHD_USE_IDLECOUNT

# SKB TAILPAD to avoid out of boundary memory access
DHD_CFLAGS += -DDHDENABLE_TAILPAD

# Wi-Fi Direct
DHD_CFLAGS += -DWL_CFG80211_VSDB_PRIORITIZE_SCAN_REQUEST
DHD_CFLAGS += -DWL_CFG80211_STA_EVENT
DHD_CFLAGS += -DWL_IFACE_COMB_NUM_CHANNELS
DHD_CFLAGS += -DWL_ENABLE_P2P_IF
#SCAN time
DHD_CFLAGS += -DCUSTOM_SET_SHORT_DWELL_TIME
DHD_CFLAGS += -DCUSTOM_FORCE_NODFS_FLAG


# SoftAP
DHD_CFLAGS += -DSET_RANDOM_MAC_SOFTAP
DHD_CFLAGS += -DWL_CFG80211_ACL


#########################
# Chip dependent feature
#########################

DHD_CFLAGS += -DUSE_WL_TXBF
# DHD_CFLAGS += -DUSE_WL_FRAMEBURST
DHD_CFLAGS += -DCUSTOM_DPC_CPUCORE=0
DHD_CFLAGS += -DPROP_TXSTATUS_VSDB
DHD_CFLAGS += -DCUSTOM_PSPRETEND_THR=30
# New Features
#DHD_CFLAGS += -DWL11U
#DHD_CFLAGS += -DMFP
# The LPC would have the Tx 5G tput drop around 10Mbps but saving powers
# DHD_CFLAGS += -DDHD_ENABLE_LPC
DHD_CFLAGS += -DCUSTOM_COUNTRY_CODE

#  DHD_CFLAGS += -DBDC -DTOE -DDHD_BCMEVENTS -DMMC_SDIO_ABORT
  DHD_CFLAGS += -DBDC -DDHD_BCMEVENTS -DMMC_SDIO_ABORT
  DHD_CFLAGS += -DBCMSDIO -DBCMLXSDMMC -DUSE_SDIOFIFO_IOVAR
  DHD_CFLAGS += -DPROP_TXSTATUS
  DHD_CFLAGS += -DCUSTOM_AMPDU_MPDU=16
  DHD_CFLAGS += -DCUSTOM_AMPDU_BA_WSIZE=64
# tput enhancement
  DHD_CFLAGS += -DCUSTOM_GLOM_SETTING=8
  DHD_CFLAGS += -DBCMSDIOH_TXGLOM -DCUSTOM_TXGLOM=1
  DHD_CFLAGS += -DDHDTCPACK_SUPPRESS
  DHD_CFLAGS += -DRXFRAME_THREAD
  DHD_CFLAGS += -DCUSTOM_MAX_TXGLOM_SIZE=31
  DHD_CFLAGS += -DMAX_HDR_READ=128
  DHD_CFLAGS += -DDHD_FIRSTREAD=128
  # bcn_timeout
  DHD_CFLAGS += -DCUSTOM_BCN_TIMEOUT=5


# Print 802.1X packets
#DHD_CFLAGS += -DDHD_8021X_DUMP
# Print DHCP packets
#DHD_CFLAGS += -DDHD_DHCP_DUMP

## Attend to fix the suspend/resume -16 error issue, refer to SWDHD-409
DHD_CFLAGS += -DSUSPEND16
## Flags merged from Linux build
DHD_CFLAGS += -DCUSTOMER_IPCAM -DWL11ULB -DWL_NAN -DWL_SUPPORT_AUTO_CHANNEL -DWL_VIRTUAL_APSTA

## Flags that may depends on Linux kernel versions
DHD_CFLAGS += -DTSQ_MULTIPLIER=10

## Flags maybe useful but disable it

# The WLFC AC queue credit borrow
# DHD_CFLAGS += -DLIMIT_BORROW

# Advanced IBSS features
# DHD_CFLAGS += -DWLAIBSS -DWLAIBSS_PS

# p2P service discovery offload
# DHD_CFLAGS += -DWL_SDO

DHD_CFLAGS += -DDHD_DEBUG


EXTRA_CFLAGS += -I$(PKG_BUILD_DIR)/include -I$(PKG_BUILD_DIR) -I$(STAGING_DIR)/usr/include
EXTRA_CFLAGS += $(DHD_CFLAGS)

EXTRA_LDFLAGS += --strip-debug

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(USE_SOURCE_DIR)/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,bcmdhd_cypress))

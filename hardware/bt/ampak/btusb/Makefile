
# Specify Include folders
EXTRA_CFLAGS += -I$(SUBDIRS)
EXTRA_CFLAGS += -I$(SUBDIRS)/inc
EXTRA_CFLAGS += -I$(SUBDIRS)/src
EXTRA_CFLAGS += -I$(SUBDIRS)/src/gki
EXTRA_CFLAGS += -DEXPORT_SYMTAB

LITE ?= FALSE
ifeq ($(strip $(LITE)),TRUE)
EXTRA_CFLAGS += -DBUILDCFG
EXTRA_CFLAGS += -DBTUSB_LITE
EXTRA_CFLAGS += -I$(SUBDIRS)/../btpcm
EXTRA_CFLAGS += -I$(SUBDIRS)/../btsbc
COMPONENTS_PATH := $(SUBDIRS)/../../../../../Components
EXTRA_CFLAGS += -I$(COMPONENTS_PATH)/bta/include
EXTRA_CFLAGS += -I$(COMPONENTS_PATH)/stack/include
EXTRA_CFLAGS += -I$(COMPONENTS_PATH)/hcis
endif

SEC ?= FALSE
ifeq ($(strip $(SEC)),TRUE)
EXTRA_CFLAGS += -DBTUSB_LITE_SEC
EXTRA_CFLAGS += -I$(SUBDIRS)/../btsec
endif

EXTRA_CFLAGS += -DEXPORT_SYMTAB

# BTUSB_VID and BTUSB_PID can be defined to support a single BT controller type
# The following definitions are examples for 20702A1 chip
#EXTRA_CFLAGS += -DBTUSB_VID=0x0A5C
#EXTRA_CFLAGS += -DBTUSB_PID=0x22BE

usbobjs := \
        src/btusb.o \
        src/btusb_dev.o \
        src/btusb_isoc.o \
        src/btusb_version.o

ifeq ($(CONFIG_PROC_FS),y)
usbobjs += \
        src/btusb_proc.o
endif

ifeq ($(strip $(LITE)),TRUE)
liteobjs := src/btusb_lite.o \
            src/btusb_lite_av.o \
            src/btusb_lite_avdt.o \
            src/btusb_lite_l2c.o \
            src/btusb_lite_hci.o
endif

gkiobjs := \
        src/gki/gki_buffer.o \
        src/gki/gki_klinux.o

obj-m += btusb.o
btusb-objs := $(usbobjs) $(gkiobjs) $(liteobjs)

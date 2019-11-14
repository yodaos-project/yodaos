# makefile to build adbd natively for linux:

top_srcdir ?= ..
srcdir = $(top_srcdir)/adb
VPATH = $(srcdir)
include $(top_srcdir)/include.mk

LOCAL_PATH:= $(shell cd $(top_srcdir) && pwd)
adb_version := $(shell git -C $(LOCAL_PATH) rev-parse --short=12 HEAD 2>/dev/null)-android

LIBADB_SRC_FILES := \
    adb.cpp \
    adb_auth.cpp \
    adb_utils.cpp \
    adb_trace.cpp \
    adb_io.cpp \
    adb_listeners.cpp \
    diagnose_usb.cpp \
    shell_service.cpp \
    shell_service_protocol.cpp \
    sockets.cpp \
    transport.cpp \
    transport_local.cpp \
    transport_usb.cpp \
    log-non-android.cpp

LIBADB_linux_SRC_FILES := \
    fdevent.cpp \
    get_my_path_linux.cpp \

LOCAL_SRC_FILES := \
    $(LIBADB_SRC_FILES) \
    $(LIBADB_linux_SRC_FILES) \
    adb_auth_client.cpp \
    fdevent.cpp

LOCAL_SRC_FILES += \
    services.cpp \
	rk_sign.c \
    file_sync_service.cpp \
    framebuffer_service.cpp \
    remount_service.cpp \
    set_verity_enable_state_service.cpp \
    daemon/main.cpp

# DISABLED SOURCES for ADB_NON_ANDROID:
#   qemu_tracing.cpp
#   usb_linux.cpp
#   jdwp_service.cpp

LOCAL_SRC_C_FILES := \
	usb_linux_client.c

LOCAL_CFLAGS := -DADB_HOST=0 -Wall -Wno-unused-parameter
LOCAL_CFLAGS += -D_XOPEN_SOURCE -D_GNU_SOURCE -DHAVE_PTHREADS=1
LOCAL_CFLAGS += -DADB_NON_ANDROID=1 -DADB_REVISION='"$(adb_version)"'
LOCAL_CFLAGS += -DPROP_NAME_MAX=32 -DPROP_VALUE_MAX=92 -DALLOW_ADBD_NO_AUTH=1
LOCAL_INCLUDES := -I$(top_srcdir)/include -I$(top_srcdir)/base/include/ -I$(top_srcdir)/libcrypto_utils/include/ -I$(srcdir)

LOCAL_MODULE := adbd

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)

LOCAL_STATIC_LIBRARIES := libcutils libc

################################

OBJS = $(LOCAL_SRC_FILES:.cpp=.o) $(LOCAL_SRC_C_FILES:.c=.o)
CXXFLAGS = $(LOCAL_INCLUDES) $(OPT_CXXFLAGS) -std=c++14 $(LOCAL_CFLAGS)
CFLAGS= $(LOCAL_INCLUDES) $(OPT_CFLAGS) $(LOCAL_CFLAGS)
LOCAL_LDFLAGS := $(OPT_LDFLAGS)

%.o: %.cpp
	$(CXX) -g -c -fPIC $(CXXFLAGS) $(LFLAGS) $< -o $@

%.o: %.c
	$(CC) -g -c -fPIC $(CFLAGS) $(LFLAGS) $< -o $@

all: $(LOCAL_MODULE) xdg-adbd

$(LOCAL_MODULE): $(OBJS)
	$(CXX) -fPIC $(CXXFLAGS) $^ $(LOCAL_LDFLAGS)  -L ../libcutils/*.o ../base/*.o ../libcrypto_utils/*.o -lpthread -lresolv -lcrypto -lssl -lutil -o $@

xdg-adbd: xdg-adbd.c
	$(CC) -o $@ $< $(CFLAGS) $(LFLAGS) `pkg-config --libs --cflags glib-2.0`

clean:
	rm -f $(LOCAL_MODULE) xdg-adbd *.o daemon/*.o


top_srcdir ?= ..
srcdir = $(top_srcdir)/libcutils
VPATH = $(srcdir)
include $(top_srcdir)/include.mk

targetSmpFlag := -DANDROID_SMP=1

commonSources := \
	hashmap.c \
	native_handle.c \
	socket_inaddr_any_server_unix.c \
	socket_local_client_unix.c \
	socket_local_server_unix.c \
	socket_loopback_client_unix.c \
	socket_loopback_server_unix.c \
	socket_network_client_unix.c \
	sockets.cpp \
	config_utils.c \
	load_file.c \
	open_memstream.c \
	strdup16to8.c \
	strdup8to16.c \
	record_stream.c \
	process_name.c \
	properties_nonandroid.c \
	threads.c \
	sched_policy.c \
	iosched_policy.c

# Shared and static library for target
# ========================================================
LOCAL_MODULE := libcutils.a
LOCAL_SRC_FILES := $(commonSources) \
#	ashmem-dev.c \
	mq.c \
	android_reboot.c \
	partition_utils.c \
	uevent.c \
	qtaguid.c \
	klog.c
# LOCAL_SRC_FILES += memory.c

LOCAL_CXX_SRC_FILES := \
	sockets.cpp \
	sockets_unix.cpp

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_STATIC_LIBRARIES := liblog
LOCAL_CFLAGS += $(targetSmpFlag)

################################

OBJS = $(LOCAL_SRC_FILES:.c=.o) $(LOCAL_CXX_SRC_FILES:.cpp=.o)
EXTRA_CFLAGS = $(LOCAL_CFLAGS) -I$(top_srcdir)/include -I$(srcdir) -DHAVE_PTHREADS=1 -DADB_NON_ANDROID=1
EXTRA_CFLAGS += -DPROP_NAME_MAX=32 -DPROP_VALUE_MAX=92
CFLAGS = $(EXTRA_CFLAGS) $(OPT_CFLAGS)
CXXFLAGS = $(EXTRA_CFLAGS) $(OPT_CXXFLAGS)

all: $(LOCAL_MODULE)

%.o: %.cpp
	$(CXX) -c -fPIC $(CXXFLAGS) $(LFLAGS) $< -o $@

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $(LFLAGS) $< -o $@

$(LOCAL_MODULE): $(OBJS)
	$(AR) rcs $@ $?
	$(RANLIB) $@

clean:
	rm -f $(LOCAL_MODULE) *.o


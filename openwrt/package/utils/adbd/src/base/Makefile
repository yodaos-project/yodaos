top_srcdir ?= ..
srcdir = $(top_srcdir)/base
VPATH = $(srcdir)
include $(top_srcdir)/include.mk

targetSmpFlag := -DANDROID_SMP=1

commonSources := \
	errors_unix.cpp \
	file.cpp \
	logging.cpp \
	parsenetaddress.cpp \
	stringprintf.cpp \
	strings.cpp

# Shared and static library for target
# ========================================================
LOCAL_MODULE := libbase.a
LOCAL_SRC_FILES := $(commonSources) \
#	ashmem-dev.c \
	mq.c \
	android_reboot.c \
	partition_utils.c \
	uevent.c \
	qtaguid.c \
	klog.c
# LOCAL_SRC_FILES += memory.c

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_STATIC_LIBRARIES := liblog
LOCAL_CFLAGS += $(targetSmpFlag)

################################

OBJS = $(LOCAL_SRC_FILES:.cpp=.o)
CXXFLAGS = -std=c++0x $(LOCAL_CFLAGS) -I$(top_srcdir)/include/ -I$(srcdir)/include -I$(srcdir) -DHAVE_PTHREADS=1 -DADB_NON_ANDROID=1
CXXFLAGS += -DPROP_NAME_MAX=32 -DPROP_VALUE_MAX=92 $(OPT_CXXFLAGS)

all: $(LOCAL_MODULE)

%.o: %.cpp
	$(CXX) -g -c -fPIC $(CXXFLAGS) $(LFLAGS) $< -o $@

$(LOCAL_MODULE): $(OBJS)
	$(AR) rcs $@ $?
	$(RANLIB) $@

clean:
	rm -f $(LOCAL_MODULE) *.o


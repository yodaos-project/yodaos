top_srcdir ?= ..
srcdir = $(top_srcdir)/libcrypto_utils
VPATH = $(srcdir)
include $(top_srcdir)/include.mk

LOCAL_SRC_FILES = android_pubkey.c
LOCAL_MODULE = libcrypto_utils.a

################################

OBJS = $(LOCAL_SRC_FILES:.c=.o)
CFLAGS = $(LOCAL_CFLAGS) -I$(top_srcdir)/include/ -I$(srcdir)/include/ -I$(srcdir) -DHAVE_PTHREADS=1 -DADB_NON_ANDROID=1
CFLAGS += -DPROP_NAME_MAX=32 -DPROP_VALUE_MAX=92 $(OPT_CFLAGS)

all: $(LOCAL_MODULE)

%.o: %.c
	$(CC) -g -c -fPIC $(CFLAGS) $(LFLAGS) $< -o $@

$(LOCAL_MODULE): $(OBJS)
	$(AR) rcs $@ $?
	$(RANLIB) $@

clean:
	rm -f $(LOCAL_MODULE) *.o


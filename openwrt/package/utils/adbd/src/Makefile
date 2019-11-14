top_srcdir ?= .
VPATH = $(top_srcdir)
include $(top_srcdir)/include.mk

SUBDIRS= \
	adb \
	adb/daemon \
	base \
	libcrypto_utils \
	libcutils

all: adb/adbd adbd.service

makefiles:
	@ts=$(top_srcdir); \
	[ "$${ts#/}" = "$$ts" ] && ts=../$$ts; \
	if [ "$(top_srcdir)" != "." ] ; then \
	    for d in $(SUBDIRS) ; do \
                 mkdir -p $$d ; \
		 if [ -e "$(top_srcdir)/$$d/Makefile" ] ; then \
	             echo "top_srcdir=$$ts" > $$d/Makefile; \
	             echo "top_builddir=.." >> $$d/Makefile; \
                     echo "include \$$(top_srcdir)/$$d/Makefile" >> $$d/Makefile; \
	         fi \
	    done ; \
	fi

libcutils/libcutils.a: subdirs $(top_srcdir)/libcutils/*.c $(top_srcdir)/libcutils/*.cpp
	$(MAKE) -C libcutils

base/libbase.a: subdirs $(top_srcdir)/base/*.cpp
	$(MAKE) -C base

libcrypto_utils/libcrypto_utils.a: subdirs libcrypto_utils/android_pubkey.c
	$(MAKE) -C libcrypto_utils

adb/adbd adb/xdg-adbd: subdirs libcutils/libcutils.a base/libbase.a libcrypto_utils/libcrypto_utils.a $(top_srcdir)/adb/*.cpp adb/xdg-adbd.c
	$(MAKE) -C adb

clean: subdirs
	$(MAKE) -C libcutils clean
	$(MAKE) -C base clean
	$(MAKE) -C libcrypto_utils clean
	$(MAKE) -C adb clean

install: all
	install -d -m 0755 $(DESTDIR)$(sbindir)
	install -D -m 0755 adb/adbd $(DESTDIR)$(sbindir)
	install -D -m 0755 adb/xdg-adbd $(DESTDIR)$(sbindir)
	install -d -m 0755 $(DESTDIR)$(prefix)/lib/systemd/system/
	install -D -m 0644 $(top_srcdir)/adbd.service $(DESTDIR)$(prefix)/lib/systemd/system/

.PHONY: subdirs

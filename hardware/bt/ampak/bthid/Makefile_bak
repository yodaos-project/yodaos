
EXTRA_CFLAGS += -DLINUX_KERNEL -D_LINUX_ -DEXPORT_SYMTAB
 
obj-m := bthid.o

KDIR  := /lib/modules/$(shell uname -r)/build

PWD   := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

clean : 
	$(MAKE) -C $(KDIR) M=$(PWD) clean


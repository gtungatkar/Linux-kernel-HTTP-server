# 
# ksocket project
# BSD-style socket APIs for kernel 2.6 developers
# 
# @2007-2008, China
# @song.xian-guang@hotmail.com (MSN Accounts)
# 
# This code is licenced under the GPL
# Feel free to contact me if any questions
# 

.PHONY: clean install uninstall

obj-m := ksocket.o
#ksocket-objs := xxx.o yyy.o
KDIR  := /lib/modules/$(shell uname -r)/build
KMOD  := /lib/modules/$(shell uname -r)/kernel
PWD   := $(shell pwd)
EXTRA_CFLAGS = -Wall

#Building ksocket for debug version : make DEBUG=true
ifeq ($(DEBUG),true)
	EXTRA_CFLAGS += -D_sxg_debug_
endif

#Let the module believe all addr are safe to access: make ADDRSAFE=true
#This can give you some speed promotion if krecv/ksend are frequently called
#Use this only if you handled the addr-pointers issues in your own module
ifeq ($(ADDRSAFE),true)
	EXTRA_CFLAGS += -DKSOCKET_ADDR_SAFE
endif

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: ksocket.ko
	@install -vD ksocket.h $(KDIR)/include/net/ksocket.h
	@install -vD ksocket.ko $(KMOD)/net/ksocket/ksocket.ko
	-depmod -aq

uninstall:
	rm -rf $(KDIR)/include/net/ksocket.h $(KMOD)/net/ksocket/ksocket.ko
	-depmod -aq

clean:
	rm -rf *.ko *.o *.mod.* .H* .tm* .*cmd Module.symvers


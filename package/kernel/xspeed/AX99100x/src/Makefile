KDIR:=/lib/modules/$(shell  uname -r)/build/ 

DEBIAN_VERSION_FILE:=/etc/debian_version
DEBIAN_DISTRO:=$(wildcard $(DEBIAN_VERSION_FILE))
CURRENT=$(shell uname -r)
MAJORVERSION=$(shell uname -r | cut -d '.' -f 1)
MINORVERSION=$(shell uname -r | cut -d '.' -f 2)
SUBLEVEL=$(shell uname -r | cut -d '.' -f 3)

ifeq ($(MAJORVERSION),6)
MDIR=kernel/drivers/tty/serial
else
ifeq ($(MAJORVERSION),5)
MDIR=kernel/drivers/tty/serial
else
ifeq ($(MAJORVERSION),4)
MDIR=drivers/tty/serial
else
ifeq ($(MAJORVERSION),3)
MDIR=drivers/tty/serial
else
ifeq ($(MAJORVERSION),2)
ifneq (,$(filter $(SUBLEVEL),38 39))
MDIR=drivers/tty/serial
else
MDIR=drivers/serial
endif
else
MDIR=drivers/serial
endif
endif
endif
endif
endif




RM_PATH:= /lib/modules/$(shell uname -r)/$(MDIR)/ax99100x.ko

UNAME := $(shell uname -m)

ifeq ($(UNAME), x86_64)
obj-m +=ax99100x.o
obj-m +=ax99100x_pp.o
obj-m +=parport_pc.o
ax99100x-objs := ax99100x_spi.o ax99100x_sp.o
else
#ifeq ($(UNAME), aarch64)
obj-m +=ax99100x.o
obj-m +=ax99100x_pp.o
ax99100x-objs := ax99100x_sp.o
endif

default:
	$(RM) *.mod.c *.o *.ko .*.cmd *.symvers
	make -C $(KDIR) M=$(PWD) modules
	gcc -pthread select_BR.c -o select_BR
	gcc -pthread advanced_BR.c -o advanced_BR
	gcc -pthread gpio_99100.c -o gpio_99100
	gcc -pthread -Wall 9bit_test.c -o 9bit_test
	gcc -I /usr/include spi_test.c -o spi_test
	$(RM) *.mod.c *.o .*.cmd *.symvers *.mod
	rm -rf .tmp_version* *~
	rm -rf Module.markers modules.*	

install: default
ifeq ($(MAJORVERSION),6)
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	cp ax99100x.ko  /lib/modules/$(shell uname -r)/$(MDIR)
	su -c "/sbin/depmod -a"
	modprobe ax99100x
else ifeq ($(MAJORVERSION),5)
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	cp ax99100x.ko  /lib/modules/$(shell uname -r)/$(MDIR)
	su -c "/sbin/depmod -a"
	modprobe ax99100x
else
	cp ax99100x.ko  /lib/modules/$(shell uname -r)/kernel/$(MDIR)
	depmod -A
	chmod +x ax99100x
	cp ax99100x /etc/init.d/
ifeq ($(DEBIAN_DISTRO), $(DEBIAN_VERSION_FILE))
	ln -s /etc/init.d/ax99100x /etc/rcS.d/Sax99100x || true
else
	ln -s /etc/init.d/ax99100x /etc/rc.d/rc3.d/Sax99100x || true  	
	ln -s /etc/init.d/ax99100x /etc/rc.d/rc5.d/Sax99100x || true
endif
	modprobe ax99100x
endif

uninstall:
ifeq ($(MAJORVERSION),5)
ifneq ($(shell lsmod | grep electrum),)
	rmmod ax99100x
endif	
else ifeq ($(MAJORVERSION),6)	
ifneq ($(shell lsmod | grep electrum),)
	rmmod ax99100x
endif
	rm -rf $(RM_PATH)
	su -c "/sbin/depmod -a"
else
	modprobe -r ax99100x
	rm -f /lib/modules/$(shell uname -r)/kernel/$(MDIR)/ax99100x.*
	depmod -A
	rm -f /etc/init.d/ax99100x
ifeq ($(DEBIAN_DISTRO), $(DEBIAN_VERSION_FILE))
	rm -f /etc/init.d/ax99100x /etc/rcS.d/Sax99100x || true
else
	rm -f /etc/rc.d/rc3.d/Sax99100x
	rm -f /etc/rc.d/rc5.d/Sax99100x
endif
endif

clean:
	$(RM) *.mod.c *.o *.o.* *.ko .*.cmd *.symvers *.o.ur-safe *.mod *.o.d
	rm -rf .tmp_version* *~
	rm -rf Module.markers modules.* .cache.*
	rm -f .ax99100x_spi.o.d
	rm -f select_BR advanced_BR gpio_99100 spi_test 9bit_test

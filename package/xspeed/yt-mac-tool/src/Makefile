OPENWRT_LIB_DIR := $(STAGING_DIR)/usr/lib
$(warning ================================================)
$(warning $(OPENWRT_LIB_DIR))
$(warning $(CXX))
$(warning ================================================)
LIB_INPUT =-lpci

CPPSRC = .
TARGET = yt-mac-tool

SUBARCH := $(shell uname -m | sed -e s/i.86/x86/ -e s/x86_64/x86/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/s390x/s390/ -e s/parisc64/parisc/ -e s/ppc.*/powerpc/ -e s/mips.*/mips/ -e s/sh[234].*/sh/ -e s/aarch64.*/arm64/ -e s/riscv.*/riscv/) 
ARCH := $(SUBARCH)
ifeq ($(ARCH),x86 ) 
	CFLAGS += -D__$(ARCH)
endif 

objects = fuxipci_device.o fuxipci_lpbk.o fuxipci_lso.o fuxipci_net.o fuxipci_xsum.o fuxipci_linux_driver.o fuxipci_err.o fuxipci_linux_msc.o fuxipci_rss.o

ifeq ($(TARGET), yt-mac-tool)
	CPPSRC = ./fuxipci
    EXTRA_CFLAGS = yt-mac-burn.cpp user_pci_device.c ${LIB_INPUT}
else
    EXTRA_CFLAGS = fuxipci.cpp
endif
	CFLAGS = -I ./ -I $(CPPSRC) -DLINUX

$(TARGET):$(objects)
	@echo $(ARCH) cflags=$(CFLAGS) 
	$(CXX) -o $@ $(objects) $(CFLAGS) $(EXTRA_CFLAGS)
%.o:$(CPPSRC)/%.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

.PHONY:clean install $(TARGET)

clean : 
	rm -f $(TARGET)
	find ./ -iname "*.o" -exec rm -f {} \;

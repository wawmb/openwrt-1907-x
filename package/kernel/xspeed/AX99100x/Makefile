#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=AX99100x
PKG_RELEASE:=2.1.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/AX99100x
	CATEGORY:=X-Speed Kernel Modules
	TITLE:=PCIe to Multi I/O (4S, 2S+1P, 2S+SPI, LB) Controller
	FILES:=$(PKG_BUILD_DIR)/ax99100x.ko
	AUTOLOAD:=$(call AutoLoad,30,ax99100x)
	KCONFIG:=
endef

define KernelPackage/AX99100x/description
	The AX99100A is a PCIe to Multi I/O (4S, 2S+1P, 2S+SPI, LB) Controller that integrates a single-channel (X1)
	PCIe 2.0 Gen 1 endpoint controller and PHY with a variety of peripherals such as four high-speed serial ports,
	one parallel port, high-speed SPI master, I2C master, Local Bus (ISA-Like) and 24 GPIOs. AX99100A supports four modes,
	namely 4S (PCIe to Quad Serial), 2S+1P (PCIe to Dual Serial and Single Parallel), 2S+SPI (PCIe to Dual Serial and SPI),
	and LB (PCIe to Local Bus/ISA-Like). It is suitable for various I/O interface connectivity applications such as PCIe
	serial/parallel cards, PCIe data acquisition (DAQ) cards, industrial computers, industrial automation equipment,
	measurement instrumentation equipment, medical devices, POS terminals and industrial embedded systems.
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
	$(MAKE_OPTS) \
	modules
endef

$(eval $(call KernelPackage,AX99100x))

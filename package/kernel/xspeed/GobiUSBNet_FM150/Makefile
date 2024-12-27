#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=GobiUSBNet_FM150
PKG_RELEASE:=1.00.00.07

include $(INCLUDE_DIR)/package.mk

define KernelPackage/GobiUSBNet_FM150
  CATEGORY:=X-Speed Kernel Modules
  TITLE:=GobiUSBNet for fm150 9x07 support
  FILES:=$(PKG_BUILD_DIR)/GobiUSBNet_FM150.ko
  AUTOLOAD:=$(call AutoLoad,30,GobiUSBNet_FM150)
  KCONFIG:=
endef

define KernelPackage/GobiUSBNet_FM150/description
 This is a replacement for the following in-kernel drivers:
 GobiUSBNet_FM150

 Instead of generating input events (like in-kernel drivers do) it generates
 uevent-s and broadcasts them. This allows disabling input subsystem which is
 an overkill for OpenWrt simple needs.
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

$(eval $(call KernelPackage,GobiUSBNet_FM150))

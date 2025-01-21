#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=nf51xx
PKG_RELEASE:=1.3.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/nf51xx
	CATEGORY:=X-Speed Kernel Modules
	DEPENDS:=+libc +libpthread
	TITLE:=NF51xx nf5180 Controller
	FILES:=$(PKG_BUILD_DIR)/nf51xx/build/arm64el_xspeed_linux419_kernel/sdk_linuxkm.ko
	AUTOLOAD:=$(call AutoLoad,31,sdk_linuxkm)
	KCONFIG:=
endef

define KernelPackage/nf51xx/description
	NF51xx nf5180 Controller
	http://www.netforward-tech.com/
endef

MAKE_OPTS:= \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	KERNELDIR=$(LINUX_DIR)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/nf51xx arm64el_xspeed_linux419_kernel \
	$(MAKE_OPTS)
endef

define KernelPackage/nf51xx/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nf51xx/build/arm64el_xspeed_linux419_kernel/diag_shell $(1)/sbin
endef

$(eval $(call KernelPackage,nf51xx))

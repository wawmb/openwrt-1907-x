#
# Copyright (C) 2024 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=compiled-tools
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/compiled-tools
	CATEGORY:=X-Speed Modules
	TITLE:=Some compiled tools that don't have source code
	DEPENDS:=+libudev +libstdcpp +zlib
endef

define Package/compiled-tools/description
	Some compiled tools that don't have source code
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/compiled-tools/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sbin/* $(1)/sbin/
endef

$(eval $(call BuildPackage,compiled-tools))

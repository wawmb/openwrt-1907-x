#
# Copyright (C) 2024 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=lcm-api-tool
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/lcm-api-tool
	CATEGORY:=X-Speed Modules
	TITLE:=lcm-api-tool for lcmtest
endef

define Package/lcm-api-tool/description
	lcm-api-tool for lcmtest
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/lcm-api-tool/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lcm-api-tool $(1)/sbin
	ln -s lcm-api-tool $(1)/sbin/lcmtest
endef

$(eval $(call BuildPackage,lcm-api-tool))

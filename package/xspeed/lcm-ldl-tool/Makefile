#
# Copyright (C) 2024 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=lcm-ldl-tool
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/lcm-ldl-tool
	CATEGORY:=X-Speed Modules
	TITLE:=LCM_LocalDownload lcm-ldl-tool for LCM
endef

define Package/lcm-ldl-tool/description
	LCM_LocalDownload lcm-ldl-tool for LCM
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/lcm-ldl-tool/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lcm-ldl-tool $(1)/sbin
	ln -s lcm-ldl-tool $(1)/sbin/LCM_LocalDownload_arm
endef

$(eval $(call BuildPackage,lcm-ldl-tool))

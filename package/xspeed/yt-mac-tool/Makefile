#
# Copyright (C) 2024 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=yt-mac-tool
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/yt-mac-tool
	CATEGORY:=X-Speed Modules
	TITLE:=yt-mac-tool write mac for yt6801
	DEPENDS:=+libpci +libstdcpp
endef

define Package/yt-mac-tool/description
	yt-mac-tool write mac for yt6801
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/yt-mac-tool/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/yt-mac-tool $(1)/usr/bin
endef

$(eval $(call BuildPackage,yt-mac-tool))

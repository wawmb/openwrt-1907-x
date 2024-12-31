#
# Copyright (C) 2024 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=yt-led-tool
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/yt-led-tool
	CATEGORY:=X-Speed Modules
	TITLE:=Motorcomm NIC Burn Helper yt-led-tool for yt6801
	DEPENDS:=+libpci +libstdcpp
endef

define Package/yt-led-tool/description
	Motorcomm NIC Burn Helper yt-led-tool for yt6801
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/yt-led-tool/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/yt6801.cfg $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/yt-led-tool $(1)/sbin
	ln -s yt-led-tool $(1)/sbin/Motorcomm_NIC_Burn_Helper
endef

$(eval $(call BuildPackage,yt-led-tool))

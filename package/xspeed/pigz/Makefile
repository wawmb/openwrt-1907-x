#
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# https://github.com/madler/pigz/tree/master
include $(TOPDIR)/rules.mk

PKG_NAME:=pigz
PKG_VERSION:=2.8
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/madler/pigz/tar.gz/refs/tags/v2.8
PKG_HASH:=eb872b4f0e1f0ebe59c9f7bd8c506c4204893ba6a8492de31df416f0d5170fd0

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/pigz
	CATEGORY:=X-Speed Modules
	TITLE:=A parallel implementation of gzip
	URL:=https://zlib.net/pigz/
	DEPENDS:=+zlib
endef

define Package/pigz/description
	A parallel implementation of gzip for modern multi-processor, multi-core machines
	https://zlib.net/pigz/
endef

define Build/Configure
	$(call Build/Configure/Default)
endef

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
    CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -Dloff_t=off_t"
endef

define Package/pigz/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pigz $(1)/sbin/
endef

$(eval $(call BuildPackage,pigz))

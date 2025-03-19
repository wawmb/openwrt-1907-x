#
# Copyright (C) 2025 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 10812 2008-04-13 12:06:07Z olli $
#
include $(TOPDIR)/rules.mk

PKG_NAME:=parted
PKG_VERSION:=3.6
PKG_RELEASE:=1

PKG_INSTALL:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://ftp.gnu.org/gnu/parted/
PKG_HASH:=3b43dbe33cca0f9a18601ebab56b7852b128ec1a3df3a9b30ccde5e73359e612

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/parted
	CATEGORY:=X-Speed Modules
	TITLE:=parted partition editor
	URL:=http://www.gnu.org/software/parted/index.shtml
	DEPENDS:=+libuuid +libblkid
endef

define Package/parted/description
	parted Partition editor
	http://www.gnu.org/software/parted/index.shtml
endef

define Build/Configure
	$(call Build/Configure/Default, \
		--without-readline \
		--disable-device-mapper \
		--disable-nls \
	)
endef

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
    CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -Dloff_t=off_t"
endef

define Package/parted/install	
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP)          $(PKG_INSTALL_DIR)/usr/lib/*.so*      $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/parted    $(1)/usr/sbin	
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/partprobe $(1)/usr/sbin
endef

$(eval $(call BuildPackage,parted))

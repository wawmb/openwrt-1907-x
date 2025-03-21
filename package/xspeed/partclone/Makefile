#
# Copyright (C) 2025 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# https://github.com/Thomas-Tsai/partclone/releases
include $(TOPDIR)/rules.mk

PKG_NAME:=partclone
PKG_VERSION:=0.3.27
PKG_RELEASE:=1

PKG_INSTALL:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/Thomas-Tsai/partclone/tar.gz/refs/tags/0.3.27
PKG_HASH:=933e81dceb9c05bd2cb6027ef115a4422eb35b2bc05ed9a5fd42f637dfba5c17

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/partclone
	CATEGORY:=X-Speed Modules
	TITLE:=Partclone is a project similar to backup
	URL:=https://github.com/Thomas-Tsai/partclone
	DEPENDS:=+libblkid +libext2fs +libopenssl
endef

define Package/partclone/description
	Partclone provides utilities to backup a partition smartly and it is designed for higher compatibility of the file system by using existing library.
	https://github.com/Thomas-Tsai/partclone
endef

define Build/Configure
	cd $(PKG_BUILD_DIR) && ./autogen
	cd $(PKG_BUILD_DIR) && ./configure --enable-extfs --enable-fat --enable-xfs
endef

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
    CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -Dloff_t=off_t 	-L$(PKG_BUILD_DIR)../e2fsprogs-1.47.2/.pkgdir/libext2fs/usr/lib"
endef

define Package/partclone/install	
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/sbin/partclone.info $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/sbin/partclone.extfs $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/sbin/partclone.fat $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/sbin/partclone.xfs $(1)/usr/sbin
endef

$(eval $(call BuildPackage,partclone))

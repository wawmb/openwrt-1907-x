# Copyright (C) 2007-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=libdrm
PKG_VERSION:=2.4.100
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://dri.freedesktop.org/libdrm
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_HASH:=c77cc828186c9ceec3e56ae202b43ee99eb932b4a87255038a80e8a1060d0a5d

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

PKG_MAINTAINER:=Lucian Cristian <lucian.cristian@gmail.com>

include $(INCLUDE_DIR)/package.mk

define Package/libdrm
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libpciaccess
  TITLE:=libdrm
  URL:=https://dri.freedesktop.org/
endef

define Package/libdrm/description
  LIBDRM is the cross-driver middleware which allows user-space applications
  (such as Mesa and 2D drivers) to communicate with the Kernel by the means
  of the DRI protocol.
endef

CONFIGURE_ARGS += \
  --disable-udev \
  --enable-kms \
  --disable-omap-experimental-api \
  --disable-exynos-experimental-api \
  --disable-tegra-experimental-api \
  --disable-cairo-tests \
  --disable-manpages \
  --disable-valgrind

define Build/InstallDev
	$(INSTALL_DIR) \
	  $(1)/usr/include \
	  $(1)/usr/lib

	$(CP) \
	  $(PKG_INSTALL_DIR)/usr/include/* \
	  $(1)/usr/include/

	$(CP) \
	  $(PKG_INSTALL_DIR)/usr/lib/{pkgconfig,*.so*} \
	  $(1)/usr/lib/
endef

define Package/libdrm/install
	$(INSTALL_DIR) \
	  $(1)/usr/lib

	$(CP) \
	  $(PKG_INSTALL_DIR)/usr/lib/*.so* \
	  $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/modetest/.libs/modetest $(1)/usr/bin/
endef

$(eval $(call BuildPackage,libdrm))

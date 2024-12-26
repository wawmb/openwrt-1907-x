#
# For a description of this  Version 0.32 utility see:
#  https://sg.danny.cz/scsi/lsscsi.html
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=lsscsi
PKG_VERSION:=r0.32

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/doug-gilbert/lsscsi/archive/refs/tags/r0.32.tar.gz
PKG_HASH:=e65451da08f20e80883c3756f02b3e5e0f72d7e4271d073c05acbe26a180f20a

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/lsscsi
	CATEGORY:=X-Speed Modules
	TITLE:=lsscsi for read block dev info
endef

define Package/lsscsi/description
	lsscsi for read block dev info
	https://sg.danny.cz/scsi/lsscsi.html
endef

define Build/Configure
	$(call Build/Configure/Default)
endef

define Build/Compile
    $(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS)
endef

define Package/lsscsi/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/lsscsi $(1)/usr/bin
endef

$(eval $(call BuildPackage,lsscsi))

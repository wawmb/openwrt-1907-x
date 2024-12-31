#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# MTK Property Software.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=vendor_info
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/vendor_info
	CATEGORY:=X-Speed Modules
	TITLE:=vendor_info tool for rockchip write info
endef

define Package/vendor_info/description
	vendor_info tool for rockchip write info
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/vendor_info/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vendor_info $(1)/sbin
endef

$(eval $(call BuildPackage,vendor_info))

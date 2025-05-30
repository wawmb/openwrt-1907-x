#
# Copyright (C) 2020 Sebastian Kemper <sebastian_ml@gmx.net>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sofia-sip

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/freeswitch/sofia-sip.git
PKG_SOURCE_DATE=2022-01-26
PKG_SOURCE_VERSION:=1607ade8b6d1c4f82379c7489e7573a769b61994
PKG_RELEASE:=1
PKG_MIRROR_HASH:=560049069c43a8c0652a6f7034c005fb889b4ba3139dd79e0389e6e7d575dd97

# sofia-sip adds a version to include path
# need to update this when the version changes
VERSION_EQUIVALENT:=1.13

PKG_BUILD_PARALLEL:=1

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

PKG_LICENSE:=LGPL-2.1+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Sebastian Kemper <sebastian_ml@gmx.net>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/sofia-sip/Default
  SUBMENU:=Telephony
  URL:=http://sofia-sip.sourceforge.net/index.html
endef

define Package/sofia-sip
$(call Package/sofia-sip/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Sofia-SIP open-source SIP User-Agent library
  ABI_VERSION:=0
  DEPENDS:= \
	  +libopenssl \
	  +zlib
endef

define Package/sofia-sip/description
Sofia-SIP is an open-source SIP User-Agent library, compliant with the
IETF RFC3261 specification (see the feature table). It can be used as a
building block for SIP client software for uses such as VoIP, IM, and
many other real-time and person-to-person communication services. The
primary target platform for Sofia-SIP is GNU/Linux. Sofia-SIP is based
on a SIP stack developed at the Nokia Research Center. Sofia-SIP is
licensed under the LGPL.
endef

CONFIGURE_ARGS+= \
	--disable-stun \
	--without-doxygen \
	--without-glib

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/sofia-sip-$(VERSION_EQUIVALENT)/sofia-{sip,resolv} \
		$(1)/usr/share/sofia-sip
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/sofia-sip-$(VERSION_EQUIVALENT)/sofia-sip/*.h{,.in} \
		$(1)/usr/include/sofia-sip-$(VERSION_EQUIVALENT)/sofia-sip
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/sofia-sip-$(VERSION_EQUIVALENT)/sofia-resolv/*.h \
		$(1)/usr/include/sofia-sip-$(VERSION_EQUIVALENT)/sofia-resolv
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libsofia-sip-ua.{a,so*} $(1)/usr/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/sofia-sip-ua.pc \
		$(1)/usr/lib/pkgconfig
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/sofia-sip/{msg_parser,tag_dll}.awk \
		$(1)/usr/share/sofia-sip
endef

define Package/sofia-sip/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libsofia-sip-ua.so.$(ABI_VERSION)* \
		$(1)/usr/lib
endef

$(eval $(call BuildPackage,sofia-sip))

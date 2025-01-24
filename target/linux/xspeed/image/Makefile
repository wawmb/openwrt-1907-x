#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

include $(SUBTARGET).mk
$(warning $(DEVICE_DTS) $(KERNELNAME))
override DEVICE_DTS := $(DEVICE_DTS)

define Image/Build/Initramfs
	$(CP) $(KERNEL_BUILD_DIR)/vmlinux $(BIN_DIR)/vmlinux
	$(CP) $(KERNEL_BUILD_DIR)/vmlinux $(BIN_DIR)/$(IMG_PREFIX)$(if $(PROFILE_SANITIZED),-$(PROFILE_SANITIZED))-vmlinux
	$(CP) $(KERNEL_BUILD_DIR)/vmlinux-initramfs $(BIN_DIR)/$(IMG_PREFIX)$(if $(PROFILE_SANITIZED),-$(PROFILE_SANITIZED))-vmlinux-initramfs
	[ -e $(TOPDIR)/scripts/xspeed/mk_kernel_img.sh ] && \
		$(TOPDIR)/scripts/xspeed/mk_kernel_img.sh "$(KERNELNAME)" "$(PROFILE)" "$(BIN_DIR)" "$(KDIR)" "$(LINUX_DIR)" "$(DTS_DIR)" "$(DEVICE_DTS)"
endef

define Image/Build
	$(call Image/Build/$(1))
	cp $(KDIR)/root.$(1) $(BIN_DIR)/$(IMG_PREFIX)$(if $(PROFILE_SANITIZED),-$(PROFILE_SANITIZED)).img
	cp $(KDIR)/vmlinux $(BIN_DIR)/$(IMG_PREFIX)$(if $(PROFILE_SANITIZED),-$(PROFILE_SANITIZED))-vmlinux
endef

$(eval $(call BuildImage))

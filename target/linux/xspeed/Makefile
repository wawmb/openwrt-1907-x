#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
BOARD:=xspeed
BOARDNAME:=Xspeed Board
FEATURES:=fpu usb pci pcie gpio nand squashfs ramdisk targz rtc ubifs ext4
SUBTARGETS:=kernel_raxx linux4197_d2000 linux419246_e2000 lsdk1903_autotest lsdk1903_NH15 lsdk1903_NH25_autotest lsdk1903_NZXX_ate lsdk2012_autotest lsdk2012_nz60_autotest

KERNEL_PATCHVER:=4.19

define Target/Description
	Build firmware images for Xspeed boards.
endef

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))

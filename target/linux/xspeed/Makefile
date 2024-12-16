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
SUBTARGETS:=d2000 e2000 raxx nhxx_NH15 nhxx_NH25-aut nhxx_NZXX-ate nhxx_autotest nzxx_autotest nzxx_gen-auto nzxx_nz60-aut

KERNEL_PATCHVER:=4.19

define Target/Description
	Build firmware images for Xspeed boards.
endef

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))

#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
define Device/NZ01
  DEVICE_TITLE := NZ01
  DTS_DIR := $(DTS_DIR)/freescale/
  KERNEL_NAME := Image
  KERNEL := kernel-bin
endef
TARGET_DEVICES += NZ01

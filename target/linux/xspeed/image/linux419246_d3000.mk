#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
define Device/D3000
  DEVICE_TITLE := D3000
  DTS_DIR := $(DTS_DIR)/phytium/
  KERNEL_NAME := Image
  KERNEL := kernel-bin
endef
TARGET_DEVICES += D3000

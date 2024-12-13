#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
define Device/RA01
  DEVICE_TITLE := RA01
  DTS_DIR := $(DTS_DIR)/rockchip
  DEVICE_DTS := rk3566-xspeed-ra01-v010-linux_cfg1
endef
TARGET_DEVICES += RA01

define Device/RA02
  DEVICE_TITLE := RA02
  DTS_DIR := $(DTS_DIR)/rockchip
  DEVICE_DTS := rk3568-xspeed-ra02-v010-linux_cfg1
endef
TARGET_DEVICES += RA02

define Device/RA03
  DEVICE_TITLE := RA03
  DTS_DIR := $(DTS_DIR)/rockchip
  DEVICE_DTS := rk3568-xspeed-ra03-v010-linux_cfg1
endef
TARGET_DEVICES += RA03

define Device/RA05
  DEVICE_TITLE := RA05
  DTS_DIR := $(DTS_DIR)/rockchip
  DEVICE_DTS := rk3566-xspeed-ra05-v010-linux_cfg1
endef
TARGET_DEVICES += RA05

define Device/RA06
  DEVICE_TITLE := RA06
  DTS_DIR := $(DTS_DIR)/rockchip
  DEVICE_DTS := rk3566-xspeed-ra06-v010-linux_cfg1
endef
TARGET_DEVICES += RA06

define Device/RA08
  DEVICE_TITLE := RA08
  DTS_DIR := $(DTS_DIR)/rockchip
  DEVICE_DTS := rk3568-xspeed-ra08-v010-linux_cfg1
endef
TARGET_DEVICES += RA08

ifneq ($(strip $(findstring RA01,$(PROFILE))),)
  DEVICE_DTS := rk3566-xspeed-ra01-v010-linux_cfg1
else ifneq ($(strip $(findstring RA02,$(PROFILE))),)
  DEVICE_DTS := rk3568-xspeed-ra02-v010-linux_cfg1
else ifneq ($(strip $(findstring RA03,$(PROFILE))),)
  DEVICE_DTS := rk3568-xspeed-ra03-v010-linux_cfg1
else ifneq ($(strip $(findstring RA05,$(PROFILE))),)
  DEVICE_DTS := rk3566-xspeed-ra05-v010-linux_cfg1
else ifneq ($(strip $(findstring RA06,$(PROFILE))),)
  DEVICE_DTS := rk3566-xspeed-ra06-v010-linux_cfg1
else ifneq ($(strip $(findstring RA08,$(PROFILE))),)
  DEVICE_DTS := rk3568-xspeed-ra08-v010-linux_cfg1
else
endif
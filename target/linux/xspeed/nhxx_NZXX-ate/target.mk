#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
ARCH:=aarch64
BOARDNAME:=NXP lsdk-1903_NZXX-ate Board (ARM64)
KERNELNAME:=Image

define Target/Description
	Build firmware image for Xspeed NXP lsdk-1903_NZXX-ate Board.
	Ps: NZ01,NZ02,NZ10,NZ20,NZ21 models use this branch
endef

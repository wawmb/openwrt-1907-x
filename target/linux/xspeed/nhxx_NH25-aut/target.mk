#
# Copyright (C) 2024 Dustin <huangyanjie@x-speed.com.cn>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
ARCH:=aarch64
BOARDNAME:=NXP lsdk-1903_NH25-autotest Board (ARM64)
KERNELNAME:=Image dtbs

define Target/Description
	Build firmware image for Xspeed NXP lsdk-1903_NH25-autotest Board.
	Ps: Winicssec,NHO1,NHO2,NHO8,NH14,NH17,NH18,NH19,same NH25 models use this branch
endef

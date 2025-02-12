#!/bin/bash

################################################################################
#                            展示编译信息 编译img镜像                           #
################################################################################

KERNELNAME=$1
PROFILE=$2
BIN_DIR=$3
KDIR=$4
LINUX_DIR=$5
DTS_DIR=$6
DEVICE_DTS=$7

function RED() {
	echo -e "\033[31m$@\033[0m"
}

function BLUE() {
	echo -e "\033[34m$@\033[0m"
}

function find_substring() {
	case $1 in
	*"$2"*)
		return 0
		;; # substring found
	*)
		return 1
		;; # substring not found
	esac
}

function show_info {
	local width=70
	local title="Xspeed Board Info"
	local border=$(printf "%${width}s" | tr ' ' '*')
	local sub_border=$(printf "%${width}s" | tr ' ' '-')
	local title_length=${#title}
	local padding=$(((width - title_length) / 2 - 2))
	local formatted_title=$(printf "%${padding}s%s%${padding}s" "" "$title" "")
	RED "${border}"
	BLUE "$formatted_title"
	BLUE "${sub_border}"
	printf "  %12s %s\n" "PROFILE =" "${PROFILE}"
	printf "  %12s %s\n" "BIN_DIR =" "${BIN_DIR}"
	printf "  %12s %s\n" "KDIR =" "${KDIR}"
	printf "  %12s %s\n" "LINUX_DIR =" "${LINUX_DIR}"
	printf "  %12s %s\n" "DTS_DIR =" "${DTS_DIR}"
	printf "  %12s %s\n" "DEVICE_DTS =" "${DEVICE_DTS}"
	RED "${border}"
}

function rockchip_build_kernel_image {
	cp $KDIR/Image $LINUX_DIR
	rm $LINUX_DIR/resource.img

	script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
	target_file="${script_dir}/../../configs/.target.cfg"
	if [ -s "$target_file" ]; then
		rockchip_board=$(cat "$target_file" | awk -F '|' '{gsub(/^[ \t]+|[ \t]+$/, "", $1); gsub(/[ \t]+/, "", $4); print $4}')
	else
		RED "File [$target_file] does not exist or is empty."
		exit 1
	fi

	case "$rockchip_board" in
	RA01)
		board_dts=rk3566-xspeed-ra01-v010-linux_cfg1
		;;
	RA02)
		board_dts=rk3568-xspeed-ra02-v010-linux_cfg1
		;;
	RA03)
		board_dts=rk3568-xspeed-ra03-v010-linux_cfg1
		;;
	RA05)
		board_dts=rk3566-xspeed-ra05-v010-linux_cfg1
		;;
	RA06)
		board_dts=rk3566-xspeed-ra06-v010-linux_cfg1
		;;
	RA08)
		board_dts=rk3568-xspeed-ra08-v010-linux_cfg1
		;;
	"")
		break
		;;
	esac

	BLUE "Use $DTS_DIR/rockchip/${board_dts}.dtb!"
	if [ ! -f $DTS_DIR/rockchip/${board_dts}.dtb ]; then
		RED "File $DTS_DIR/rockchip/${board_dts}.dtb does not exist. Exiting..."
		exit 1
	fi

	if [ -f $LINUX_DIR/scripts/resource_tool ]; then
		$LINUX_DIR/scripts/resource_tool $DTS_DIR/rockchip/${board_dts}.dtb $LINUX_DIR/logo.bmp $LINUX_DIR/logo_kernel.bmp
		mv resource.img $LINUX_DIR
		BLUE "Pack resource.img!"
	fi

	if [ -f $LINUX_DIR/resource.img ]; then
		$LINUX_DIR/scripts/mkbootimg --kernel $LINUX_DIR/Image --second $LINUX_DIR/resource.img -o $LINUX_DIR/boot.img
		cp $LINUX_DIR/boot.img $BIN_DIR/boot.img
		BLUE "Pack boot.img!"
	else
		RED "$LINUX_DIR/resource.img is not exit."
	fi
}

show_info

KERNEL_NAME=$(echo "$1" | awk '{print $1}')
if [ -n "$KERNEL_NAME" ]; then
	cp "$KDIR/$KERNEL_NAME-initramfs" "$BIN_DIR/auto-factory.bin"
else
	RED "KERNEL_NAME is empty, not copying the file."
fi

if find_substring "$PROFILE" "RA"; then
	rockchip_build_kernel_image
fi

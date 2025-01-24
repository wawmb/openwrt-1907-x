#!/bin/bash
KERNELNAME=$1
PROFILE=$2
BIN_DIR=$3
KDIR=$4
LINUX_DIR=$5
DTS_DIR=$6
DEVICE_DTS=$7
LOCAL_DIR=$(pwd)

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

function mk_kernel_img {
	if [ -f $KDIR/Image ]; then
		cp $KDIR/Image $LINUX_DIR
	fi

	if [ -f $KDIR/image-${DEVICE_DTS}.dtb ]; then
		cp $KDIR/image-${DEVICE_DTS}.dtb $DTS_DIR/${DEVICE_DTS}.dtb
	fi

	if [ -f $LINUX_DIR/resource.img ]; then
		rm $LINUX_DIR/resource.img
	fi

	if [ -f $LINUX_DIR/scripts/resource_tool ]; then
		$LINUX_DIR/scripts/resource_tool $DTS_DIR/${DEVICE_DTS}.dtb $LINUX_DIR/logo.bmp $LINUX_DIR/logo_kernel.bmp
		mv resource.img $LINUX_DIR
		echo "pack resource.img"
	fi

	if [ -f $LINUX_DIR/resource.img ]; then
		$LINUX_DIR/scripts/mkbootimg --kernel $LINUX_DIR/Image --second $LINUX_DIR/resource.img -o $LINUX_DIR/boot.img
		cp $(LINUX_DIR)/boot.img $(BIN_DIR)/boot.img
		echo "pack boot.img"
	else
		echo "resource.img not exit"
	fi
}

show_info

KERNEL_NAME=$(echo "$1" | awk '{print $1}')
if [ -n "$KERNEL_NAME" ]; then
	cp "$KDIR/$KERNEL_NAME-initramfs" "$BIN_DIR/auto-factory.bin"
else
	echo "KERNEL_NAME is empty, not copying the file."
fi

if find_substring "$PROFILE" "RA"; then
	mk_kernel_img
fi

#!/bin/bash

################################################################################
#                                  编译img镜像                                 #
################################################################################

function RED() {
	echo -e "\033[31m$@\033[0m"
}

function BLUE() {
	echo -e "\033[34m$@\033[0m"
}

function pause() {
	RED "Press any key to quit:"
	read -n1 -s key
	exit 1
}

function main() {
	BLUE "Start to make update.img..."

	if [ ! -f "./parameter" -a ! -f "./parameter.txt" ]; then
		RED "Error：No found parameter!"
		exit 1
	fi

	if [ ! -f "package-file" ]; then
		RED "Error：No found package-file!"
		exit 1
	fi

	target_file="${script_dir}/../configs/.target.cfg"
	if [ -s "$target_file" ]; then
		rockchip_board=$(cat "$target_file" | awk -F '|' '{gsub(/^[ \t]+|[ \t]+$/, "", $1); gsub(/[ \t]+/, "", $4); print $4}')
	else
		RED "File [$target_file] does not exist or is empty."
		exit 1
	fi

	board_name="${rockchip_board,,}"
	cp u-boot/xspeed/$board_name/* u-boot/
	BLUE "Making ./u-boot/xspeed/$board_name/* OK."

	./afptool -pack ./ ./early_update.img || pause
	BLUE "Making ./early_update.img OK."

	./rkImageMaker -RK3568 ./MiniLoaderAll.bin ./early_update.img update.img -os_type:androidos || pause
	rm early_update.img
	BLUE "Making ./update.img OK."

	mv update.img ../bin/targets/xspeed/kernel419232_raxx-glibc/$rockchip_board-update.img
	BLUE "Moving update.img to ../bin/targets/xspeed/kernel419232_raxx-glibc/$rockchip_board-update.img OK."
	exit $?
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd $script_dir
main

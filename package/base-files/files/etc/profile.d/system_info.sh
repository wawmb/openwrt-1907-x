#!/bin/sh
#
# Copyright (C) 2025 Dustin <huangyanjie@x-speed.com.cn>
#

display_system_info() {
	echo
	echo "        Basic System Information:"
	echo "        ------------------------------------------------------"
	echo "        Current Time   : $(date)"
	echo "        System Version : $(cat /etc/os-release | grep -w 'PRETTY_NAME' | cut -d= -f2 | tr -d '"')"
	echo "        Kernel Version : $(uname -r)"
	echo "        HW Machine     : $(uname -m)"
	echo "        Board Name     : $(cat /etc/os-release | grep -w 'OPENWRT_BOARD' | cut -d= -f2 | tr -d '"')"
	echo "        Memory         : $(free -h | awk '/^Mem:/ { print $3 "/" $2 }')"
	echo
	echo "        Disk Usage Ranking:"
	echo "        ------------------------------------------------------"
	df -h | grep -vE 'tmpfs|devtmpfs|shm' | awk '{printf "\t%-12s %-6s %-6s %-11s %-6s %-9s\n",$1 ,$2, $3, $4 ,$5, $6}'
	echo
	echo "        NetWork Information:"
	echo "        ------------------------------------------------------"
	a=$(ifconfig -a | grep -E '^[a-z]' | awk '{print $1}' | grep -v 'lo')
	b=$(ifconfig -a | grep 'HWaddr' | awk '{print $NF}')
	i=0
	for iface in $a; do
		IP=$(ifconfig "$iface" | grep 'inet addr' | awk '{print $2}')
		current_hwaddr=$(echo "$b" | sed -n "$((i + 1))p")
		echo "$iface $current_hwaddr $IP" | awk '{printf "\t%-12s %-20s %-18s\n",$1 ,$2 ,$3}'
		i=$((i + 1))
	done
	echo
}

display_system_info

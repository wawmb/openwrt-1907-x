#!/usr/bin/env bash

function try_git() {
	target_file="${script_dir}/../configs/.target.cfg"
	kernel_path=$(cat "$target_file" | awk -F '|' '{gsub(/^[ \t]+|[ \t]+$/, "", $1); gsub(/[ \t]+/, "", $2); print $2}')
	cd ${script_dir}/../../$kernel_path
	git rev-parse --git-dir >/dev/null 2>&1 || return 1
	REV="r$(git rev-list --count HEAD)-$(git rev-parse --short HEAD)"
	[ -n "$REV" ]
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
try_git || REV="unknown"
echo "$REV"

#!/usr/bin/env bash
export LANG=C
export LC_ALL=C
[ -n "$TOPDIR" ] && cd $TOPDIR

GET_REV=$1

try_version() {
	[ -f version ] || return 1
	REV="$(cat version)"
	[ -n "$REV" ]
}

try_git() {
	REBOOT=e95528e3ce23c9358b5cccaff31e59eb96ae7763
	git rev-parse --git-dir >/dev/null 2>&1 || return 1

	[ -n "$GET_REV" ] || GET_REV="HEAD"

	case "$GET_REV" in
	r*)
		GET_REV="$(echo $GET_REV | tr -d 'r')"
		BASE_REV="$(git rev-list ${REBOOT}..HEAD | wc -l | awk '{print $1}')"
		REV="$(git rev-parse HEAD~$((BASE_REV - GET_REV)))"
		;;
	*)
		BRANCH="$(git rev-parse --abbrev-ref HEAD)"
		ORIGIN="$(git rev-parse --verify --symbolic-full-name ${BRANCH}@{u} 2>/dev/null)"
		REV="r$(git rev-list --count HEAD)-$(git rev-parse --short HEAD)"
		;;
	esac

	[ -n "$REV" ]
}

try_hg() {
	[ -d .hg ] || return 1
	REV="$(hg log -r-1 --template '{desc}' | awk '{print $2}' | sed 's/\].*//')"
	REV="${REV:+r$REV}"
	[ -n "$REV" ]
}

try_version || try_git || try_hg || REV="unknown"
echo "$REV"

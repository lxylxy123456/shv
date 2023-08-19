#!/bin/bash
#
# SHV - Small HyperVisor for testing nested virtualization in hypervisors
# Copyright (C) 2023  Eric Li
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

usage () {
	echo "$0: automatically configure and build SHV"
	echo '	-h              Print this help message.'
	echo '	-i, --i386      Use 32-bit paging.'
	echo '	-p, --pae       Use 32-bit PAE paging.'
	echo '	-a, --amd64     Use 64-bit 4-level paging.'
	echo '	-O <level>      Set GCC optimization level.'
	echo '	-n              Only print configuration command, do not execute.'
	exit 1
}

conf=()

DRY_RUN='n'

opt=$(getopt -o 'hipaO:n' --long 'i386,pae,amd64' -- "$@")
[ "$?" == "0" ] || usage
eval set -- "$opt"
while true; do
	case "$1" in
	-h)
		usage
		;;
	-i|--i386)
		conf+=("--host=i686-linux-gnu" "--disable-i386-pae")
		;;
	-p|--pae)
		conf+=("--host=i686-linux-gnu" "--enable-i386-pae")
		;;
	-a|--amd64)
		conf+=("--host=x86_64-linux-gnu")
		;;
	-O)
		[ "$2" = "0" -o "$2" = "1" -o "$2" = "2" -o "$2" = "3" -o \
		  "$2" = "s" -o "$2" = "fast" -o "$2" = "g" -o "$2" = "z" ] || usage
		conf+=("CFLAGS=-g -O$2")
		shift
		;;
	-n)
		DRY_RUN='y'
		;;
	--)
		break
		;;
	*)
		false
		;;
	esac
	shift
done

if [ "$DRY_RUN" = 'y' ]; then
	echo $'\n'"./autogen.sh; ./configure ${conf[@]@Q}"$'\n'
else
	set -xe
	autoreconf --install
	./configure "${conf[@]}"
	make -j "$(nproc)"
fi


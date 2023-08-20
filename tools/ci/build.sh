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
	if [ "$#" != "0" ]; then
		echo "Error: $@"
	fi
	echo "$0: automatically configure and build SHV."
	echo '	-h                  Print this help message.'
	echo '	-n                  Dry run, only print configuration command.'
	echo '	-s, --srcdir <DIR>  Configure source directory, instead of ".".'
	echo '	-A, --ac            Force performing autoreconf --install.'
	echo '	-i, --i386          Use 32-bit paging.'
	echo '	-p, --pae           Use 32-bit PAE paging.'
	echo '	-a, --amd64         Use 64-bit 4-level paging.'
	echo '	-O <level>          Set GCC optimization level.'
	echo '	-S, --shv-opt <OPT> Set SHV_OPT.'
	exit 1
}

# Initialize variables.
conf=()
DRY_RUN='n'
SRCDIR='.'
AC='n'

# Parse arguments.
opt=$(getopt -o 'hns:AipaO:S:' --long 'srcdir:,ac,i386,pae,amd64,shv-opt:' \
	-- "$@")
[ "$?" == "0" ] || usage 'getopt failed'
eval set -- "$opt"
while true; do
	#echo "$@"
	case "$1" in
	-h)
		usage
		;;
	-n)
		DRY_RUN='y'
		;;
	-s|--srcdir)
		SRCDIR="$2"
		shift
		;;
	-A|--ac)
		AC='y'
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
		  "$2" = "s" -o "$2" = "fast" -o "$2" = "g" -o "$2" = "z" ] || \
			usage "unknown argument to -O: $2"
		conf+=("CFLAGS=-g -O$2")
		shift
		;;
	-S|--shv-opt)
		conf+=("--with-shv-opt=$2")
		shift
		;;
	--)
		break
		;;
	*)
		usage "unknown argument to case: $1"
		;;
	esac
	shift
done

# Make sure there are no unparsed arguments.
if [ "$#" != "1" ]; then
	usage "unknown remaining arguments: $@"
fi

CONFIGURE="$SRCDIR/configure"

# Run configure / print the command.
if [ "$DRY_RUN" = 'y' ]; then
	echo $'\n'"autoreconf --install"$'\n'"./configure ${conf[@]@Q}"$'\n'
else
	set -xe
	if [ "$AC" = "y" -o ! -f "$CONFIGURE" ]; then
		pushd "$SRCDIR"
		autoreconf --install
		popd
	fi
	"$CONFIGURE" "${conf[@]}"
	make -j "$(nproc)"
fi


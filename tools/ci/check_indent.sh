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

set -xe

export LANG=C

NOTHING_MSG='nothing to commit, working tree clean'

check_git_status () {
	diff <(echo "$NOTHING_MSG") <(git status | tail -n +2)
}

check_git_status

for i in {1..10}; do
	echo "Try iter $i"
	tools/indent.sh
	if check_git_status; then
		echo "git status succeeded at iteration $i"
		exit 0
	else
		echo "git status failed"
	fi
	git clean -Xdf
	check_git_status
	git checkout HEAD^
	check_git_status
done

echo "Error: indent.sh failed for 10 iterations"
exit 1


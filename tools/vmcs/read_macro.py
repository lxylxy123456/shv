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

'''
Read VMCS macro definitions and output VMCS fields as CSV. From XMHF64.
'''

import sys, re, csv

F01 = 'include/_vmx_vmcs_fields.h'

def parse_macro(lines):
	# 'Virtual-processor identifier (VPID)'
	name = re.fullmatch('/\* (.+) \*/', lines[0]).groups()[0]
	# DECLARE_FIELD_16_RW(0x0000, control_vpid, (FIELD_PROP_CTRL), ..., UNUSED)
	macro = ' '.join(map(str.strip, lines[1:]))
	matched = re.fullmatch('DECLARE_FIELD_(16|64|32|NW)_(RO|RW)\((.+)\)', macro)
	# '16', 'RW', ...
	bits, write, args = matched.groups()
	# ['0x0000', 'control_vpid', ..., 'UNDEFINED']
	# TODO: currently ',' in macros is not supported
	lst = list(map(str.strip, args.split(',')))
	return name, bits, write, lst

def read_file(f):
	lines = open(f).read().split('\n')
	while len(lines) > 1:
		if lines[1].startswith('DECLARE_FIELD_'):
			end = 2
			while lines[end].strip() != 'UNDEFINED)':
				end += 1
			yield parse_macro(lines[:end + 1])
			lines = lines[end + 1:]
		else:
			lines.pop(0)

def read_files():
	for i in read_file(F01):
		n1, b1, w1, l1 = i
		e1, c1, h1, un1 = l1
		yield n1, b1, w1, e1, c1, h1

def main():
	w = csv.writer(sys.stdout)
	w.writerow(['SDM_name', 'bits', 'write', 'encoding', 'C_name', 'exist'])
	for i in read_files():
		w.writerow(i)

if __name__ == '__main__':
	main()


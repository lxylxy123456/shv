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
	Test running SHV in QEMU
'''

import os
from subprocess import Popen, check_call
from collections import defaultdict
import argparse, subprocess, time, os, re, random, socket, threading

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

println_lock = threading.Lock()

SERIAL_WAITING = 0
SERIAL_PASS = 1
SERIAL_FAIL = 2

def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('--xmhf-img')
	parser.add_argument('--shv-img', required=True)
	parser.add_argument('--qcow2-suffix')
	parser.add_argument('--smp', type=int, default=4)
	parser.add_argument('--work-dir', required=True)
	parser.add_argument('--no-display', action='store_true')
	parser.add_argument('--verbose', action='store_true')
	parser.add_argument('--watch-serial', action='store_true')
	parser.add_argument('--memory', default='1024M')
	parser.add_argument('--qemu-timeout', type=int, default=30)
	args = parser.parse_args()
	return args

def printlog(line):
	with println_lock:
		print(line)

def println(*args):
	with println_lock:
		print('{', *args, '}')

def spawn_qemu(args, serial_file):
	if args.xmhf_img is None:
		img = ['-kernel', args.shv_img]
	else:
		img = ['-d', args.xmhf_img, args.qcow2_suffix,
			   '-d', args.shv_img, args.qcow2_suffix]
	qemu_args = [
		os.path.join(SCRIPT_DIR, 'qemu.sh'), '-m', args.memory, *img,
		'-smp', str(args.smp), '-serial', 'file:%s' % serial_file,
	]
	if args.no_display:
		qemu_args.append('-display')
		qemu_args.append('none')
	popen_stderr = { 'stderr': -1 }
	if args.verbose:
		del popen_stderr['stderr']
		print(' '.join(qemu_args))
	p = Popen(qemu_args, stdin=-1, stdout=-1, **popen_stderr)
	return p

def serial_thread(args, serial_file, serial_result):
	def gen_lines():
		while not os.path.exists(serial_file):
			time.sleep(0.1)
		println('serial_file exists')
		with open(serial_file, 'r') as f:
			while True:
				line = f.readline()
				if line:
					i = line.strip('\n')
					if args.watch_serial:
						printlog(i)
					yield i
				else:
					time.sleep(0.1)
	gen = gen_lines()
#	for i in gen:
#		if 'eXtensible Modular Hypervisor' in i:
#			println('Banner found!')
#			break
#	for i in gen:
#		if 'e820' in i:
#			println('E820 found!')
#			break
	test_count = defaultdict(int)
	for i in gen:
		assert len(test_count) <= args.smp
		if len(test_count) == args.smp and min(test_count.values()) > 20:
			with serial_result[0]:
				serial_result[1] = SERIAL_PASS
				break
		if args.xmhf_img is None:
			fmt = 'CPU\((0x[0-9a-f]+)\): SHV test iter \d+'
		else:
			fmt = 'CPU\((0x[0-9a-f]+)\): SHV in XMHF test iter \d+'
		matched = re.fullmatch(fmt, i)
		if matched:
			test_count[matched.groups()[0]] += 1
			continue
	for i in gen:
		pass

def main():
	args = parse_args()
	serial_file = os.path.join(args.work_dir, 'serial')
	check_call(['rm', '-f', serial_file])
	p = spawn_qemu(args, serial_file)

	try:
		serial_result = [threading.Lock(), SERIAL_WAITING]
		threading.Thread(target=serial_thread,
						args=(args, serial_file, serial_result),
						daemon=True).start()
		for i in range(args.qemu_timeout):
			println('MET = %d' % i)
			with serial_result[0]:
				if serial_result[1] != SERIAL_WAITING:
					break
			time.sleep(1)
	finally:
		p.kill()
		p.wait()

	with serial_result[0]:
		if serial_result[1] == SERIAL_PASS:
			println('TEST PASSED')
			return 0
		elif serial_result[1] == SERIAL_WAITING:
			println('TEST TIME OUT')
			return 1

	println('TEST FAILED')
	return 1

if __name__ == '__main__':
	exit(main())


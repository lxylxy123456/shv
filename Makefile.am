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

# Template from http://www.idryman.org/blog/2016/03/10/autoconf-tutorial-1/

bin_PROGRAMS = shv.bin

# List all source files in src.
# "subdir-objects" in configure.ac allows source file in sub-directories.
shv_bin_SOURCES = \
	src/boot.S \
	src/debug-uart.c \
	src/debug-vga.c \
	src/debug.c \
	src/gdt.c \
	src/hpt.c \
	src/hpto.c \
	src/hptw.c \
	src/idt-asm.S \
	src/idt.c \
	src/kernel.c \
	src/libc_stdio.c \
	src/libc_string.c \
	src/paging.c \
	src/shv-asm.S \
	src/shv-console.c \
	src/shv-ept.c \
	src/shv-global.c \
	src/shv-guest-asm.S \
	src/shv-guest.c \
	src/shv-keyboard.c \
	src/shv-mouse.c \
	src/shv-nmi.c \
	src/shv-pic.c \
	src/shv-timer.c \
	src/shv-user-asm.S \
	src/shv-user.c \
	src/shv-vmcs.c \
	src/shv-vmx.c \
	src/shv.c \
	src/smp-asm.S \
	src/smp.c \
	src/spinlock.c

shv_bin_CPPFLAGS = -I$(top_srcdir)/include/
shv_bin_CCASFLAGS = -D__ASSEMBLY__ \
	-Wall -Werror
shv_bin_CFLAGS = \
	-Wall -Werror -Wno-format -Wno-array-bounds -Wno-address-of-packed-member \
	-fno-stack-protector -fno-mudflap -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
	-mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 \
	-mno-sse4.1 -mno-sse4.2 -mno-sse4 -mno-avx -mno-aes \
	-mno-pclmul -mno-sse4a -mno-3dnow -mno-popcnt -mno-abm

shv_bin_LDFLAGS = -T $(top_srcdir)/linker.ld -ffreestanding -nostdlib -no-pie
shv_bin_LDADD = -lgcc

if I386
shv_bin_CCASFLAGS += -m32
shv_bin_CFLAGS += -m32
else
shv_bin_CCASFLAGS += -m64
shv_bin_CFLAGS += -m64
shv_bin_CCASFLAGS += -fno-pie -fno-pic -mno-red-zone
shv_bin_CFLAGS += -fno-pie -fno-pic -mno-red-zone
endif

all: grub.iso

grub.iso: shv.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp shv.bin isodir/boot/shv.bin
	cp $(top_srcdir)/grub.cfg isodir/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o grub.iso isodir

clean-local: clean-local-grub

clean-local-grub:
	rm -f grub.iso
	rm -rf isodir/


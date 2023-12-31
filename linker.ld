/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Template from https://wiki.osdev.org/Bare_Bones and XMHF runtime.lds.S */

/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

MEMORY
{
	all (rwxai) : ORIGIN = 1M, LENGTH = 4095M
	debug (rwxai) : ORIGIN = 0, LENGTH = 1024M
	unaccounted (rwxai) : ORIGIN = 0, LENGTH = 0 /* see .unaccounted at end */
}

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 1M;

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	/*
	 * Note: we put .text, .rodata, and .data into a single section. When
	 * buildingin 32-bits, this allows the ELF file to be loaded directly into
	 * QEMU.
	 */
	.text :
	{
		_shv_ept_low = .;

		KEEP(*(.multiboot))
		*(.multiboot)

		/* .text */
		*(.text)
		*(.text.*)
		*(.rel.text)
		*(.rel.text.*)

		/* .rodata */
		*(.rodata)
		*(.rodata.*)
		*(.rel.rodata)
		*(.rel.rodata.*)
		*(.comment)
		*(.eh_frame) /* exception-metadata. might be able to discard */
		. = ALIGN(8);
		_begin_xcph_table = .;
		KEEP(*(.xcph_table))
		*(.xcph_table)
		_end_xcph_table = .;

		/* .data */
		*(.data)
		*(.data.*)
		*(.rel.data)
		*(.rel.data.*)
		*(.got)
		*(.got.plt)

		/* unknown */
		*(.note.gnu.build-id)

		/* Mark end of non-zero sections */
		. = ALIGN(16);
		_end_multiboot_data = .;
	}

	/* Read-write data (uninitialized) and stack */
	.bss :
	{
		*(COMMON)
		*(.bss)
		*(SORT_BY_ALIGNMENT(.bss.*))
		*(.rel.bss)
		*(.rel.bss.*)
		. = ALIGN(16);
		_end_multiboot_bss = .;
		. = ALIGN(4096);
		_shv_ept_high = .;
	}

	/* debug sections */
	.debug_abbrev : { *(.debug_abbrev) } >debug
	.debug_aranges : { *(.debug_aranges) } >debug
	.debug_info : { *(.debug_info) } >debug
	.debug_line : { *(.debug_line) } >debug
	.debug_line_str : { *(.debug_line_str) } >debug
	.debug_loc : { *(.debug_loc) } >debug
	.debug_loclists : { *(.debug_loclists) } >debug
	.debug_ranges : { *(.debug_ranges) } >debug
	.debug_rnglists : { *(.debug_rnglists) } >debug
	.debug_str : { *(.debug_str) } >debug
	.rel.debug_info : { *(.rel.debug_info) } >debug
	.rel.debug_aranges : { *(.rel.debug_aranges) } >debug
	.rel.debug_line : { *(.rel.debug_line) } >debug

	/* this is to cause the link to fail if there is
	* anything we didn't explicitly place.
	* when this does cause link to fail, temporarily comment
	* this part out to see what sections end up in the output
	* which are not handled above, and handle them.
	*/
	.unaccounted : {
		*(*)
	} >unaccounted
}

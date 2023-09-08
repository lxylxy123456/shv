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

#include <xmhf.h>
#include <shv.h>

#ifdef __amd64__
.global shv_disable_enable_paging
shv_disable_enable_paging:
	/* Argument %rdi is pointer to string to print */
	movq	%rdi, %rcx

	/* Disable interrupts */
	cli

	/* Jump to compatibility mode */
	pushq	$(__CS32)
	pushq	$1f
	lretq
1:
.code32

	/* Disable paging */
	movl	%cr0, %eax
	andl	$0x7fffffff, %eax
	movl	%eax, %cr0

	/* Get printf lock */
	movl	(emhfc_putchar_linelock_arg), %edx
	movl	$1, %eax
1:	pause
	cmpl	$0, (%edx)
	jne		1b
	lock
	xchg	%eax, (%edx)
	test	%eax, %eax
	jne		1b

	/* Print the string at %ecx */
2:	cmpb	$0, (%ecx)
	je		3f

	/* Use inb to make sure the serial port is ready */
	movw	$(0x3f8 + 5), %dx
1:	inb		(%dx), %al
	testb	$0x20, %al
	jz		1b

	/* Use outb to print to serial port */
	movw	$(0x3f8), %dx
	movb	(%ecx), %al
	outb	%al, (%dx)
	incl	%ecx

	jmp		2b
3:

	/* Release printf lock */
	movl	(emhfc_putchar_linelock_arg), %edx
	movl	$0, %eax
	xchg	%eax, (%edx)

	/* Enable paging */
	movl	%cr0, %eax
	orl		$0x80000000, %eax
	movl	%eax, %cr0

	/* Jump to long mode */
	pushl	$(__CS)
	pushl	$1f
	lretl
1:
.code64

	/* Enable interrupts */
	sti

	/* Done */
	ret
#endif /* __amd64__ */

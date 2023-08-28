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

#ifdef __amd64__

#define PTR		quad
#define SIZE	8
#define PUSHA	PUSHAQ
#define POPA	POPAQ
#define IRET	iretq
#define SP		%rsp
#define AX		%rax

#elif defined(__i386__)

#define PTR		long
#define SIZE	4
#define PUSHA	pushal
#define POPA	popal
#define IRET	iretl
#define SP		%esp
#define AX		%eax

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

idt_stub_common:

	/* Push GPR. */
	PUSHA

	/* Save segment registers. */
	xor		AX, AX
	movw	%gs, %ax
	push	AX
	movw	%fs, %ax
	push	AX
	movw	%es, %ax
	push	AX
	movw	%ds, %ax
	push	AX

	/* Load segment registers. */
	movw	$(__DS), %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	/* Prepare argument. */
#ifdef __amd64__
	movq	%rsp, %rdi
#elif defined(__i386__)
	pushl	%esp
#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

	/* Call C handler. */
	call	handle_idt

	/* Check the return value. From now on, RFLAGS cannot change. */
	test	%eax, %eax

	/* Restore stack. */
#ifdef __i386__
	popl	%eax
#endif /* __i386__ */

	/* Restore segment registers. */
	pop		AX
	movw	%ax, %ds
	pop		AX
	movw	%ax, %es
	pop		AX
	movw	%ax, %fs
	pop		AX
	movw	%ax, %gs

	/* Pop GPR. */
	POPA

	/* Skip error code on the stack. */
	lea		2*SIZE(SP), SP

	/* Jump based on return value of handle_idt(). */
	jne		1f

	/* If handle_idt() returns 0, we perform IRET. */
	IRET

	/*
	 * If handle_idt() returns 1, we simulate IRET using other instructions.
	 * This is prevents NMI unblocking, and is used in NMI tests.
	 *
	 * When simulating IRET, we cannot modify GPR other than ESP. The
	 * modification of ESP depends on the definition of the IRET instruction.
	 * We also need to restore EIP and EFLAGS based on IRET definition.
	 *
	 * The limitation of simulating IRET is that CS, SS cannot be
	 * changed before and after the IRET instruction.
	 */
1:

#ifdef __amd64__

	// TODO: x64 not implemented
	#error 'Not implemented'

#elif defined(__i386__)

	/*
	 * At this point the stack looks like:
	 *  esp+12: esp of interrupted code
	 *  esp+8:  EFLAGS
	 *  esp+4:  CS
	 *  esp+0:  EIP
	 *
	 * We need to modify the stack to look like:
	 *  esp+16: esp of interrupted code
	 *  esp+12: EIP
	 *  esp+8:  EFLAGS
	 *  esp+4:  EAX
	 *  esp+0:  EAX
	 *
	 * Then use pop to restore EAX, use POPF to restore EFLAGS, and use RET to
	 * restore EIP and ESP.
	 */
	pushl   %eax
	movl	12(%esp), %eax
	movl	%eax, 8(%esp)
	movl	4(%esp), %eax
	movl	%eax, 12(%esp)
	movl	(%esp), %eax
	movl	%eax, 4(%esp)
	popl	%eax
	popl	%eax
	popfl
	ret

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

/*
 * Macro for defining IDT stubs.
 * Tutorial in https://sourceware.org/binutils/docs/as/Macro.html .
 */
.macro def_stubs cur=0 name=

.global idt_stub_&name&
idt_stub_&name&:

	/* Push error code. */
	.ifne \cur - 0x08
	.ifne \cur - 0x0a
	.ifne \cur - 0x0b
	.ifne \cur - 0x0c
	.ifne \cur - 0x0d
	.ifne \cur - 0x0e
	.ifne \cur - 0x11
	push	$0
	.endif
	.endif
	.endif
	.endif
	.endif
	.endif
	.endif

	/* Push vector. */
	push	$\cur

	jmp		idt_stub_common

	/* 0 -> 1 -> ... -> 15 */
	.if (\cur + 1) % 16
		def_stubs "(\cur + 1)" &name&1
	.endif

	/* 0 -> 16 -> ... -> 240 */
	.ifeq (\cur % 16)
		.iflt \cur - 240
			def_stubs "(\cur + 16)" &name&h
		.endif
	.endif
.endm

def_stubs

/* Macro for referencing IDT stubs. */
.macro ref_stubs cur=0 name=

	.PTR	idt_stub_&name&

	/* 0 -> 1 -> ... -> 15 */
	.if (\cur + 1) % 16
		ref_stubs "(\cur + 1)" &name&1
	.endif

	/* 0 -> 16 -> ... -> 240 */
	.ifeq (\cur % 16)
		.iflt \cur - 240
			ref_stubs "(\cur + 16)" &name&h
		.endif
	.endif
.endm

.section .rodata
.global g_idt_stubs
g_idt_stubs:
	ref_stubs


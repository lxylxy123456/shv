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
#define DX		%rdx

#elif defined(__i386__)

#define PTR		long
#define SIZE	4
#define PUSHA	pushal
#define POPA	popal
#define IRET	iretl
#define SP		%esp
#define AX		%eax
#define DX		%edx

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

	// TODO: is it possible to do something and let debugger jump to interrupted code?

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
	// TODO: combine similar code

#ifdef __amd64__

	push	AX
	push	DX

	/*
	 * According to SDM, Intel aligns the stack to 16 bytes. So there may be
	 * an extra 8 bytes between SS and interrupted code. We need to read RSP
	 * and move data accordingly.
	 *
	 * At this point the stack looks like:
	 *  rsp+SIZE*8: rsp of interrupted code (case 2)
	 *  rsp+SIZE*7: rsp of interrupted code (case 1)
	 *  rsp+SIZE*6: SS
	 *  rsp+SIZE*5: RSP (points to rsp+SIZE*7 or rsp+SIZE*8)
	 *  rsp+SIZE*4: RFLAGS
	 *  rsp+SIZE*3: CS
	 *  rsp+SIZE*2: RIP
	 *  rsp+SIZE*1: RAX
	 *  rsp+SIZE*0: RDX
	 *
	 * We need to modify the stack to look like:
	 *  rsp+SIZE*4: rsp of interrupted code
	 *  rsp+SIZE*3: RIP
	 *  rsp+SIZE*2: RFLAGS
	 *  rsp+SIZE*1: RAX
	 *  rsp+SIZE*0: RDX
	 *
	 * Then use pop to restore RDX and RAX, use POPF to restore RFLAGS, and use
	 * RET to restore RIP and RSP.
	 */

	mov		SIZE*5(SP), DX
	mov		SIZE*2(SP), AX	/* Move value of RIP around */
	mov		AX, -SIZE*1(DX)
	mov		SIZE*4(SP), AX	/* Move value of RFLAGS around */
	mov		AX, -SIZE*2(DX)
	mov		SIZE*1(SP), AX	/* Move value of RAX around */
	mov		AX, -SIZE*3(DX)
	mov		SIZE*0(SP), AX	/* Move value of RDX around */
	mov		AX, -SIZE*4(DX)
	lea		-SIZE*4(DX), SP	/* Adjust RSP */
	pop		DX				/* Restore value of RDX */
	popq	AX				/* Restore value of RAX */
	popfq					/* Restore value of RFLAGS */
	ret						/* Restore value of RIP and RSP */

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
	 *  esp+12: EIP			was EFLAGS
	 *  esp+8:  EFLAGS		was CS
	 *  esp+4:  EAX			was EIP
	 *  esp+0:  ignored		was EAX
	 *
	 * Then use pop to restore EAX, use POPF to restore EFLAGS, and use RET to
	 * restore EIP and ESP.
	 */
	push	AX
	movl	12(%esp), %eax	/* Move value of EFLAGS around */
	movl	%eax, 8(%esp)
	movl	4(%esp), %eax	/* Move value of EIP around */
	movl	%eax, 12(%esp)
	movl	(%esp), %eax	/* Move value of EAX around */
	movl	%eax, 4(%esp)
	popl	%eax
	popl	%eax			/* Restore value of EAX */
	popfl					/* Restore value of EFLAGS */
	ret						/* Restore value of EIP and ESP */

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


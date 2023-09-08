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
#include <asm_helper.h>

idt_stub_common:

	/* Push GPR. */
	PUSHA

	/* Save segment registers. */
	xor		_AX, _AX
	movw	%gs, %ax
	push	_AX
	movw	%fs, %ax
	push	_AX
	movw	%es, %ax
	push	_AX
	movw	%ds, %ax
	push	_AX

	/* Load segment registers. */
	movw	$(__DS), %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	/*
	 * Prepare arguments. GDB uses the first value above the return address for
	 * handle_idt on stack to construct back trace. We use this feature to
	 * let GDB display the interrupted RIP in back trace.
	 */
	mov		SIZE*(NGPRS+6)(_SP), _DI
	SET_ARG2(_SP)
	SET_ARG1(_DI)
	SET_STACKTOP(_DI)		/* Push interrupted RIP on stack. */

	/* Call C handler. */
	call	handle_idt

	/* Check the return value. From now on, RFLAGS cannot change. */
	test	%eax, %eax

	/* Restore stack. */
	UNSET_ARGS(2, 1)

	/* Restore segment registers. */
	pop		_AX
	movw	%ax, %ds
	pop		_AX
	movw	%ax, %es
	pop		_AX
	movw	%ax, %fs
	pop		_AX
	movw	%ax, %gs

	/* Pop GPR. */
	POPA

	/* Skip error code on the stack. */
	lea		2*SIZE(_SP), _SP

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

	push	_AX
	push	_DX

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

	mov		SIZE*5(_SP), _DX
	mov		SIZE*2(_SP), _AX	/* Move value of RIP around */
	mov		_AX, -SIZE*1(_DX)
	mov		SIZE*4(_SP), _AX	/* Move value of RFLAGS around */
	mov		_AX, -SIZE*2(_DX)
	mov		SIZE*1(_SP), _AX	/* Move value of RAX around */
	mov		_AX, -SIZE*3(_DX)
	mov		SIZE*0(_SP), _AX	/* Move value of RDX around */
	mov		_AX, -SIZE*4(_DX)
	lea		-SIZE*4(_DX), _SP	/* Adjust RSP */
	pop		_DX					/* Restore value of RDX */
	popq	_AX					/* Restore value of RAX */
	popfq						/* Restore value of RFLAGS */
	ret							/* Restore value of RIP and RSP */

#elif defined(__i386__)

	push	_AX

	/*
	 * At this point the stack looks like:
	 *  esp+SIZE*4:  esp of interrupted code
	 *  esp+SIZE*3:  EFLAGS
	 *  esp+SIZE*2:  CS
	 *  esp+SIZE*1:  EIP
	 *  esp+SIZE*0:  EAX
	 *
	 * We need to modify the stack to look like:
	 *  esp+SIZE*3:  esp of interrupted code
	 *  esp+SIZE*2:  EIP		was EFLAGS
	 *  esp+SIZE*1:  EFLAGS		was CS
	 *  esp+SIZE*0:  EAX		was EIP
	 *  esp+SIZE*-1: ignored	was EAX
	 *
	 * Then use pop to restore EAX, use POPF to restore EFLAGS, and use RET to
	 * restore EIP and ESP.
	 */

	mov		SIZE*3(_SP), _AX	/* Move value of EFLAGS around */
	mov		_AX, SIZE*2(_SP)
	mov		SIZE*1(_SP), _AX	/* Move value of EIP around */
	mov		_AX, SIZE*3(_SP)
	mov		SIZE*0(_SP), _AX	/* Move value of EAX around */
	mov		_AX, SIZE*1(_SP)
	pop		_AX
	pop		_AX					/* Restore value of EAX */
	popfl						/* Restore value of EFLAGS */
	ret							/* Restore value of EIP and ESP */

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

/*
 * Macro for defining IDT stubs.
 * Tutorial in https://sourceware.org/binutils/docs/as/Macro.html .
 * mode is either host or guest.
 * cur evaluates to the vector number.
 * name contains 'h' and '1', where each 'h' represents 16 and each '1'
 *  represents 1. Adding all these characters give the vector number. For
 *  example, "hh11" means 16 + 16 + 1 + 1 = 34 = 0x22.
 */
.macro def_stubs mode cur=0 name=

.global idt_stub_&mode&_&name&
idt_stub_&mode&_&name&:

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
	.ifc \mode,guest
	push	$0x100 + \cur
	.else
	push	$\cur
	.endif

	jmp		idt_stub_common

	/* 0 -> 1 -> ... -> 15 */
	.if (\cur + 1) % 16
		def_stubs &mode "(\cur + 1)" &name&1
	.endif

	/* 0 -> 16 -> ... -> 240 */
	.ifeq (\cur % 16)
		.iflt \cur - 240
			def_stubs &mode "(\cur + 16)" &name&h
		.endif
	.endif
.endm

def_stubs host
def_stubs guest

/* Macro for referencing IDT stubs. */
.macro ref_stubs mode cur=0 name=

	._PTR	idt_stub_&mode&_&name&

	/* 0 -> 1 -> ... -> 15 */
	.if (\cur + 1) % 16
		ref_stubs &mode "(\cur + 1)" &name&1
	.endif

	/* 0 -> 16 -> ... -> 240 */
	.ifeq (\cur % 16)
		.iflt \cur - 240
			ref_stubs &mode "(\cur + 16)" &name&h
		.endif
	.endif
.endm

.section .rodata
.global g_idt_stubs_host
g_idt_stubs_host:
	ref_stubs host

.global g_idt_stubs_guest
g_idt_stubs_guest:
	ref_stubs guest

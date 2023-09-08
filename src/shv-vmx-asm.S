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

#define ASM_HALT \
	701119: \
		hlt; \
		jmp 701119b;

#ifdef __amd64__

#define NGPRS	16
#define SIZE	8
#define PUSHA	PUSHAQ
#define POPA	POPAQ
#define PUSHF	pushfq
#define SP		%rsp
#define AX		%rax

/* For convenience, push the first argument to the stack. */
#define SET_ARG1(x)	movq x, %rdi; pushq x;
#define SET_ARG2(x)	movq x, %rsi;
#define SET_ARG3(x)	movq x, %rdx;
#define GET_ARG1(x)	movq %rdi, x;

#elif defined(__i386__)

#define NGPRS	8
#define SIZE	4
#define PUSHA	pushal
#define POPA	popal
#define PUSHF	pushfl
#define SP		%esp
#define AX		%eax

/* Must be called in 32-bit argument push order (arg3, arg2, arg1). */
#define SET_ARG1(x)	pushl x;
#define SET_ARG2(x)	pushl x;
#define SET_ARG3(x)	pushl x;
#define GET_ARG1(x)	movl SIZE(SP), x

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

.global vmexit_asm
vmexit_asm:
	PUSHA

	/*
	 * Optional code to check EFLAGS / RFLAGS.
	 *
	 * Note: PUSHAQ changes RFLAGS, so we do not check status flags in 64-bit.
	 */
	PUSHF
	pop		AX
#ifdef __amd64__
	testq	$~0x8d7, AX
#elif defined(__i386__)
	cmpl	$2, AX
#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */
	je		9f
	ASM_HALT
9:

	/*
	 * Prepare arguments for vmexit_handler(). Place guest_RIP at top of stack
	 * for GDB to construct back trace.
	 */
	mov		SIZE*NGPRS(SP), AX
	SET_ARG3(SP)
	SET_ARG2(AX)
	mov		$0x681e, AX
	vmread	AX, AX
	SET_ARG1(AX)

	call	vmexit_handler
	ASM_HALT			/* Should never return */

.altmacro
.macro vmentry_asm_template inst_name inst_num

inst_name&_asm:
	GET_ARG1(SP)
	POPA
	inst_name
	je		2f				/* ZF set means VMfailValid */
	jb		3f				/* CF set means VMfailInvalid */
	ASM_HALT				/* Unknown error */
2:	SET_ARG2($1)
	jmp		4f
3:	SET_ARG2($0)
4:	SET_ARG1($&inst_num&)	/* vmlaunch = 0, vmresume = 1 */
	call	vmentry_error
	ASM_HALT				/* Should never return */

.endm

.global vmlaunch_asm
vmentry_asm_template vmlaunch 0

.global vmresume_asm
vmentry_asm_template vmresume 1

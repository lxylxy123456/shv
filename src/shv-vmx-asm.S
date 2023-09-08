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
#include <asm_helper.h>

.global vmexit_asm
vmexit_asm:
	PUSHA

	/*
	 * Optional code to check EFLAGS / RFLAGS.
	 *
	 * Note: PUSHAQ changes RFLAGS, so we do not check status flags in 64-bit.
	 */
	PUSHF
	pop		_AX
#ifdef __amd64__
	testq	$~0x8d7, _AX
#elif defined(__i386__)
	cmpl	$2, _AX
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
	mov		SIZE*NGPRS(_SP), _AX
	SET_ARG3(_SP)
	SET_ARG2(_AX)
	mov		$0x681e, _AX
	vmread	_AX, _AX
	SET_ARG1(_AX)

	call	vmexit_handler
	ASM_HALT			/* Should never return */

.altmacro
.macro vmentry_asm_template inst_name inst_num

inst_name&_asm:
	GET_ARG1(_SP)
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

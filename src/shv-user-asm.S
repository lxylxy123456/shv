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

.global enter_user_mode_asm
enter_user_mode_asm:
	/* Prologue */
	push	_BP
	mov		_SP, _BP

	/* Save kernel mode state */
	PUSHA
	PUSHF
	GET_ARG2(_AX)
	mov		_SP, (_AX)

	/* Set user DS and ES */
	movw	$__DS_R3, %ax
	movw	%ax, %ds
	movw	%ax, %es

	/* IRET to user mode */
	GET_ARG1(_SP)
	POPA
	IRET

.global exit_user_mode_asm
exit_user_mode_asm:
	/* Prologue */
	push	_BP
	mov		_SP, _BP

	/* Restore kernel mode state */
	GET_ARG1(_SP)
	POPF
	POPA
	movw	$__DS, %ax
	movw	%ax, %ss

	/* Epilogue */
	pop		_BP
	ret

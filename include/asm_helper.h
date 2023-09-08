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

#ifdef __amd64__
#define _CASE_BITSIZE(_32, _64) _64
#elif defined(__i386__)
#define _CASE_BITSIZE(_32, _64) _32
#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

/* Assembly version of HALT(). */
#define ASM_HALT \
	701119: \
		hlt; \
		jmp 701119b;

#define _PTR	_CASE_BITSIZE(long, quad)
#define NGPRS	_CASE_BITSIZE(8, 16)
#define SIZE	_CASE_BITSIZE(4, 8)

#define PUSHA	_CASE_BITSIZE(pushal, PUSHAQ)
#define POPA	_CASE_BITSIZE(popal, POPAQ)
#define PUSHF	_CASE_BITSIZE(pushfl, pushfq)
#define IRET	_CASE_BITSIZE(iretl, iretq)

#define _AX		_CASE_BITSIZE(%eax, %rax)
#define _CX		_CASE_BITSIZE(%ecx, %rcx)
#define _DX		_CASE_BITSIZE(%edx, %rdx)
#define _BX		_CASE_BITSIZE(%ebx, %rbx)
#define _SP		_CASE_BITSIZE(%esp, %rsp)
#define _BP		_CASE_BITSIZE(%ebp, %rbp)
#define _SI		_CASE_BITSIZE(%esi, %rsi)
#define _DI		_CASE_BITSIZE(%edi, %rdi)

#ifdef __amd64__

/* Define macros to interact with C calling convention. */
#define SET_STACKTOP(x)	pushq x;
#define SET_ARG1(x)		movq x, %rdi;
#define SET_ARG2(x)		movq x, %rsi;
#define SET_ARG3(x)		movq x, %rdx;
#define UNSET_ARGS(a,s)	leaq SIZE*(s)(%rsp), %rsp;
#define GET_ARG1(x)		movq %rdi, x;

#elif defined(__i386__)

/* Must be called in 32-bit argument push order (arg3, arg2, arg1). */
#define SET_STACKTOP(x)	;
#define SET_ARG1(x)		pushl x;
#define SET_ARG2(x)		pushl x;
#define SET_ARG3(x)		pushl x;
#define UNSET_ARGS(a,s)	leal SIZE*(a)(%esp), %esp;
#define GET_ARG1(x)		movl SIZE(_SP), x

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

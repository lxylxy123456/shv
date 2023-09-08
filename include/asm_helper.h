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

// TODO: SP -> _SP
#define SP		%rsp
#define AX		%rax
#define DI		%rdi

/* For convenience, push the first argument to the stack. */
// TODO: do not push the first argument to the stack by default.
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

// TODO: SP -> _SP
#define SP		%esp
#define AX		%eax
#define DI		%edi

/* Must be called in 32-bit argument push order (arg3, arg2, arg1). */
#define SET_ARG1(x)	pushl x;
#define SET_ARG2(x)	pushl x;
#define SET_ARG3(x)	pushl x;
#define GET_ARG1(x)	movl SIZE(SP), x

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */


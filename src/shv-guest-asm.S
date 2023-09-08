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

.global shv_guest_entry

#define ASM_HALT \
	701119: \
		hlt; \
		jmp 701119b;

#ifdef __amd64__

#define DI		%rdi
#define SET_ARG1(x)	movq x, %rdi;

#elif defined(__i386__)

#define DI		%edi
#define SET_ARG1(x)	pushl x;

#else /* !defined(__i386__) && !defined(__amd64__) */
	#error "Unsupported Arch"
#endif /* __amd64__ */

shv_guest_entry:
	SET_ARG1(DI)
	call	shv_guest_main
	ASM_HALT		/* Should never return */

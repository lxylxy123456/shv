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

/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

// init low-level support routines
// author: amit vasudevan (amitvasudevan@acm.org)

#include <xmhf.h>
#include <asm_helper.h>

//---AP boot-strap code---------------------------------------------------------
.section .text
.code16
.global _ap_bootstrap_start
_ap_bootstrap_start:
	jmp ap_bootstrap_bypassdata
_ap_gdtdesc:
	.word _ap_gdt_end - _ap_gdt_start - 1
	.long _ap_gdt_start - _ap_bootstrap_start + 0x10000
	.align 16
_ap_gdt_start:
	.quad 0x0000000000000000
	.quad 0x00cf9a000000ffff	/* __CS32 */
	.quad 0x00cf92000000ffff	/* __DS */
	.quad 0x00af9a000000ffff	/* __CS64 */
	.quad 0x0000000000000000	/* __TRSEL */
	.quad 0x0000000000000000
	.quad 0x0000000000000000	/* __CS_R3 */
	.quad 0x0000000000000000	/* __DS_R3 */
_ap_gdt_end:
	.word 0
ap_bootstrap_bypassdata:
	movw	$0x1000, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	$0xFFFF, %sp
	movw	$0x4000, %ax
	movw	%ax, %ss

	movw	$(_ap_gdtdesc - _ap_bootstrap_start), %si

	lgdt	(%si)

	movl	%cr0, %eax
	orl		$0x1, %eax
	movl	%eax, %cr0

	jmpl	$0x08, $(_ap_clear_pipe - _ap_bootstrap_start + (AP_BOOTSTRAP_CODE_SEG << 4))
.code32
_ap_clear_pipe:
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	movl	$init_core_lowlevel_setup_32, %eax
	jmpl	*%eax
	hlt

.global _ap_bootstrap_end
_ap_bootstrap_end:
	nop
	nop
	nop
	nop

//---init_core_lowlevel_setup---------------------------------------------------

init_core_lowlevel_setup_32:

#ifdef __amd64__
	/* Set CR3 and CR4 (copy from BSP's value) */
	movl	(g_cr3), %eax
	movl	%eax, %cr3
	movl	(g_cr4), %eax
	movl	%eax, %cr4

	/* Set EFER MSR LME */
	movl	$(MSR_EFER), %ecx
	rdmsr
	btsl	$(EFER_LME), %eax
	wrmsr

	/* Set stack to perform lret */
	movl	$(g_smp_lret_stack), %esp

	/* set CR0 */
	movl	%cr0, %eax
	orl		$(CR0_PG), %eax
	andl	$(~(CR0_NW | CR0_CD)), %eax
	movl	%eax, %cr0

	/* jump to 64-bit mode */
	lret

.code64
#endif /* __amd64__ */

.global init_core_lowlevel_setup
init_core_lowlevel_setup:

	//load our gdt
	lgdt	init_gdt

	//get hold of local APIC id
	movl	$(MSR_APIC_BASE), %ecx
	rdmsr
	andl	$0xFFFFF000, %eax
	addl	$0x20, %eax
	movl	(_AX), %eax
	shrl	$24, %eax

	movl	g_midtable_numentries, %edx

	//get vcpu virtual address of this CPU/core
	mov		$(g_midtable), _BX
	xorl	%ecx, %ecx
getvcpuloop:
#ifdef __i386__
	movl	0x0(_BX, _CX, 8), %ebp  //ebp contains the lapic id
#elif defined(__amd64__)
	lea		0x0(_BX, _CX, 8), _SP
	movl	0x0(_SP, _CX, 8), %ebp  //ebp contains the lapic id
#else
	#error "Unsupported Arch"
#endif /* __i386__ */
	cmpl	%eax, %ebp
	jz		gotvcpu
	incl	%ecx
	cmpl	%edx, %ecx
	jb		getvcpuloop
	//we should never get here, if so just halt
	hlt
gotvcpu:
#ifdef __i386__
	mov		0x4(_BX, _CX, 8), _DI //edi contains vcpu pointer
#elif defined(__amd64__)
	mov		0x8(_SP, _CX, 8), _DI //rdi contains vcpu pointer
#else
	#error "Unsupported Arch"
#endif /* __i386__ */
	mov		0x0(_DI), _SP	//load stack for this CPU
	push	_DI
	call	kernel_main_smp
	//we should never get here, if so just halt
	hlt



//------------------------------------------------------------------------------
.section .data

	//the GDT
init_gdt:
	.word init_gdt_end - init_gdt_start - 1
	.long init_gdt_start

	.align 16
init_gdt_start:
	.quad 0x0000000000000000
	.quad 0x00cf9a000000ffff	/* __CS32 */
	.quad 0x00cf92000000ffff	/* __DS */
	.quad 0x00af9a000000ffff	/* __CS64 */
	.quad 0x0000000000000000	/* __TRSEL */
	.quad 0x0000000000000000
	.quad 0x0000000000000000	/* __CS_R3 */
	.quad 0x0000000000000000	/* __DS_R3 */
init_gdt_end:

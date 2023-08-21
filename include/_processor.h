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

//processor.h - CPU declarations
//author: amit vasudevan (amitvasudevan@acm.org)
#ifndef __PROCESSOR_H
#define __PROCESSOR_H

#define AMD_STRING_DWORD1 0x68747541
#define AMD_STRING_DWORD2 0x69746E65
#define AMD_STRING_DWORD3 0x444D4163

#define INTEL_STRING_DWORD1	0x756E6547
#define INTEL_STRING_DWORD2	0x49656E69
#define INTEL_STRING_DWORD3	0x6C65746E

#define EFLAGS_CF	0x00000001	/* Carry Flag */
#define EFLAGS_PF	0x00000004	/* Parity Flag */
#define EFLAGS_AF	0x00000010	/* Auxillary carry Flag */
#define EFLAGS_ZF	0x00000040	/* Zero Flag */
#define EFLAGS_SF	0x00000080	/* Sign Flag */
#define EFLAGS_TF	0x00000100	/* Trap Flag */
#define EFLAGS_IF	0x00000200	/* Interrupt Flag */
#define EFLAGS_DF	0x00000400	/* Direction Flag */
#define EFLAGS_OF	0x00000800	/* Overflow Flag */
#define EFLAGS_IOPL	0x00003000	/* IOPL mask */
#define EFLAGS_NT	0x00004000	/* Nested Task */
#define EFLAGS_RF	0x00010000	/* Resume Flag */
#define EFLAGS_VM	0x00020000	/* Virtual Mode */
#define EFLAGS_AC	0x00040000	/* Alignment Check */
#define EFLAGS_VIF	0x00080000	/* Virtual Interrupt Flag */
#define EFLAGS_VIP	0x00100000	/* Virtual Interrupt Pending */
#define EFLAGS_ID	0x00200000	/* CPUID detection flag */

#define CR0_PE		0x00000001	/* Enable Protected Mode    (RW) */
#define CR0_MP		0x00000002	/* Monitor Coprocessor      (RW) */
#define CR0_EM		0x00000004	/* Require FPU Emulation    (RO) */
#define CR0_TS		0x00000008	/* Task Switched            (RW) */
#define CR0_ET		0x00000010	/* Extension type           (RO) */
#define CR0_NE		0x00000020	/* Numeric Error Reporting  (RW) */
#define CR0_WP		0x00010000	/* Supervisor Write Protect (RW) */
#define CR0_AM		0x00040000	/* Alignment Checking       (RW) */
#define CR0_NW		0x20000000	/* Not Write-Through        (RW) */
#define CR0_CD		0x40000000	/* Cache Disable            (RW) */
#define CR0_PG		0x80000000	/* Paging                   (RW) */

#define CR4_VME			0x00000001	/* enable vm86 extensions */
#define CR4_PVI			0x00000002	/* virtual interrupts flag enable */
#define CR4_TSD			0x00000004	/* disable time stamp at ipl 3 */
#define CR4_DE			0x00000008	/* enable debugging extensions */
#define CR4_PSE			0x00000010	/* enable page size extensions */
#define CR4_PAE			0x00000020	/* enable physical address extensions */
#define CR4_MCE			0x00000040	/* Machine check enable */
#define CR4_PGE			0x00000080	/* enable global pages */
#define CR4_PCE			0x00000100	/* enable performance counters at ipl 3 */
#define CR4_OSFXSR		0x00000200	/* enable fast FPU save and restore */
#define CR4_OSXMMEXCPT	0x00000400	/* enable unmasked SSE exceptions */
#define CR4_UMIP		0x00000800	/* enable user-mode instruction prevention */
#define CR4_LA57		0x00001000	/* enable 5-level paging */
#define CR4_VMXE		0x00002000	/* enable VMX */
#define CR4_SMXE		0x00004000	/* enable SMX */
#define CR4_FSGSBASE	0x00010000	/* FSGSBASE-Enable Bit */
#define CR4_PCIDE		0x00020000	/* PCID-Enable Bit */
#define CR4_OSXSAVE		0x00040000	/* XSAVE and Processor Extended States-Enable bit */
#define CR4_KL			0x00080000	/* Key-Locker-Enable Bit */
#define CR4_SMEP		0x00100000	/* SMEP-Enable Bit */
#define CR4_SMAP		0x00200000	/* SMAP-Enable Bit */
#define CR4_PKE			0x00400000	/* Enable protection keys for user-mode pages */
#define CR4_CET			0x00800000	/* Control-flow Enforcement Technology */
#define CR4_PKS			0x01000000	/* Enable protection keys for supervisor-mode pages */

//CPUID related
#define EDX_PAE 6
#define EDX_NX 20
#define ECX_SVM 2
#define EDX_NP 0

#define SVM_CPUID_FEATURE       (1 << 2)

#define CPUID_X86_FEATURE_VMX    (1<<5)
#define CPUID_X86_FEATURE_SMX    (1<<6)

//CPU exception numbers
//(intel SDM vol 3a 6-27)
#define	CPU_EXCEPTION_DE				0	//divide error exception
#define CPU_EXCEPTION_DB				1	//debug exception
#define	CPU_EXCEPTION_NMI				2	//non-maskable interrupt
#define	CPU_EXCEPTION_BP				3	//breakpoint exception
#define	CPU_EXCEPTION_OF				4	//overflow exception
#define	CPU_EXCEPTION_BR				5	//bound-range exceeded
#define	CPU_EXCEPTION_UD				6	//invalid opcode
#define	CPU_EXCEPTION_NM				7	//device not available
#define	CPU_EXCEPTION_DF				8	//double fault exception (code)
#define	CPU_EXCEPTION_RESV9				9	//reserved
#define	CPU_EXCEPTION_TS				10	//invalid TSS (code)
#define	CPU_EXCEPTION_NP				11	//segment not present (code)
#define	CPU_EXCEPTION_SS				12	//stack fault (code)
#define	CPU_EXCEPTION_GP				13	//general protection (code)
#define CPU_EXCEPTION_PF				14	//page fault (code)
#define	CPU_EXCEPTION_RESV15			15	//reserved
#define CPU_EXCEPTION_MF				16	//floating-point exception
#define CPU_EXCEPTION_AC				17	//alignment check (code)
#define CPU_EXCEPTION_MC				18	//machine check
#define CPU_EXCEPTION_XM				19	//SIMD floating point exception

//extended control registers
#define XCR_XFEATURE_ENABLED_MASK       0x00000000

#if defined(__ASSEMBLY__) && defined(__amd64__)

/*
 * Intel hardware supports PUSHAL in 32-bits, but not a similar instruction
 * in 64-bits. Here we define an assembly macro to do this. Both PUSHAL and
 * PUSHAQ follow struct regs.
 *
 * Note: this macro uses the ADD instruction, so it changes status flags in
 * EFLAGS, such as EFLAGS.PF.
 */
#define PUSHAQ \
        pushq   %rax; \
        pushq   %rcx; \
        pushq   %rdx; \
        pushq   %rbx; \
        pushq   %rsp; \
        pushq   %rbp; \
        pushq   %rsi; \
        pushq   %rdi; \
        pushq   %r8; \
        pushq   %r9; \
        pushq   %r10; \
        pushq   %r11; \
        pushq   %r12; \
        pushq   %r13; \
        pushq   %r14; \
        pushq   %r15; \
        addq	$(4*8), 11*8(%rsp);

/*
 * Intel hardware supports POPAL in 32-bits, but not a similar instruction
 * in 64-bits. Here we define an assembly macro to do this. Both POPAL and
 * POPAQ follow struct regs.
 */
#define POPAQ \
        popq    %r15; \
        popq    %r14; \
        popq    %r13; \
        popq    %r12; \
        popq    %r11; \
        popq    %r10; \
        popq    %r9; \
        popq    %r8; \
        popq    %rdi; \
        popq    %rsi; \
        popq    %rbp; \
        leaq    8(%rsp), %rsp; \
        popq    %rbx; \
        popq    %rdx; \
        popq    %rcx; \
        popq    %rax;

#endif							/* defined(__ASSEMBLY__) && defined(__amd64__) */

#ifndef __ASSEMBLY__

// i386: follow the order enforced by PUSHAD/POPAD
// amd64: manually follow this order in assembly code
struct regs {
#ifdef __amd64__
	u64 r15;
	u64 r14;
	u64 r13;
	u64 r12;
	u64 r11;
	u64 r10;
	u64 r9;
	u64 r8;
#endif							/* __amd64__ */
	union {
#ifdef __amd64__
		u64 rdi;
#endif							/* __amd64__ */
		uintptr_t di;
		u32 edi;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rsi;
#endif							/* __amd64__ */
		uintptr_t si;
		u32 esi;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rbp;
#endif							/* __amd64__ */
		uintptr_t bp;
		u32 ebp;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rsp;
#endif							/* __amd64__ */
		uintptr_t sp;
		u32 esp;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rbx;
#endif							/* __amd64__ */
		uintptr_t bx;
		u32 ebx;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rdx;
#endif							/* __amd64__ */
		uintptr_t dx;
		u32 edx;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rcx;
#endif							/* __amd64__ */
		uintptr_t cx;
		u32 ecx;
	} __attribute__((packed));
	union {
#ifdef __amd64__
		u64 rax;
#endif							/* __amd64__ */
		uintptr_t ax;
		u32 eax;
	} __attribute__((packed));
} __attribute__((packed));

typedef struct {
	u16 isrLow16;
	u16 isrSelector;
	u8 count;
	u8 type;
	u16 isrHigh16;
#ifdef __amd64__
	u32 isrHigh32;
	u32 reserved_zero;
#elif !defined(__i386__)
#error "Unsupported Arch"
#endif							/* !defined(__i386__) */
} __attribute__((packed)) idtentry_t;

typedef struct {
	u16 limit0_15;
	u16 baseAddr0_15;
	u8 baseAddr16_23;
	u8 attributes1;
	u8 limit16_19attributes2;
	u8 baseAddr24_31;
#ifdef __amd64__
	u32 baseAddr32_63;
	u32 reserved_zero;
#elif !defined(__i386__)
#error "Unsupported Arch"
#endif							/* !defined(__i386__) */
} __attribute__((packed)) TSSENTRY;

#endif							//__ASSEMBLY__

#endif							/* __PROCESSOR_H */

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

#ifndef _XMHF_H_
#define _XMHF_H_

#include <_msr.h>

#ifndef __ASSEMBLY__

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include <shv_types.h>

#endif							/* !__ASSEMBLY__ */

#include <_processor.h>
#include <_paging.h>

#ifndef __ASSEMBLY__

#include <cpu.h>

#define __DEBUG_QEMU__

#define ASSERT(expr) \
	do { \
		if (!(expr)) { \
			printf("Error: ASSERT(%s) @ %s:%d failed\n", #expr, __FILE__, \
				   __LINE__); \
			cpu_halt(); \
		} \
	} while (0)

#include <_vmx.h>
#include <_vmx_ctls.h>
#include <_acpi.h>
#include <hptw.h>

#include <smp.h>

/* spinlock.c */
typedef volatile uint32_t spin_lock_t;
extern void spin_lock(spin_lock_t * lock);
extern void spin_unlock(spin_lock_t * lock);

/* debug.c */
extern void *emhfc_putchar_arg;
extern spin_lock_t *emhfc_putchar_linelock_arg;
extern void emhfc_putchar(int c, void *arg);
extern void emhfc_putchar_linelock(spin_lock_t * arg);
extern void emhfc_putchar_lineunlock(spin_lock_t * arg);

#define HALT cpu_halt

typedef u64 spa_t;
typedef uintptr_t hva_t;

static inline spa_t hva2spa(void *hva)
{
	return (spa_t) (uintptr_t) hva;
}

static inline void *spa2hva(spa_t spa)
{
	return (void *)(uintptr_t) spa;
}

#define MAX_VCPU_ENTRIES 10
#define MAX_PCPU_ENTRIES (MAX_VCPU_ENTRIES)

typedef struct {
	u32 vmexit_reason;
	ulong_t guest_rip;
	u32 inst_len;
} vmexit_info_t;

#define NUM_FIXED_MTRRS 11
#define MAX_VARIABLE_MTRR_PAIRS 10

#define INDEX_IA32_VMX_BASIC_MSR                0x0
#define INDEX_IA32_VMX_PINBASED_CTLS_MSR        0x1
#define INDEX_IA32_VMX_PROCBASED_CTLS_MSR       0x2
#define INDEX_IA32_VMX_EXIT_CTLS_MSR            0x3
#define INDEX_IA32_VMX_ENTRY_CTLS_MSR           0x4
#define INDEX_IA32_VMX_MISC_MSR                 0x5
#define INDEX_IA32_VMX_CR0_FIXED0_MSR           0x6
#define INDEX_IA32_VMX_CR0_FIXED1_MSR           0x7
#define INDEX_IA32_VMX_CR4_FIXED0_MSR           0x8
#define INDEX_IA32_VMX_CR4_FIXED1_MSR           0x9
#define INDEX_IA32_VMX_VMCS_ENUM_MSR            0xA
#define INDEX_IA32_VMX_PROCBASED_CTLS2_MSR      0xB
#define INDEX_IA32_VMX_EPT_VPID_CAP_MSR         0xC
#define INDEX_IA32_VMX_TRUE_PINBASED_CTLS_MSR   0xD
#define INDEX_IA32_VMX_TRUE_PROCBASED_CTLS_MSR  0xE
#define INDEX_IA32_VMX_TRUE_EXIT_CTLS_MSR       0xF
#define INDEX_IA32_VMX_TRUE_ENTRY_CTLS_MSR      0x10
#define INDEX_IA32_VMX_VMFUNC_MSR               0x11
// IA32_VMX_MSRCOUNT should be 18, but Bochs does not support the last one
#define IA32_VMX_MSRCOUNT                       17

struct _guestvarmtrrmsrpair {
	u64 base;					/* IA32_MTRR_PHYSBASEi */
	u64 mask;					/* IA32_MTRR_PHYSMASKi */
};

struct _guestmtrrmsrs {
	u64 def_type;				/* IA32_MTRR_DEF_TYPE */
	u64 fix_mtrrs[NUM_FIXED_MTRRS];	/* IA32_MTRR_FIX*, see fixed_mtrr_prop */
	u32 var_count;				/* Number of valid var_mtrrs's */
	struct _guestvarmtrrmsrpair var_mtrrs[MAX_VARIABLE_MTRR_PAIRS];
};

typedef struct _vcpu {
	uintptr_t sp;
	u32 id;
	u32 idx;
	bool isbsp;

	u64 vmx_msrs[IA32_VMX_MSRCOUNT];
	u64 vmx_pinbased_ctls;		//IA32_VMX_PINBASED_CTLS or IA32_VMX_TRUE_...
	u64 vmx_procbased_ctls;		//IA32_VMX_PROCBASED_CTLS or IA32_VMX_TRUE_...
	u64 vmx_exit_ctls;			//IA32_VMX_EXIT_CTLS or IA32_VMX_TRUE_...
	u64 vmx_entry_ctls;			//IA32_VMX_ENTRY_CTLS or IA32_VMX_TRUE_...
	vmx_ctls_t vmx_caps;

	u8 vmx_ept_defaulttype;
	bool vmx_ept_fixmtrr_support;
	bool vmx_ept_mtrr_enable;
	bool vmx_ept_fixmtrr_enable;
	u64 vmx_ept_paddrmask;
	struct _guestmtrrmsrs vmx_guestmtrrmsrs;
	struct _vmx_vmcsfields vmcs;

	int shv_lapic_x[2];
	int shv_pit_x[2];
	u64 lapic_time;
	u64 pit_time;

	void *vmxon_region;
	void *my_vmcs;
	void *my_stack;
	msr_entry_t *my_vmexit_msrstore;
	msr_entry_t *my_vmexit_msrload;
	msr_entry_t *my_vmentry_msrload;
	u32 ept_exit_count;
	u8 ept_num;
	void (*volatile vmexit_handler_override)(struct _vcpu *, struct regs *,
											 vmexit_info_t *);
} VCPU;

#define SHV_STACK_SIZE (65536)

/* shv-global.c */
extern u32 g_midtable_numentries;
extern PCPU g_cpumap[MAX_PCPU_ENTRIES];
extern MIDTAB g_midtable[MAX_VCPU_ENTRIES];
extern VCPU g_vcpus[MAX_VCPU_ENTRIES];
extern u8 g_cpu_stack[MAX_VCPU_ENTRIES][SHV_STACK_SIZE];
extern uintptr_t g_cr3;
extern uintptr_t g_cr4;
#ifdef __amd64__
extern u32 g_smp_lret_stack[2];
#endif							/* __amd64__ */

/* paging.c */
extern uintptr_t shv_page_table_init(void);
#ifdef __amd64__
extern volatile u64 shv_pml4t[P4L_NPLM4T * P4L_NEPT] ALIGNED_PAGE;
extern volatile u64 shv_pdpt[P4L_NPDPT * P4L_NEPT] ALIGNED_PAGE;
extern volatile u64 shv_pdt[P4L_NPDT * P4L_NEPT] ALIGNED_PAGE;
#elif defined(__i386__)
#if I386_PAE
extern volatile u64 shv_pdpt[PAE_NPDPTE] __attribute__((aligned(32)));
extern volatile u64 shv_pdt[PAE_NPDT * PAE_NEPT] ALIGNED_PAGE;
#else							/* !I386_PAE */
extern volatile u32 shv_pdt[P32_NPDT * P32_NEPT] ALIGNED_PAGE;
#endif							/* I386_PAE */
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */

/* idt.c */
#define IDT_NELEMS 256
typedef struct {
	uintptr_t ds;
	uintptr_t es;
	uintptr_t fs;
	uintptr_t gs;
	struct regs r;
	uintptr_t vector;
	uintptr_t error_code;
	uintptr_t ip;
	uintptr_t cs;
	uintptr_t flags;
#ifdef __amd64__
	uintptr_t sp;
	uintptr_t ss;
#endif							/* __amd64__ */
} __attribute__((packed)) iret_info_t;
extern uintptr_t g_idt_stubs_host[IDT_NELEMS];
extern uintptr_t g_idt_stubs_guest[IDT_NELEMS];
extern uintptr_t g_idt_host[IDT_NELEMS][2];
extern uintptr_t g_idt_guest[IDT_NELEMS][2];
extern void init_idt(void);
extern VCPU *get_vcpu(void);
extern void dump_exception(VCPU * vcpu, struct regs *r, iret_info_t * info);
extern u32 handle_idt(uintptr_t _ip, iret_info_t * info);

/* gdt.c */
#define GDT_NELEMS 10
extern u64 g_gdt[MAX_VCPU_ENTRIES][GDT_NELEMS];
extern u8 g_tss[MAX_VCPU_ENTRIES][PAGE_SIZE_4K] ALIGNED_PAGE;
extern void init_gdt(VCPU * vcpu);

#endif							/* !__ASSEMBLY__ */

#define __CS32	0x08			/* CS (code segment selector) */
#define __DS	0x10			/* DS (data segment selector) */
#define __CS64	0x18			/* 64-bit CS, only used in amd64 */
#define __TRSEL	0x20			/* TSS selector (also occupies 0x28 in amd64) */
#define __CS_R3	0x33			/* CS for user mode */
#define __DS_R3 0x3b			/* DS for user mode */

#ifdef __amd64__
#define __CS	__CS64
#elif defined(__i386__)
#define __CS	__CS32
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */

#define AP_BOOTSTRAP_CODE_SEG 			0x1000

#endif							/* _XMHF_H_ */

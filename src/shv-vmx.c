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
#include <limits.h>

#define MAX_GUESTS 4
#define MAX_MSR_LS 16			/* Max number of MSRs in MSR load / store */

static u8 all_vmxon_region[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
 ALIGNED_PAGE;

static u8 all_vmcs[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
 ALIGNED_PAGE;

static u8 all_guest_stack[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
 ALIGNED_PAGE;

static msr_entry_t
	vmexit_msrstore_entries[MAX_VCPU_ENTRIES][MAX_GUESTS][MAX_MSR_LS]
	__attribute__((aligned(16)));
static msr_entry_t
	vmexit_msrload_entries[MAX_VCPU_ENTRIES][MAX_GUESTS][MAX_MSR_LS]
	__attribute__((aligned(16)));
static msr_entry_t
	vmentry_msrload_entries[MAX_VCPU_ENTRIES][MAX_GUESTS][MAX_MSR_LS]
	__attribute__((aligned(16)));

extern u64 x_gdt_start[MAX_VCPU_ENTRIES][GDT_NELEMS];

static void shv_vmx_vmcs_init(VCPU * vcpu)
{
	// From vmx_initunrestrictedguestVMCS
	__vmx_vmwriteNW(VMCS_host_CR0, read_cr0());
	__vmx_vmwriteNW(VMCS_host_CR4, read_cr4());
	__vmx_vmwriteNW(VMCS_host_CR3, read_cr3());
	__vmx_vmwrite16(VMCS_host_CS_selector, read_cs());
	__vmx_vmwrite16(VMCS_host_DS_selector, read_ds());
	__vmx_vmwrite16(VMCS_host_ES_selector, read_es());
	__vmx_vmwrite16(VMCS_host_FS_selector, read_fs());
	__vmx_vmwrite16(VMCS_host_GS_selector, read_gs());
	__vmx_vmwrite16(VMCS_host_SS_selector, read_ss());
	__vmx_vmwrite16(VMCS_host_TR_selector, read_tr());
	__vmx_vmwriteNW(VMCS_host_GDTR_base, (uintptr_t) g_gdt[vcpu->idx]);
	__vmx_vmwriteNW(VMCS_host_IDTR_base, (uintptr_t) g_idt_host);
	__vmx_vmwriteNW(VMCS_host_TR_base, (u64) (hva_t) g_tss[vcpu->idx]);
	__vmx_vmwriteNW(VMCS_host_RIP, (u64) (hva_t) vmexit_asm);

	//store vcpu at TOS
	vcpu->sp = vcpu->sp - sizeof(hva_t);
	*(hva_t *) vcpu->sp = (hva_t) vcpu;
	__vmx_vmwriteNW(VMCS_host_RSP, (u64) vcpu->sp);

	__vmx_vmwrite32(VMCS_host_SYSENTER_CS, rdmsr64(IA32_SYSENTER_CS_MSR));
	__vmx_vmwriteNW(VMCS_host_SYSENTER_ESP, rdmsr64(IA32_SYSENTER_ESP_MSR));
	__vmx_vmwriteNW(VMCS_host_SYSENTER_EIP, rdmsr64(IA32_SYSENTER_EIP_MSR));
	__vmx_vmwriteNW(VMCS_host_FS_base, rdmsr64(IA32_MSR_FS_BASE));
	__vmx_vmwriteNW(VMCS_host_GS_base, rdmsr64(IA32_MSR_GS_BASE));

	//setup default VMX controls
	__vmx_vmwrite32(VMCS_control_VMX_pin_based,
					vcpu->vmx_msrs[INDEX_IA32_VMX_PINBASED_CTLS_MSR]);
	//activate secondary processor controls
	{
		u32 proc_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS_MSR];
		proc_ctls |= (1U << VMX_PROCBASED_ACTIVATE_SECONDARY_CONTROLS);
		proc_ctls &= ~(1U << VMX_PROCBASED_CR3_LOAD_EXITING);
		proc_ctls &= ~(1U << VMX_PROCBASED_CR3_STORE_EXITING);
		__vmx_vmwrite32(VMCS_control_VMX_cpu_based, proc_ctls);
	}
	{
		u32 vmexit_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR];
#ifdef __amd64__
		vmexit_ctls |= (1U << VMX_VMEXIT_HOST_ADDRESS_SPACE_SIZE);
#elif !defined(__i386__)
#error "Unsupported Arch"
#endif							/* !defined(__i386__) */
		__vmx_vmwrite32(VMCS_control_VM_exit_controls, vmexit_ctls);
	}
	{
		u32 vmentry_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_ENTRY_CTLS_MSR];
#ifdef __amd64__
		vmentry_ctls |= (1U << VMX_VMENTRY_IA_32E_MODE_GUEST);
#elif !defined(__i386__)
#error "Unsupported Arch"
#endif							/* !defined(__i386__) */
		__vmx_vmwrite32(VMCS_control_VM_entry_controls, vmentry_ctls);
	}
	__vmx_vmwrite32(VMCS_control_VMX_seccpu_based,
					vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS2_MSR]);

	//Critical MSR load/store
	if (g_shv_opt & SHV_USE_MSR_LOAD) {
		vcpu->my_vmexit_msrstore = vmexit_msrstore_entries[vcpu->idx][0];
		vcpu->my_vmexit_msrload = vmexit_msrload_entries[vcpu->idx][0];
		vcpu->my_vmentry_msrload = vmentry_msrload_entries[vcpu->idx][0];
		__vmx_vmwrite64(VMCS_control_VM_exit_MSR_store_address,
						hva2spa(vcpu->my_vmexit_msrstore));
		__vmx_vmwrite64(VMCS_control_VM_exit_MSR_load_address,
						hva2spa(vcpu->my_vmexit_msrload));
		__vmx_vmwrite64(VMCS_control_VM_entry_MSR_load_address,
						hva2spa(vcpu->my_vmentry_msrload));
		printf("CPU(0x%02x): configured load/store MSR\n", vcpu->id);
	}
	__vmx_vmwrite32(VMCS_control_VM_exit_MSR_load_count, 0);
	__vmx_vmwrite32(VMCS_control_VM_entry_MSR_load_count, 0);
	__vmx_vmwrite32(VMCS_control_VM_exit_MSR_store_count, 0);

	if (g_shv_opt & SHV_USE_EPT) {
		u64 eptp;
		u32 seccpu;
		shv_ept_init(vcpu);
		eptp = shv_build_ept(vcpu, 0);
		seccpu = __vmx_vmread32(VMCS_control_VMX_seccpu_based);
		seccpu |= (1U << VMX_SECPROCBASED_ENABLE_EPT);
		__vmx_vmwrite32(VMCS_control_VMX_seccpu_based, seccpu);
		__vmx_vmwrite64(VMCS_control_EPT_pointer, eptp | 0x1eULL);
#ifdef __i386__
#if I386_PAE
		/* For old SHV code, which uses PAE paging. SHV uses 32-bit paging. */
		{
			u64 *cr3 = (u64 *) read_cr3();
			__vmx_vmwrite64(VMCS_guest_PDPTE0, cr3[0]);
			__vmx_vmwrite64(VMCS_guest_PDPTE1, cr3[1]);
			__vmx_vmwrite64(VMCS_guest_PDPTE2, cr3[2]);
			__vmx_vmwrite64(VMCS_guest_PDPTE3, cr3[3]);
		}
#endif							/* I386_PAE */
#endif							/* __i386__ */
	}

	if (g_shv_opt & SHV_USE_VPID) {
		u32 seccpu = __vmx_vmread32(VMCS_control_VMX_seccpu_based);
		seccpu |= (1U << VMX_SECPROCBASED_ENABLE_VPID);
		__vmx_vmwrite32(VMCS_control_VMX_seccpu_based, seccpu);
		__vmx_vmwrite16(VMCS_control_vpid, 1);
	}

	if (g_shv_opt & SHV_USE_UNRESTRICTED_GUEST) {
		u32 seccpu;
		ASSERT(g_shv_opt & SHV_USE_EPT);
		seccpu = __vmx_vmread32(VMCS_control_VMX_seccpu_based);
		seccpu |= (1U << VMX_SECPROCBASED_UNRESTRICTED_GUEST);
		__vmx_vmwrite32(VMCS_control_VMX_seccpu_based, seccpu);
	}

	__vmx_vmwrite32(VMCS_control_pagefault_errorcode_mask, 0);
	__vmx_vmwrite32(VMCS_control_pagefault_errorcode_match, 0);
	__vmx_vmwrite32(VMCS_control_exception_bitmap, 0);
	__vmx_vmwrite32(VMCS_control_CR3_target_count, 0);

	//setup guest state
	//CR0, real-mode, PE and PG bits cleared, set ET bit
	{
		ulong_t cr0 = vcpu->vmx_msrs[INDEX_IA32_VMX_CR0_FIXED0_MSR];
		cr0 |= CR0_ET;
		__vmx_vmwriteNW(VMCS_guest_CR0, cr0);
	}
	//CR4, required bits set (usually VMX enabled bit)
	{
		ulong_t cr4 = vcpu->vmx_msrs[INDEX_IA32_VMX_CR4_FIXED0_MSR];
#ifdef __amd64__
		cr4 |= CR4_PAE;
#else							/* __i386__ */
#if I386_PAE
		cr4 |= CR4_PAE;
#else							/* !I386_PAE */
		cr4 |= CR4_PSE;
#endif							/* I386_PAE */
#endif							/* __amd64__ */
		__vmx_vmwriteNW(VMCS_guest_CR4, cr4);
	}
	//CR3 set to 0, does not matter
	__vmx_vmwriteNW(VMCS_guest_CR3, read_cr3());
	//IDTR
	__vmx_vmwriteNW(VMCS_guest_IDTR_base, (uintptr_t) g_idt_guest);
	__vmx_vmwrite32(VMCS_guest_IDTR_limit, 0xffff);
	//GDTR
	__vmx_vmwriteNW(VMCS_guest_GDTR_base, (uintptr_t) g_gdt[vcpu->idx]);
	__vmx_vmwrite32(VMCS_guest_GDTR_limit, 0xffff);
	//LDTR, unusable
	__vmx_vmwriteNW(VMCS_guest_LDTR_base, 0);
	__vmx_vmwrite32(VMCS_guest_LDTR_limit, 0x0);
	__vmx_vmwrite16(VMCS_guest_LDTR_selector, 0);
	__vmx_vmwrite32(VMCS_guest_LDTR_access_rights, 0x10000);
	// TR, should be usable for VMX to work, but not used by guest
	__vmx_vmwriteNW(VMCS_guest_TR_base, (u64) (hva_t) g_tss[vcpu->idx]);
	__vmx_vmwrite32(VMCS_guest_TR_limit, 0x67);
	__vmx_vmwrite16(VMCS_guest_TR_selector, __TRSEL);
	__vmx_vmwrite32(VMCS_guest_TR_access_rights, 0x8b);
	//DR7
	__vmx_vmwriteNW(VMCS_guest_DR7, 0x400);
	//RSP
	{
		vcpu->my_stack = &all_guest_stack[vcpu->idx][0][PAGE_SIZE_4K];
		__vmx_vmwriteNW(VMCS_guest_RSP, (u64) (ulong_t) vcpu->my_stack);
	}
	//RIP
	__vmx_vmwrite16(VMCS_guest_CS_selector, __CS);
	__vmx_vmwriteNW(VMCS_guest_CS_base, 0);
	__vmx_vmwriteNW(VMCS_guest_RIP, (u64) (ulong_t) shv_guest_entry);
	__vmx_vmwriteNW(VMCS_guest_RFLAGS, (1 << 1));
	//CS, DS, ES, FS, GS and SS segments
	__vmx_vmwrite32(VMCS_guest_CS_limit, 0xffffffff);
#ifdef __amd64__
	__vmx_vmwrite32(VMCS_guest_CS_access_rights, 0xa09b);
#elif defined(__i386__)
	__vmx_vmwrite32(VMCS_guest_CS_access_rights, 0xc09b);
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
	__vmx_vmwrite16(VMCS_guest_DS_selector, __DS);
	__vmx_vmwriteNW(VMCS_guest_DS_base, 0);
	__vmx_vmwrite32(VMCS_guest_DS_limit, 0xffffffff);
	__vmx_vmwrite32(VMCS_guest_DS_access_rights, 0xc093);
	__vmx_vmwrite16(VMCS_guest_ES_selector, __DS);
	__vmx_vmwriteNW(VMCS_guest_ES_base, 0);
	__vmx_vmwrite32(VMCS_guest_ES_limit, 0xffffffff);
	__vmx_vmwrite32(VMCS_guest_ES_access_rights, 0xc093);
	__vmx_vmwrite16(VMCS_guest_FS_selector, __DS);
	__vmx_vmwriteNW(VMCS_guest_FS_base, 0);
	__vmx_vmwrite32(VMCS_guest_FS_limit, 0xffffffff);
	__vmx_vmwrite32(VMCS_guest_FS_access_rights, 0xc093);
	__vmx_vmwrite16(VMCS_guest_GS_selector, __DS);
	__vmx_vmwriteNW(VMCS_guest_GS_base, 0);
	__vmx_vmwrite32(VMCS_guest_GS_limit, 0xffffffff);
	__vmx_vmwrite32(VMCS_guest_GS_access_rights, 0xc093);
	__vmx_vmwrite16(VMCS_guest_SS_selector, __DS);
	__vmx_vmwriteNW(VMCS_guest_SS_base, 0);
	__vmx_vmwrite32(VMCS_guest_SS_limit, 0xffffffff);
	__vmx_vmwrite32(VMCS_guest_SS_access_rights, 0xc093);

	//setup VMCS link pointer
	__vmx_vmwrite64(VMCS_guest_VMCS_link_pointer, (u64) 0xFFFFFFFFFFFFFFFFULL);

	//trap access to CR0 fixed 1-bits
	{
		ulong_t cr0_mask = vcpu->vmx_msrs[INDEX_IA32_VMX_CR0_FIXED0_MSR];
		cr0_mask &= ~(CR0_PE);
		cr0_mask &= ~(CR0_PG);
		cr0_mask |= CR0_CD;
		cr0_mask |= CR0_NW;
		__vmx_vmwriteNW(VMCS_control_CR0_mask, cr0_mask);
	}
	__vmx_vmwriteNW(VMCS_control_CR0_shadow, CR0_ET);

	//trap access to CR4 fixed bits (this includes the VMXE bit)
	__vmx_vmwriteNW(VMCS_control_CR4_mask,
					vcpu->vmx_msrs[INDEX_IA32_VMX_CR4_FIXED0_MSR]);
	__vmx_vmwriteNW(VMCS_control_CR0_shadow, 0);
}

void shv_vmx_main(VCPU * vcpu)
{
	u32 vmcs_revision_identifier;

	/* Make sure this is Intel CPU */
	{
		u32 eax, ebx, ecx, edx;
		cpuid(0, &eax, &ebx, &ecx, &edx);
		ASSERT(ebx == INTEL_STRING_DWORD1);
		ASSERT(ecx == INTEL_STRING_DWORD3);
		ASSERT(edx == INTEL_STRING_DWORD2);
	}

	/* Save contents of MSRs (from _vmx_initVT) */
	for (u32 i = 0; i < IA32_VMX_MSRCOUNT; i++) {
		vcpu->vmx_msrs[i] = rdmsr64(IA32_VMX_BASIC_MSR + i);
	}

	/* Initialize vcpu->vmx_caps */
	if (vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR] & (1ULL << 55)) {
		vcpu->vmx_pinbased_ctls =
			vcpu->vmx_msrs[INDEX_IA32_VMX_TRUE_PINBASED_CTLS_MSR];
		vcpu->vmx_procbased_ctls =
			vcpu->vmx_msrs[INDEX_IA32_VMX_TRUE_PROCBASED_CTLS_MSR];
		vcpu->vmx_exit_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_TRUE_EXIT_CTLS_MSR];
		vcpu->vmx_entry_ctls =
			vcpu->vmx_msrs[INDEX_IA32_VMX_TRUE_ENTRY_CTLS_MSR];
	} else {
		vcpu->vmx_pinbased_ctls =
			vcpu->vmx_msrs[INDEX_IA32_VMX_PINBASED_CTLS_MSR];
		vcpu->vmx_procbased_ctls =
			vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS_MSR];
		vcpu->vmx_exit_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR];
		vcpu->vmx_entry_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_ENTRY_CTLS_MSR];
	}
	vcpu->vmx_caps.pinbased_ctls = (vcpu->vmx_pinbased_ctls >> 32);
	vcpu->vmx_caps.procbased_ctls = (vcpu->vmx_procbased_ctls >> 32);
	vcpu->vmx_caps.procbased_ctls2 =
		(vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS2_MSR] >> 32);
	vcpu->vmx_caps.exit_ctls = (vcpu->vmx_exit_ctls >> 32);
	vcpu->vmx_caps.entry_ctls = (vcpu->vmx_entry_ctls >> 32);

	/* Discover support for VMX (22.6 DISCOVERING SUPPORT FOR VMX) */
	{
		u32 eax, ebx, ecx, edx;
		cpuid(1, &eax, &ebx, &ecx, &edx);
		ASSERT(ecx & (1U << 5));
	}

	/* Allocate VM (23.11.5 VMXON Region) */
	{
		u64 basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
		vmcs_revision_identifier = (u32) basic_msr & 0x7fffffffU;
		vcpu->vmxon_region = (void *)all_vmxon_region[vcpu->idx];
		*((u32 *) vcpu->vmxon_region) = vmcs_revision_identifier;
	}

	write_cr0(read_cr0() | CR0_NE);

	/* Set CR4.VMXE (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		ulong_t cr4 = read_cr4();
		ASSERT((cr4 & CR4_VMXE) == 0);
		write_cr4(cr4 | CR4_VMXE);
	}

	/* Check IA32_FEATURE_CONTROL (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		u64 vmx_msr_efcr = rdmsr64(MSR_EFCR);
		// printf("rdmsr64(MSR_EFCR) = 0x%016x\n", vmx_msr_efcr);
		ASSERT(vmx_msr_efcr & 1);
		ASSERT(vmx_msr_efcr & 4);
	}

	/* VMXON */
	{
		ASSERT(__vmx_vmxon(hva2spa(vcpu->vmxon_region)));
	}

	/* VMCLEAR, VMPTRLD */
	{
		vcpu->my_vmcs = all_vmcs[vcpu->idx][0];
		if (!"test_vmclear" && vcpu->isbsp) {
			for (u32 i = 0; i < 0x1000 / sizeof(u32); i++) {
				((u32 *) vcpu->my_vmcs)[i] = (i << 20) | i;
			}
		}
		ASSERT(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		*((u32 *) vcpu->my_vmcs) = vmcs_revision_identifier;
		ASSERT(__vmx_vmptrld(hva2spa(vcpu->my_vmcs)));
		if (!"test_vmclear" && vcpu->isbsp) {
			for (u32 i = 0; i < 0x1000 / sizeof(u32); i++) {
				printf("vmcs[0x%03x] = 0x%08x\n", i,
					   ((u32 *) vcpu->my_vmcs)[i]);
			}
			vmcs_print_all(vcpu);
		}
	}

	/* Modify VMCS */
	shv_vmx_vmcs_init(vcpu);
	vmcs_dump(vcpu, 0);

	/* VMLAUNCH */
	{
		struct regs r;
		memset(&r, 0, sizeof(r));
		r.di = (uintptr_t) vcpu;
		vmlaunch_asm(&r);
	}

	ASSERT(0 && "vmlaunch_asm() should never return");
}

void vmexit_handler(ulong_t guest_rip, VCPU * vcpu, struct regs *r)
{
	u32 vmexit_reason = __vmx_vmread32(VMCS_info_vmexit_reason);
	u32 inst_len = __vmx_vmread32(VMCS_info_vmexit_instruction_length);
	ASSERT(guest_rip == __vmx_vmreadNW(VMCS_guest_RIP));

	if (vcpu->vmexit_handler_override) {
		vmexit_info_t vmexit_info = {
			.vmexit_reason = vmexit_reason,
			.guest_rip = guest_rip,
			.inst_len = inst_len,
		};
		vcpu->vmexit_handler_override(vcpu, r, &vmexit_info);
	}

	ASSERT(vcpu == get_vcpu());
	switch (vmexit_reason) {
	case VMX_VMEXIT_CPUID:
		{
			u32 old_eax = r->eax;
			cpuid_raw(&r->eax, &r->ebx, &r->ecx, &r->edx);
			if (old_eax == 0x1) {
				/* Clear VMX capability */
				r->ecx &= ~(1U << 5);
			}
			__vmx_vmwriteNW(VMCS_guest_RIP, guest_rip + inst_len);
			break;
		}
	case VMX_VMEXIT_RDMSR:
		{
			rdmsr(r->ecx, &r->eax, &r->edx);
			__vmx_vmwriteNW(VMCS_guest_RIP, guest_rip + inst_len);
			break;
		}
	case VMX_VMEXIT_EPT_VIOLATION:
		ASSERT(g_shv_opt & SHV_USE_EPT);
		{
			ulong_t q = __vmx_vmreadNW(VMCS_info_exit_qualification);
			u64 paddr = __vmx_vmread64(VMCS_guest_paddr);
			ulong_t vaddr = __vmx_vmreadNW(VMCS_info_guest_linear_address);
			/* Unknown EPT violation */
			printf("CPU(0x%02x): ept: 0x%08lx\n", vcpu->id, q);
			printf("CPU(0x%02x): paddr: 0x%016llx\n", vcpu->id, paddr);
			printf("CPU(0x%02x): vaddr: 0x%08lx\n", vcpu->id, vaddr);
			vmcs_dump(vcpu, 0);
			ASSERT(0 && "Unknown EPT violation");
			break;
		}
	case VMX_VMEXIT_VMCALL:
		printf("CPU(0x%02x): unknown vmcall\n", vcpu->id);
		/* fallthrough */
	default:
		{
			printf("CPU(0x%02x): unknown vmexit %u\n", vcpu->id, vmexit_reason);
			printf("CPU(0x%02x): rip = 0x%x\n", vcpu->id, guest_rip);
			vmcs_dump(vcpu, 0);
			ASSERT(0 && "Unknown VMEXIT");
			break;
		}
	}
	vmresume_asm(r);
}

void vmentry_error(ulong_t is_resume, ulong_t valid)
{
	VCPU *vcpu = get_vcpu();
	/* 29.4 VM INSTRUCTION ERROR NUMBERS */
	ulong_t vminstr_error = __vmx_vmread32(VMCS_info_vminstr_error);
	printf("CPU(0x%02x): is_resume = %ld, valid = %ld, err = %ld\n",
		   vcpu->id, is_resume, valid, vminstr_error);
	ASSERT(is_resume && valid && 0);
	ASSERT(0);
}

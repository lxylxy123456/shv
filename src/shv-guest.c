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

/*
 * From peh-x86-safemsr.c
 * Perform RDMSR instruction.
 * If successful, return 0. If RDMSR causes #GP, return 1.
 * Implementation similar to Linux's native_read_msr_safe().
 */
static u32 _rdmsr_safe(u32 index, u64 * value)
{
	u32 result;
	u32 eax, edx;
	asm volatile ("1:\r\n"
				  "rdmsr\r\n"
				  "xor %%ebx, %%ebx\r\n"
				  "jmp 3f\r\n"
				  "2:\r\n"
				  "movl $1, %%ebx\r\n" "jmp 3f\r\n" ".section .xcph_table\r\n"
#ifdef __amd64__
				  ".quad 0xd\r\n" ".quad 1b\r\n" ".quad 2b\r\n"
#elif defined(__i386__)
				  ".long 0xd\r\n" ".long 1b\r\n" ".long 2b\r\n"
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
				  ".previous\r\n" "3:\r\n":"=a" (eax), "=d"(edx), "=b"(result)
				  :"c"(index));
	if (result == 0) {
		*value = ((u64) edx << 32) | eax;
	}
	return result;
}

/*
 * Perform WRMSR instruction.
 * If successful, return 0. If WRMSR causes #GP, return 1.
 */
static u32 _wrmsr_safe(u32 index, u64 value)
{
	u32 result;
	u32 eax = value, edx = value >> 32;
	asm volatile ("1:\r\n"
				  "wrmsr\r\n"
				  "xor %%ebx, %%ebx\r\n"
				  "jmp 3f\r\n"
				  "2:\r\n"
				  "movl $1, %%ebx\r\n" "jmp 3f\r\n" ".section .xcph_table\r\n"
#ifdef __amd64__
				  ".quad 0xd\r\n" ".quad 1b\r\n" ".quad 2b\r\n"
#elif defined(__i386__)
				  ".long 0xd\r\n" ".long 1b\r\n" ".long 2b\r\n"
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
				  ".previous\r\n" "3:\r\n":"=b" (result)
				  :"c"(index), "a"(eax), "d"(edx));
	return result;
}

/* Return whether MSR is writable with a wild value. */
static bool msr_writable(u32 index)
{
	u64 old_val;
	u64 new_val;
	if (_rdmsr_safe(index, &old_val)) {
		return false;
	}
	new_val = old_val ^ 0x1234abcdbeef6789ULL;
	if (_wrmsr_safe(index, new_val)) {
		return false;
	}
	ASSERT(_wrmsr_safe(index, old_val) == 0);
	return true;
}

/* Test whether MSR load / store during VMEXIT / VMENTRY are effective */
typedef struct shv_guest_test_msr_ls_data {
	bool skip_mc_msrs;
	u32 mc_msrs[3];
	u64 old_vals[3][3];
} shv_guest_test_msr_ls_data_t;

static void shv_guest_test_msr_ls_vmexit_handler(VCPU * vcpu, struct regs *r,
												 vmexit_info_t * info)
{
	shv_guest_test_msr_ls_data_t *data;
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	data = (void *)r->bx;
	switch (r->eax) {
	case 12:
		/*
		 * Check support of machine check MSRs.
		 * On QEMU, 0x402 and 0x403 are writable.
		 * On HP 2540P, 0x400 is writable.
		 */
		if ((rdmsr64(0x179U) & 0xff) > 1 && msr_writable(0x402)) {
			data->skip_mc_msrs = false;
			ASSERT(msr_writable(0x403));
			ASSERT(msr_writable(0x406));
			data->mc_msrs[0] = 0x402U;
			data->mc_msrs[1] = 0x403U;
			data->mc_msrs[2] = 0x406U;
		} else {
			data->skip_mc_msrs = true;
		}
		/* Save old MSRs */
		data->old_vals[0][0] = rdmsr64(0x20aU);
		data->old_vals[0][1] = rdmsr64(0x20bU);
		data->old_vals[0][2] = rdmsr64(0x20cU);
		data->old_vals[1][0] = rdmsr64(MSR_EFER);
		data->old_vals[1][1] = rdmsr64(0xc0000081U);
		data->old_vals[1][2] = rdmsr64(MSR_IA32_PAT);
		if (!data->skip_mc_msrs) {
			data->old_vals[2][0] = rdmsr64(data->mc_msrs[0]);
			data->old_vals[2][1] = rdmsr64(data->mc_msrs[1]);
			data->old_vals[2][2] = rdmsr64(data->mc_msrs[2]);
		}
		/* Set experiment */
		if (!data->skip_mc_msrs) {
			__vmx_vmwrite32(VMCS_control_VM_exit_MSR_store_count, 3);
			__vmx_vmwrite32(VMCS_control_VM_exit_MSR_load_count, 3);
			__vmx_vmwrite32(VMCS_control_VM_entry_MSR_load_count, 3);
		} else {
			__vmx_vmwrite32(VMCS_control_VM_exit_MSR_store_count, 2);
			__vmx_vmwrite32(VMCS_control_VM_exit_MSR_load_count, 2);
			__vmx_vmwrite32(VMCS_control_VM_entry_MSR_load_count, 2);
		}
		ASSERT((rdmsr64(0xfeU) & 0xff) > 6);
		wrmsr64(0x20aU, 0x00000000aaaaa000ULL);
		wrmsr64(0x20bU, 0x00000000deadb000ULL);
		wrmsr64(0x20cU, 0x00000000deadc000ULL);
		if (!data->skip_mc_msrs) {
			wrmsr64(data->mc_msrs[0], 0x2222222222222222ULL);
			wrmsr64(data->mc_msrs[1], 0xdeaddeadbeefbeefULL);
			wrmsr64(data->mc_msrs[2], 0xdeaddeadbeefbeefULL);
		}
		ASSERT(rdmsr64(MSR_IA32_PAT) == 0x0007040600070406ULL);
		vcpu->my_vmexit_msrstore[0].index = 0x20aU;
		vcpu->my_vmexit_msrstore[0].data = 0x00000000deada000ULL;
		vcpu->my_vmexit_msrstore[1].index = MSR_EFER;
		vcpu->my_vmexit_msrstore[1].data = 0x00000000deada000ULL;
		if (!data->skip_mc_msrs) {
			vcpu->my_vmexit_msrstore[2].index = data->mc_msrs[0];
			vcpu->my_vmexit_msrstore[2].data = 0xdeaddeadbeefbeefULL;
		}
		vcpu->my_vmexit_msrload[0].index = 0x20bU;
		vcpu->my_vmexit_msrload[0].data = 0x00000000bbbbb000ULL;
		vcpu->my_vmexit_msrload[1].index = 0xc0000081U;
		vcpu->my_vmexit_msrload[1].data = 0x0000000011111000ULL;
		if (!data->skip_mc_msrs) {
			vcpu->my_vmexit_msrload[2].index = data->mc_msrs[1];
			vcpu->my_vmexit_msrload[2].data = 0x3333333333333333ULL;
		}
		vcpu->my_vmentry_msrload[0].index = 0x20cU;
		vcpu->my_vmentry_msrload[0].data = 0x00000000ccccc000ULL;
		vcpu->my_vmentry_msrload[1].index = MSR_IA32_PAT;
		vcpu->my_vmentry_msrload[1].data = 0x0007060400070604ULL;
		if (!data->skip_mc_msrs) {
			vcpu->my_vmentry_msrload[2].index = data->mc_msrs[2];
			vcpu->my_vmentry_msrload[2].data = 0x6666666666666666ULL;
		}
		break;
	case 16:
		/* Check effects */
		ASSERT(vcpu->my_vmexit_msrstore[0].data == 0x00000000aaaaa000ULL);
		ASSERT(rdmsr64(0x20bU) == 0x00000000bbbbb000ULL);
		ASSERT(rdmsr64(0x20cU) == 0x00000000ccccc000ULL);
		ASSERT(vcpu->my_vmexit_msrstore[1].data == rdmsr64(MSR_EFER));
		ASSERT(rdmsr64(MSR_IA32_PAT) == 0x0007060400070604ULL);
		ASSERT(rdmsr64(0xc0000081U) == 0x0000000011111000ULL);
		if (!data->skip_mc_msrs) {
			ASSERT(vcpu->my_vmexit_msrstore[2].data == 0x2222222222222222ULL);
			ASSERT(rdmsr64(data->mc_msrs[1]) == 0x3333333333333333ULL);
			ASSERT(rdmsr64(data->mc_msrs[2]) == 0x6666666666666666ULL);
		}
		/* Reset state */
		__vmx_vmwrite32(VMCS_control_VM_exit_MSR_store_count, 0);
		__vmx_vmwrite32(VMCS_control_VM_exit_MSR_load_count, 0);
		__vmx_vmwrite32(VMCS_control_VM_entry_MSR_load_count, 0);
		wrmsr64(MSR_IA32_PAT, 0x0007040600070406ULL);
		wrmsr64(0x20aU, data->old_vals[0][0]);
		wrmsr64(0x20bU, data->old_vals[0][1]);
		wrmsr64(0x20cU, data->old_vals[0][2]);
		wrmsr64(MSR_EFER, data->old_vals[1][0]);
		wrmsr64(0xc0000081U, data->old_vals[1][1]);
		wrmsr64(MSR_IA32_PAT, data->old_vals[1][2]);
		if (!data->skip_mc_msrs) {
			wrmsr64(data->mc_msrs[0], data->old_vals[2][0]);
			wrmsr64(data->mc_msrs[1], data->old_vals[2][1]);
			wrmsr64(data->mc_msrs[2], data->old_vals[2][2]);
		}
		break;
	default:
		ASSERT(0 && "Unknown argument");
		break;
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void shv_guest_test_msr_ls(VCPU * vcpu)
{
	if (SHV_OPT & SHV_USE_MSR_LOAD) {
		shv_guest_test_msr_ls_data_t data;
		vcpu->vmexit_handler_override = shv_guest_test_msr_ls_vmexit_handler;
		asm volatile ("vmcall"::"a" (12), "b"(&data));
		asm volatile ("vmcall"::"a" (16), "b"(&data));
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Test whether EPT VMEXITs happen as expected */
static void shv_guest_test_ept_vmexit_handler(VCPU * vcpu, struct regs *r,
											  vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_EPT_VIOLATION) {
		return;
	}
	{
		ulong_t q = __vmx_vmreadNW(VMCS_info_exit_qualification);
		u64 paddr = __vmx_vmread64(VMCS_guest_paddr);
		ulong_t vaddr = __vmx_vmreadNW(VMCS_info_guest_linear_address);
		if (paddr != 0x12340000 || vaddr != 0x12340000) {
			/* Let the default handler report the error */
			return;
		}
		/* On older machines: q = 0x181; on Dell 7050: q = 0x581 */
		ASSERT((q & ~0xe00UL) == 0x181UL);
		ASSERT(r->eax == 0xdeadbeefU);
		ASSERT(r->ebx == 0x12340000U);
		r->eax = 0xfee1c0de;
		vcpu->ept_exit_count++;
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void shv_guest_test_ept(VCPU * vcpu)
{
	if (SHV_OPT & SHV_USE_EPT) {
		u32 expected_ept_count;
		ASSERT(vcpu->ept_exit_count == 0);
		vcpu->vmexit_handler_override = shv_guest_test_ept_vmexit_handler;
		{
			u32 a = 0xdeadbeef;
			u32 *p = (u32 *) 0x12340000;
			asm volatile ("movl (%1), %%eax":"+a" (a):"b"(p):"cc", "memory");
			if (0) {
				printf("CPU(0x%02x): EPT result: 0x%08x 0x%02x\n", vcpu->id, a,
					   vcpu->ept_num);
			}
			if (vcpu->ept_num == 0) {
				ASSERT(a == 0xfee1c0de);
				expected_ept_count = 1;
			} else {
				ASSERT((u8) a == vcpu->ept_num);
				ASSERT((u8) (a >> 8) == vcpu->ept_num);
				ASSERT((u8) (a >> 16) == vcpu->ept_num);
				ASSERT((u8) (a >> 24) == vcpu->ept_num);
				expected_ept_count = 0;
			}
		}
		vcpu->vmexit_handler_override = NULL;
		ASSERT(vcpu->ept_exit_count == expected_ept_count);
		vcpu->ept_exit_count = 0;
	}
}

/* Switch EPT */
static void shv_guest_switch_ept_vmexit_handler(VCPU * vcpu, struct regs *r,
												vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	ASSERT(r->eax == 17);
	{
		u64 eptp;
		/* Check prerequisite */
		ASSERT(SHV_OPT & SHV_USE_EPT);
		/* Swap EPT */
		vcpu->ept_num++;
		vcpu->ept_num %= (SHV_EPT_COUNT << 4);
		eptp = shv_build_ept(vcpu, vcpu->ept_num);
		__vmx_vmwrite64(VMCS_control_EPT_pointer, eptp | 0x1eULL);
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void shv_guest_switch_ept(VCPU * vcpu)
{
	if (SHV_OPT & SHV_USE_SWITCH_EPT) {
		ASSERT(SHV_OPT & SHV_USE_EPT);
		vcpu->vmexit_handler_override = shv_guest_switch_ept_vmexit_handler;
		asm volatile ("vmcall"::"a" (17));
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Test VMCLEAR and VMXOFF */
static void shv_guest_test_vmxoff_vmexit_handler(VCPU * vcpu, struct regs *r,
												 vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	ASSERT(r->eax == 22);
	{
		bool test_vmxoff = r->ebx;
		const bool test_modify_vmcs = true;
		spa_t vmptr;
		u32 old_es_limit;
		u32 new_es_limit = 0xa69f1c74;
		/* Back up current VMCS */
		vmcs_dump(vcpu, 0);
		/* Set ES access right */
		if (test_modify_vmcs) {
			old_es_limit = __vmx_vmread32(VMCS_guest_ES_limit);
			__vmx_vmwrite32(VMCS_guest_ES_limit, new_es_limit);
		}
		/* Test VMPTRST */
		ASSERT(__vmx_vmptrst(&vmptr));
		ASSERT(vmptr == hva2spa(vcpu->my_vmcs));

		/* VMCLEAR current VMCS */
		ASSERT(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		/* Make sure that VMWRITE fails */
		ASSERT(!__vmx_vmwrite(0x0000, 0x0000));

		/* Run VMXOFF and VMXON */
		if (test_vmxoff) {
			u32 result;
			ASSERT(__vmx_vmxoff());
			asm volatile ("1:\r\n"
						  "vmwrite %2, %1\r\n"
						  "xor %%ebx, %%ebx\r\n"
						  "jmp 3f\r\n"
						  "2:\r\n"
						  "movl $1, %%ebx\r\n"
						  "jmp 3f\r\n" ".section .xcph_table\r\n"
#ifdef __amd64__
						  ".quad 0x6\r\n" ".quad 1b\r\n" ".quad 2b\r\n"
#elif defined(__i386__)
						  ".long 0x6\r\n" ".long 1b\r\n" ".long 2b\r\n"
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
						  ".previous\r\n" "3:\r\n":"=b" (result)
						  :"r"(0UL), "rm"(0UL));

			/* Make sure that VMWRITE raises #UD exception */
			ASSERT(result == 1);

			ASSERT(__vmx_vmxon(hva2spa(vcpu->vmxon_region)));
		}

		/*
		 * Find ES access right in VMCS and correct it. Here we need to assume
		 * that the hardware stores the VMCS field directly in a 4 byte aligned
		 * location (i.e. no encoding, no encryption etc).
		 */
		if (test_modify_vmcs) {
			u32 i;
			u32 found = 0;
			for (i = 2; i < PAGE_SIZE_4K / sizeof(new_es_limit); i++) {
				if (((u32 *) vcpu->my_vmcs)[i] == new_es_limit) {
					((u32 *) vcpu->my_vmcs)[i] = old_es_limit;
					found++;
				}
			}
			ASSERT(found == 1);
		}

		/* Make sure that VMWRITE still fails */
		ASSERT(!__vmx_vmwrite(0x0000, 0x0000));

		/* Restore VMCS using VMCLEAR and VMPTRLD */
		ASSERT(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		{
			u64 basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
			u32 vmcs_revision_identifier = basic_msr & 0x7fffffffU;
			ASSERT(*((u32 *) vcpu->my_vmcs) == vmcs_revision_identifier);
		}
		ASSERT(__vmx_vmptrld(hva2spa(vcpu->my_vmcs)));
		/* Make sure all VMCS fields stay the same */
		{
			struct _vmx_vmcsfields a;
			memcpy(&a, &vcpu->vmcs, sizeof(a));
			vmcs_dump(vcpu, 0);
			ASSERT(memcmp(&a, &vcpu->vmcs, sizeof(a)) == 0);
		}
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	/* Hardware thinks VMCS is not launched, so VMLAUNCH instead of VMRESUME */
	vmlaunch_asm(r);
}

static void shv_guest_test_vmxoff(VCPU * vcpu, bool test_vmxoff)
{
	if (SHV_OPT & SHV_USE_VMXOFF) {
		vcpu->vmexit_handler_override = shv_guest_test_vmxoff_vmexit_handler;
		asm volatile ("vmcall"::"a" (22), "b"((u32) test_vmxoff));
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Test changing VPID and whether INVVPID returns the correct error code */
static void shv_guest_test_vpid_vmexit_handler(VCPU * vcpu, struct regs *r,
											   vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	ASSERT(r->eax == 19);
	{
		/* VPID will always be odd */
		u16 vpid = __vmx_vmread16(VMCS_control_vpid);
		ASSERT(vpid % 2 == 1);
		/*
		 * Currently we cannot easily test the effect of INVVPID. So
		 * just make sure that the return value is correct.
		 */
		ASSERT(__vmx_invvpid(VMX_INVVPID_INDIVIDUALADDRESS, vpid, 0x12345678U));
#ifdef __amd64__
		ASSERT(!__vmx_invvpid(VMX_INVVPID_INDIVIDUALADDRESS, vpid,
							  0xf0f0f0f0f0f0f0f0ULL));
#elif !defined(__i386__)
#error "Unsupported Arch"
#endif							/* !defined(__i386__) */
		ASSERT(!__vmx_invvpid(VMX_INVVPID_INDIVIDUALADDRESS, 0, 0x12345678U));
		ASSERT(__vmx_invvpid(VMX_INVVPID_SINGLECONTEXT, vpid, 0));
		ASSERT(!__vmx_invvpid(VMX_INVVPID_SINGLECONTEXT, 0, 0));
		ASSERT(__vmx_invvpid(VMX_INVVPID_ALLCONTEXTS, vpid, 0));
		ASSERT(__vmx_invvpid(VMX_INVVPID_SINGLECONTEXTGLOBAL, vpid, 0));
		ASSERT(!__vmx_invvpid(VMX_INVVPID_SINGLECONTEXTGLOBAL, 0, 0));
		/* Update VPID */
		vpid += 2;
		__vmx_vmwrite16(VMCS_control_vpid, vpid);
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void shv_guest_test_vpid(VCPU * vcpu)
{
	if (SHV_OPT & SHV_USE_VPID) {
		vcpu->vmexit_handler_override = shv_guest_test_vpid_vmexit_handler;
		asm volatile ("vmcall"::"a" (19));
		vcpu->vmexit_handler_override = NULL;
	}
}

/*
 * Wait for interrupt in hypervisor mode, nop when SHV_NO_EFLAGS_IF or
 * SHV_NO_INTERRUPT.
 */
static void shv_guest_wait_int_vmexit_handler(VCPU * vcpu, struct regs *r,
											  vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	ASSERT(r->eax == 25);
	if (!(SHV_OPT & (SHV_NO_EFLAGS_IF | SHV_NO_INTERRUPT))) {
		asm volatile ("sti; hlt; cli;");
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void shv_guest_wait_int(VCPU * vcpu)
{
	vcpu->vmexit_handler_override = shv_guest_wait_int_vmexit_handler;
	asm volatile ("vmcall"::"a" (25));
	vcpu->vmexit_handler_override = NULL;
}

/* Test unrestricted guest by disabling paging */
static void shv_guest_test_unrestricted_guest(VCPU * vcpu)
{
	(void)vcpu;
	if (SHV_OPT & SHV_USE_UNRESTRICTED_GUEST) {
#ifdef __amd64__
		extern void shv_disable_enable_paging(char *);
		if ("quiet") {
			shv_disable_enable_paging("");
		} else {
			shv_disable_enable_paging("SHV guest can disable paging\n");
		}
#elif defined(__i386__)
		ulong_t cr0 = read_cr0();
		asm volatile ("cli");
		write_cr0(cr0 & 0x7fffffffUL);
		if (0) {
			printf("CPU(0x%02x): SHV guest can disable paging\n", vcpu->id);
		}
		write_cr0(cr0);
		if (!(SHV_OPT & SHV_NO_EFLAGS_IF)) {
			asm volatile ("sti");
		}
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
	}
}

/* Test large page in EPT */
static void shv_guest_test_large_page(VCPU * vcpu)
{
	(void)vcpu;
	if (SHV_OPT & SHV_USE_LARGE_PAGE) {
		ASSERT(SHV_OPT & SHV_USE_EPT);
		ASSERT(large_pages[0][0] == 'B');
		ASSERT(large_pages[1][0] == 'A');
	}
}

/* Test running TrustVisor */
static void shv_guest_test_user_vmexit_handler(VCPU * vcpu, struct regs *r,
											   vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	ASSERT(r->eax == 33);
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	enter_user_mode(vcpu, 0);
	vmresume_asm(r);
}

static void shv_guest_test_user(VCPU * vcpu)
{
	if (SHV_OPT & SHV_USER_MODE) {
		ASSERT(!(SHV_OPT & SHV_NO_EFLAGS_IF));
		vcpu->vmexit_handler_override = shv_guest_test_user_vmexit_handler;
		asm volatile ("vmcall"::"a" (33));
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Test running TrustVisor in L2 */
static void shv_guest_test_nested_user_vmexit_handler(VCPU * vcpu,
													  struct regs *r,
													  vmexit_info_t * info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	(void)vcpu;
	(void)r;
	ASSERT(0 && "VMEXIT not allowed");
}

static void shv_guest_test_nested_user(VCPU * vcpu)
{
	if (SHV_OPT & SHV_NESTED_USER_MODE) {
		ASSERT(!(SHV_OPT & SHV_NO_EFLAGS_IF));
		vcpu->vmexit_handler_override =
			shv_guest_test_nested_user_vmexit_handler;
		// asm volatile ("vmcall" : : "a"(0x4c4150ffU));
		enter_user_mode(vcpu, 0x4c415000U);
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Test MSR bitmap */
#define MSR_TEST_NORMAL	0x2900b05d
#define MSR_TEST_VMEXIT	0xf6d7a004
#define MSR_TEST_EXCEPT	0xc23a16e5

static void shv_guest_msr_bitmap_vmexit_handler(VCPU * vcpu, struct regs *r,
												vmexit_info_t * info)
{
	static u8 msr_bitmap_allcpu[MAX_VCPU_ENTRIES][PAGE_SIZE_4K] ALIGNED_PAGE;
	const u32 mask = (1U << 28);
	u8 *msr_bitmap = msr_bitmap_allcpu[vcpu->idx];
	switch (info->vmexit_reason) {
	case VMX_VMEXIT_VMCALL:
		switch (r->eax) {
		case 34:
			/* Enable MSR bitmap */
			{
				u32 val = __vmx_vmread32(VMCS_control_VMX_cpu_based);
				ASSERT((val & mask) == 0);
				val |= mask;
				__vmx_vmwrite32(VMCS_control_VMX_cpu_based, val);
			}
			for (u32 i = 0; i < PAGE_SIZE_4K; i++) {
				ASSERT(msr_bitmap[i] == 0);
			}
			{
				u64 addr = (u64) (ulong_t) msr_bitmap;
				__vmx_vmwrite64(VMCS_control_MSR_Bitmaps_address, addr);
			}
			ASSERT(rdmsr64(0x3b) == 0ULL);
			break;
		case 37:
			/* Disable MSR bitmap */
			{
				u32 val = __vmx_vmread32(VMCS_control_VMX_cpu_based);
				ASSERT((val & mask) == mask);
				val &= ~mask;
				__vmx_vmwrite32(VMCS_control_VMX_cpu_based, val);
			}
			wrmsr64(0x3b, 0ULL);
			break;
		case 38:
			/* Set bit, argument in EBX */
			ASSERT(r->ebx < 0x8000);
			ASSERT(!(msr_bitmap[r->ebx / 8] & (1 << (r->ebx % 8))));
			msr_bitmap[r->ebx / 8] |= (1 << (r->ebx % 8));
			break;
		case 39:
			/* Clear bit, argument in EBX */
			ASSERT(r->ebx < 0x8000);
			ASSERT((msr_bitmap[r->ebx / 8] & (1 << (r->ebx % 8))));
			msr_bitmap[r->ebx / 8] &= ~(1 << (r->ebx % 8));
			break;
		case 41:
			/* Prepare for FS / GS check */
			ASSERT(rdmsr64(IA32_MSR_FS_BASE) == 0ULL);
			ASSERT(rdmsr64(IA32_MSR_GS_BASE) == 0ULL);
			ASSERT(__vmx_vmreadNW(VMCS_guest_FS_base) == 0UL);
			ASSERT(__vmx_vmreadNW(VMCS_guest_GS_base) == 0UL);
			__vmx_vmwriteNW(VMCS_guest_FS_base, 0x680effffUL);
			__vmx_vmwriteNW(VMCS_guest_GS_base, 0x6810ffffUL);
			break;
		case 42:
			/* Check FS / GS */
			ASSERT(__vmx_vmreadNW(VMCS_guest_FS_base) == 0xffff680eUL);
			ASSERT(__vmx_vmreadNW(VMCS_guest_GS_base) == 0xffff6810UL);
			ASSERT(rdmsr64(IA32_MSR_FS_BASE) == 0ULL);
			ASSERT(rdmsr64(IA32_MSR_GS_BASE) == 0ULL);
			__vmx_vmwriteNW(VMCS_guest_FS_base, 0UL);
			__vmx_vmwriteNW(VMCS_guest_GS_base, 0UL);
			break;
		}
		break;
	case VMX_VMEXIT_WRMSR:
		r->ebx = MSR_TEST_VMEXIT;
		break;
	case VMX_VMEXIT_RDMSR:
		switch (r->ecx) {
		case MSR_APIC_BASE:	/* fallthrough */
		case IA32_X2APIC_APICID:
			return;
		default:
			ASSERT(r->ebx == MSR_TEST_NORMAL);
			r->ebx = MSR_TEST_VMEXIT;
			break;
		}
		break;
	case VMX_VMEXIT_CPUID:
		return;
	default:
		ASSERT(0 && "Unknown exit reason");
	}
	__vmx_vmwriteNW(VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void _test_rdmsr(u32 ecx, u32 expected_ebx)
{
	u32 ebx = MSR_TEST_NORMAL, eax, edx;
	asm volatile ("1:\n"
				  "rdmsr\n"
				  "jmp 3f\n"
				  "2:\n"
				  "movl %[e], %%ebx\n" "jmp 3f\n" ".section .xcph_table\n"
#ifdef __amd64__
				  ".quad 0xd\r\n" ".quad 1b\r\n" ".quad 2b\r\n"
#elif defined(__i386__)
				  ".long 0xd\r\n" ".long 1b\r\n" ".long 2b\r\n"
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
				  ".previous\r\n" "3:\r\n":"+b" (ebx), "=a"(eax), "=d"(edx)
				  :"c"(ecx),[e] "g"(MSR_TEST_EXCEPT));
	ASSERT(ebx == expected_ebx);
}

static void _test_wrmsr(u32 ecx, u32 expected_ebx, u64 val)
{
	u32 ebx = MSR_TEST_NORMAL, eax = (u32) val, edx = (u32) (val >> 32);
	asm volatile ("1:\n"
				  "wrmsr\n"
				  "jmp 3f\n"
				  "2:\n"
				  "movl %[e], %%ebx\n" "jmp 3f\n" ".section .xcph_table\n"
#ifdef __amd64__
				  ".quad 0xd\r\n" ".quad 1b\r\n" ".quad 2b\r\n"
#elif defined(__i386__)
				  ".long 0xd\r\n" ".long 1b\r\n" ".long 2b\r\n"
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
				  ".previous\r\n" "3:\r\n":"+b" (ebx)
				  :"c"(ecx), "a"(eax), "d"(edx),[e] "g"(MSR_TEST_EXCEPT));
	ASSERT(ebx == expected_ebx);
}

static void shv_guest_msr_bitmap(VCPU * vcpu)
{
	if (SHV_OPT & SHV_USE_MSRBITMAP) {
		vcpu->vmexit_handler_override = shv_guest_msr_bitmap_vmexit_handler;
		/* Enable MSR bitmap */
		asm volatile ("vmcall"::"a" (34));
		/* Initial: ignore everything */
		_test_rdmsr(0xc0000082, MSR_TEST_NORMAL);
		_test_wrmsr(0xc0000082, MSR_TEST_NORMAL, 0xffffffffffffffffULL);
		asm volatile ("vmcall"::"a" (38), "b"(0x2000 + 0x82));
		/* VMEXIT only read IA32_LSTAR */
		_test_rdmsr(0xc0000082, MSR_TEST_VMEXIT);
		_test_wrmsr(0xc0000082, MSR_TEST_NORMAL, 0xffffffffffffffffULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x2000 + 0x82));
		asm volatile ("vmcall"::"a" (38), "b"(0x6000 + 0x82));
		/* VMEXIT only write IA32_LSTAR */
		_test_rdmsr(0xc0000082, MSR_TEST_NORMAL);
		_test_wrmsr(0xc0000082, MSR_TEST_VMEXIT, 0xffffffffffffffffULL);
		_test_rdmsr(0x0000003b, MSR_TEST_NORMAL);
		_test_wrmsr(0x0000003b, MSR_TEST_NORMAL, 0x1234567890abcdefULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x6000 + 0x82));
		asm volatile ("vmcall"::"a" (38), "b"(0x0000 + 0x3b));
		/* VMEXIT only read IA32_TSC_ADJUST */
		_test_rdmsr(0x0000003b, MSR_TEST_VMEXIT);
		_test_wrmsr(0x0000003b, MSR_TEST_NORMAL, 0xfedcba0987654321ULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x0000 + 0x3b));
		asm volatile ("vmcall"::"a" (38), "b"(0x4000 + 0x3b));
		/* VMEXIT only write IA32_TSC_ADJUST */
		_test_rdmsr(0x0000003b, MSR_TEST_NORMAL);
		_test_wrmsr(0x0000003b, MSR_TEST_VMEXIT, 0xdeadbeefbeefdeadULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x4000 + 0x3b));
		asm volatile ("vmcall"::"a" (38), "b"(0x0000 + 0x1fff));
		/* VMEXIT only read 0x00001fff */
		_test_rdmsr(0x00001fff, MSR_TEST_VMEXIT);
		_test_rdmsr(0xc0001fff, MSR_TEST_EXCEPT);
		_test_wrmsr(0x00001fff, MSR_TEST_EXCEPT, 0x1234567890abcdefULL);
		_test_wrmsr(0xc0001fff, MSR_TEST_EXCEPT, 0x1234567890abcdefULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x0000 + 0x1fff));
		asm volatile ("vmcall"::"a" (38), "b"(0x2000 + 0x1fff));
		/* VMEXIT only read 0xc0001fff */
		_test_rdmsr(0x00001fff, MSR_TEST_EXCEPT);
		_test_rdmsr(0xc0001fff, MSR_TEST_VMEXIT);
		_test_wrmsr(0x00001fff, MSR_TEST_EXCEPT, 0x1234567890abcdefULL);
		_test_wrmsr(0xc0001fff, MSR_TEST_EXCEPT, 0x1234567890abcdefULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x2000 + 0x1fff));
		asm volatile ("vmcall"::"a" (38), "b"(0x4000 + 0x1fff));
		/* VMEXIT only write 0x00001fff */
		_test_rdmsr(0x00001fff, MSR_TEST_EXCEPT);
		_test_rdmsr(0xc0001fff, MSR_TEST_EXCEPT);
		_test_wrmsr(0x00001fff, MSR_TEST_VMEXIT, 0x1234567890abcdefULL);
		_test_wrmsr(0xc0001fff, MSR_TEST_EXCEPT, 0x1234567890abcdefULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x4000 + 0x1fff));
		asm volatile ("vmcall"::"a" (38), "b"(0x6000 + 0x1fff));
		/* VMEXIT only write 0xc0001fff */
		_test_rdmsr(0x00001fff, MSR_TEST_EXCEPT);
		_test_rdmsr(0xc0001fff, MSR_TEST_EXCEPT);
		_test_wrmsr(0x00001fff, MSR_TEST_EXCEPT, 0x1234567890abcdefULL);
		_test_wrmsr(0xc0001fff, MSR_TEST_VMEXIT, 0x1234567890abcdefULL);
		asm volatile ("vmcall"::"a" (39), "b"(0x6000 + 0x1fff));
		/*
		 * Test read / write IA32_FS_BASE / IA32_GS_BASE (will not VMEXIT)
		 * Interrupts need to be disabled, because xcph will move to FS and GS,
		 * which clears the base to 0.
		 */
		asm volatile ("cli");
		asm volatile ("vmcall"::"a" (41));
		_test_rdmsr(IA32_MSR_FS_BASE, MSR_TEST_NORMAL);
		ASSERT(rdmsr64(IA32_MSR_FS_BASE) == 0x680effffULL);
		ASSERT(rdmsr64(IA32_MSR_GS_BASE) == 0x6810ffffULL);
		_test_wrmsr(IA32_MSR_FS_BASE, MSR_TEST_NORMAL, 0xffff680eULL);
		_test_wrmsr(IA32_MSR_GS_BASE, MSR_TEST_NORMAL, 0xffff6810ULL);
		asm volatile ("vmcall"::"a" (42));
		if (!(SHV_OPT & SHV_NO_EFLAGS_IF)) {
			asm volatile ("sti");
		}
		/* Disable MSR bitmap */
		asm volatile ("vmcall"::"a" (37));
		_test_rdmsr(0x00001fff, MSR_TEST_VMEXIT);
		_test_rdmsr(0xc0001fff, MSR_TEST_VMEXIT);
		_test_wrmsr(0x00001fff, MSR_TEST_VMEXIT, 0x1234567890abcdefULL);
		_test_wrmsr(0xc0001fff, MSR_TEST_VMEXIT, 0x1234567890abcdefULL);
		vcpu->vmexit_handler_override = NULL;
	}
}

#undef MSR_TEST_NORMAL
#undef MSR_TEST_VMEXIT
#undef MSR_TEST_EXCEPT

/* Main logic to call subsequent tests */
void shv_guest_main(ulong_t cpu_id)
{
	u64 iter = 0;
	bool in_xmhf = false;
	VCPU *vcpu = get_vcpu();
	ASSERT(cpu_id == vcpu->idx);
	if (NMI_OPT & SHV_NMI_ENABLE) {
		shv_nmi_guest_main(vcpu);
	}
	{
		u32 eax, ebx, ecx, edx;
		cpuid(0x46484d58U, &eax, &ebx, &ecx, &edx);
		in_xmhf = (eax == 0x46484d58U);
	}
	if (!(SHV_OPT & SHV_NO_VGA_ART)) {
		console_vc_t vc;
		console_get_vc(&vc, vcpu->idx, 1);
		console_clear(&vc);
		for (int i = 0; i < vc.width; i++) {
			for (int j = 0; j < 2; j++) {
#ifndef __DEBUG_VGA__
				ASSERT(console_get_char(&vc, i, j) == ' ');
#endif							/* !__DEBUG_VGA__ */
				console_put_char(&vc, i, j, '0' + vcpu->id);
			}
		}
	}
	if (!(SHV_OPT & SHV_NO_EFLAGS_IF)) {
		asm volatile ("sti");
	}
	while (1) {
		/* Assume that iter never wraps around */
		ASSERT(++iter > 0);
		if (in_xmhf) {
			printf("CPU(0x%02x): SHV in XMHF test iter %lld\n", vcpu->id, iter);
		} else {
			printf("CPU(0x%02x): SHV test iter %lld\n", vcpu->id, iter);
		}
		if (!(SHV_OPT & (SHV_NO_EFLAGS_IF | SHV_NO_INTERRUPT))) {
			asm volatile ("hlt");
		}
		if (in_xmhf && (SHV_OPT & SHV_USE_MSR_LOAD) &&
			(SHV_OPT & SHV_USER_MODE)) {
			/*
			 * Due to the way TrustVisor is implemented, cannot change MTRR
			 * after running pal_demo. So we need to disable some tests.
			 */
			if (iter < 3) {
				shv_guest_test_msr_ls(vcpu);
			} else if (iter == 3) {
				/* Implement a barrier and make sure all CPUs arrive */
				static spin_lock_t lock;
				static volatile u32 arrived = 0;
				printf("CPU(0x%02x): enter SHV barrier\n", vcpu->id);
				spin_lock(&lock);
				arrived++;
				spin_unlock(&lock);
				while (arrived < g_midtable_numentries) {
					cpu_relax();
				}
				printf("CPU(0x%02x): leave SHV barrier\n", vcpu->id);
			} else {
				shv_guest_test_user(vcpu);
				shv_guest_test_nested_user(vcpu);
			}
		} else {
			/* Only one of MSR or (user and nested user) will execute */
			shv_guest_test_msr_ls(vcpu);
			shv_guest_test_user(vcpu);
			shv_guest_test_nested_user(vcpu);
		}
		shv_guest_test_ept(vcpu);
		shv_guest_switch_ept(vcpu);
		shv_guest_test_vpid(vcpu);
		if (iter % 5 == 0) {
			shv_guest_test_vmxoff(vcpu, iter % 3 == 0);
		}
		shv_guest_test_unrestricted_guest(vcpu);
		shv_guest_test_large_page(vcpu);
		shv_guest_msr_bitmap(vcpu);
		shv_guest_wait_int(vcpu);
	}
}

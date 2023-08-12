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
#include <lhv.h>

#define IDT_NELEMS 256
uintptr_t g_idt[IDT_NELEMS][2];

static void construct_idt(void)
{
	/* Sync to let only one CPU perform the construction. */
	{
		static spin_lock_t lock;
		static bool initialized = false;
		spin_lock(&lock);
		if (initialized) {
			spin_unlock(&lock);
			return;
		} else {
			initialized = true;
		}
		spin_unlock(&lock);
	}

	/* From XMHF64 xmhf_xcphandler_arch_initialize(). */
	for (u32 i = 0; i < IDT_NELEMS; i++) {
		uintptr_t stub = g_idt_stubs[i];
		idtentry_t *entry = (idtentry_t *)&(g_idt[i][0]);
		entry->isrLow16 = (u16)stub;
		entry->isrHigh16 = (u16)(stub >> 16);
#ifdef __amd64__
		entry->isrHigh32 = (u32)(stub >> 32);
		entry->reserved_zero = 0;
#endif /* defined(__amd64__) */
		entry->isrSelector = __CS;
		/* Set IST to 0. */
		entry->count = 0x0;
		/*
		 * 32-bit / 64-bit interrupt gate.
		 * present=1, DPL=00b, system=0, type=1110b.
		 */
		entry->type = 0x8E;
		/* For 0x23, set DPL to 11b because it is used for syscall. */
		if (i == 0x23) {
		    entry->type |= 0x60;
		}
	}
}

void init_idt(void)
{
	construct_idt();

	/* Load IDT */
	struct {
		u16 limit;
		uintptr_t base;
	} __attribute__((packed)) gdtr = {
		.limit=(uintptr_t)&g_idt[IDT_NELEMS][0] - (uintptr_t)&g_idt[0][0] - 1,
		.base=(uintptr_t)&g_idt,
	};
	asm volatile("lidt %0" : : "m"(gdtr));
}

/* From XMHF64 _svm_and_vmx_getvcpu(). */
static VCPU *get_vcpu(void)
{
	u64 msr_val;
	u32 lapic_id;
	msr_val = rdmsr64(MSR_APIC_BASE);
	if (msr_val & (1ULL << 10)) {
		/* x2APIC is enabled, use it */
		lapic_id = (u32)(rdmsr64(IA32_X2APIC_APICID));
	} else {
		lapic_id = *(u32 *)((uintptr_t)(msr_val & ~0xFFFUL) + 0x20);
		lapic_id = lapic_id >> 24;
	}

	for (u32 i = 0; i < g_midtable_numentries; i++) {
		if (g_midtable[i].cpu_lapic_id == lapic_id) {
			return (VCPU *)g_midtable[i].vcpu_vaddr_ptr;
		}
	}

	HALT_ON_ERRORCOND(0 && ("Unable to retrieve vcpu"));
}

#if 0
static void handle_idt_host(u8 vector, struct regs *r)
{
	HALT_ON_ERRORCOND(0);
}
#endif

void handle_idt(struct regs *r)
{
	VCPU * vcpu = get_vcpu();
	iret_info_t *iret_info = (iret_info_t *)r->sp;
	printf("CPU 0x%02x idt 0x%lx\n", vcpu->id, iret_info->vector);

#if 0
	if (cpuid_ecx(1, 0) & (1U << 5)) {
		handle_idt_host(iret_info->vector, r);
	} else {
		HALT_ON_ERRORCOND(0 && "TODO");
		lhv_guest_xcphandler(iret_info->vector, r);
	}
#endif
}

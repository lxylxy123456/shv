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
		idtentry_t *entry = (idtentry_t *) & (g_idt[i][0]);
		entry->isrLow16 = (u16) stub;
		entry->isrHigh16 = (u16) (stub >> 16);
#ifdef __amd64__
		entry->isrHigh32 = (u32) (stub >> 32);
		entry->reserved_zero = 0;
#endif							/* defined(__amd64__) */
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

	ASSERT(cpuid_ecx(1, 0) & (1U << 5));

	/* Load IDT */
	{
		struct {
			u16 limit;
			uintptr_t base;
		} __attribute__((packed)) idtr = {
			.limit = (uintptr_t) g_idt[IDT_NELEMS] - (uintptr_t) g_idt[0] - 1,
			.base = (uintptr_t) & g_idt,
		};
		asm volatile ("lidt %0"::"m" (idtr));
	}
}

/* From XMHF64 _svm_and_vmx_getvcpu(). */
VCPU *get_vcpu(void)
{
	u64 msr_val;
	u32 lapic_id;
	msr_val = rdmsr64(MSR_APIC_BASE);
	if (msr_val & (1ULL << 10)) {
		/* x2APIC is enabled, use it */
		lapic_id = (u32) (rdmsr64(IA32_X2APIC_APICID));
	} else {
		lapic_id = *(u32 *) ((uintptr_t) (msr_val & ~0xFFFUL) + 0x20);
		lapic_id = lapic_id >> 24;
	}

	for (u32 i = 0; i < g_midtable_numentries; i++) {
		if (g_midtable[i].cpu_lapic_id == lapic_id) {
			return (VCPU *) g_midtable[i].vcpu_vaddr_ptr;
		}
	}

	ASSERT(0 && ("Unable to retrieve vcpu"));
}

/* Dump information when handling exception. */
void dump_exception(VCPU * vcpu, struct regs *r, iret_info_t * info)
{
#define _P(fmt, ...) printf("CPU(0x%02x): " fmt "\n", vcpu->id, __VA_ARGS__)
#define _B (sizeof(uintptr_t) * 2)

	_P("vector:         0x%02lx", info->vector);
	_P("error code:     0x%04lx", info->error_code);
	_P("FLAGS:          0x%08lx", info->flags);
#ifdef __amd64__
	_P("SS: 0x%04hx  SP: 0x%08lx", info->ss, info->sp);
#endif							/* __amd64__ */
	_P("CS: 0x%04hx  IP: 0x%08lx", info->cs, info->ip);
	_P("DS: 0x%04hx  ES: 0x%04hx", info->ds, info->es);
	_P("FS: 0x%04hx  GS: 0x%04hx", info->fs, info->gs);
	_P("AX: 0x%0*lx  CX: 0x%0*lx", _B, r->ax, _B, r->cx);
	_P("DX: 0x%0*lx  BX: 0x%0*lx", _B, r->dx, _B, r->bx);
	_P("SP: 0x%0*lx  BP: 0x%0*lx", _B, r->sp, _B, r->bp);
	_P("SI: 0x%0*lx  DI: 0x%0*lx", _B, r->si, _B, r->di);
#ifdef __amd64__
	_P("R8: 0x%0*lx  R9: 0x%0*lx", _B, r->r8, _B, r->r9);
	_P("R10:0x%0*lx  R11:0x%0*lx", _B, r->r10, _B, r->r11);
	_P("R12:0x%0*lx  R13:0x%0*lx", _B, r->r12, _B, r->r13);
	_P("R14:0x%0*lx  R15:0x%0*lx", _B, r->r14, _B, r->r15);
#endif							/* __amd64__ */

#undef _P
#undef _B
}

void handle_idt(iret_info_t * info)
{
	VCPU *vcpu = get_vcpu();
	struct regs *r = &info->r;
	u8 vector = info->vector;
	bool guest = !(cpuid_ecx(1, 0) & (1U << 5));

	switch (vector) {
	case 0x02:
		handle_nmi_interrupt(vcpu, vector, guest, info->ip);
		break;

	case 0x20:
		handle_timer_interrupt(vcpu, vector, guest);
		break;

	case 0x21:
		handle_keyboard_interrupt(vcpu, vector, guest);
		break;

	case 0x22:
		handle_timer_interrupt(vcpu, vector, guest);
		break;

	case 0x23:
		handle_shv_syscall(vcpu, vector, r);
		break;

	case 0x27:
		/*
		 * We encountered the Mysterious IRQ 7. This has been observed on Bochs
		 * and Dell 7050. The correct way is likely to ignore this interrupt
		 * (without sending EOI to PIC). References:
		 * * https://en.wikipedia.org/wiki/Intel_8259#Spurious_interrupts
		 * * https://wiki.osdev.org/8259_PIC#Spurious_IRQs
		 * * Project 3: Writing a Kernel From Scratch (not publicly available)
		 *    15-410 Operating Systems
		 *    February 25, 2022
		 *    4.1.8 The Mysterious Exception 0x27, aka IRQ 7
		 *
		 * Note that calling printf() here will deadlock if the interrupted
		 * code is already calling printf().
		 */
		if (pic_spurious(7) != 1) {
			ASSERT(0);
		}
		break;

	case 0x2c:
		handle_mouse_interrupt(vcpu, vector, guest);
		break;

	case 0x54:
		handle_ipi_interrupt(vcpu, vector, guest, info->ip);
		break;

	default:
		/* Try to recover using xcph_table. */
		{
			extern uint8_t _begin_xcph_table[];
			extern uint8_t _end_xcph_table[];
			uintptr_t exception_ip = info->ip;
			bool found = false;

			for (uintptr_t * i = (uintptr_t *) _begin_xcph_table;
				 i < (uintptr_t *) _end_xcph_table; i += 3) {
				if (i[0] == vector && i[1] == exception_ip) {
					info->ip = i[2];
					found = true;
					break;
				}
			}

			if (found) {
				break;
			}
		}

		/* Print registers for debugging. */
		printf("CPU(0x%02x): V Unknown %s exception\n", vcpu->id,
			   guest ? "guest" : "host");
		dump_exception(vcpu, r, info);
		printf("CPU(0x%02x): ^ Unknown %s exception\n", vcpu->id,
			   guest ? "guest" : "host");
		HALT();
		break;
	}
}

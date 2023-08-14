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

u64 g_gdt[MAX_VCPU_ENTRIES][GDT_NELEMS];
u8 g_tss[MAX_VCPU_ENTRIES][PAGE_SIZE_4K] ALIGNED_PAGE;

void init_gdt(VCPU * vcpu)
{
	/* Modify GDT entries. */
	g_gdt[vcpu->idx][0] = 0x0000000000000000ULL;
	g_gdt[vcpu->idx][1] = 0x00cf9a000000ffffULL;	// CS 32-bit
	g_gdt[vcpu->idx][2] = 0x00cf92000000ffffULL;	// DS
	g_gdt[vcpu->idx][3] = 0x00af9a000000ffffULL;	// CS 64-bit
	g_gdt[vcpu->idx][4] = 0x0000000000000000ULL;	// TSS
	g_gdt[vcpu->idx][5] = 0x0000000000000000ULL;	// TSS (only used in 64-bit)
#ifdef __amd64__
	g_gdt[vcpu->idx][6] = 0x00affa000000ffffULL;	// CS Ring 3, 64-bit
#else /* !__amd64__ */
	g_gdt[vcpu->idx][6] = 0x00cffa000000ffffULL;	// CS Ring 3, 32-bit
#endif /* __amd64__ */
	g_gdt[vcpu->idx][7] = 0x00cff2000000ffffULL;	// DS Ring 3
	g_gdt[vcpu->idx][8] = 0x0000000000000000ULL;
	g_gdt[vcpu->idx][9] = 0x0000000000000000ULL;

	/* Modify GDT entries for TSS. */
	{
		TSSENTRY * t = (void *)&g_gdt[vcpu->idx][4];
		uintptr_t tss_addr = (uintptr_t)&g_tss[vcpu->idx][0];
		t->attributes1 = 0x89;
		t->limit16_19attributes2 = 0x00;
		t->baseAddr0_15 = (u16)(tss_addr & 0x0000FFFF);
		t->baseAddr16_23 = (u8)((tss_addr & 0x00FF0000) >> 16);
		t->baseAddr24_31 = (u8)((tss_addr & 0xFF000000) >> 24);
#ifdef __amd64__
		t->baseAddr32_63 = (u32)((tss_addr & 0xFFFFFFFF00000000) >> 32);
		t->reserved_zero = 0;
#endif /* __amd64__ */
		t->limit0_15=0x67;
	}

	/* Load GDT. */
	{
		struct {
			u16 limit;
			uintptr_t base;
		} __attribute__((packed)) gdtr = {
			.limit=GDT_NELEMS * 8 - 1,
			.base=(uintptr_t)&(g_gdt[vcpu->idx][0]),
		};
		asm volatile("lgdt %0" : : "m"(gdtr));
	}

	/* Load TR. */
	{
		u16 trsel = __TRSEL;
		asm volatile("ltr %0" : : "m"(trsel));
	}
}

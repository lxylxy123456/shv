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

/* Template from https://wiki.osdev.org/Bare_Bones */

#include <xmhf.h>

/* This function is called from boot.S, only BSP. */
void kernel_main(multiboot_info_t * mbi)
{
	/* Initialize terminal interface. */
	{
		extern void emhfc_debug_init(void);
		emhfc_debug_init();
	}

	/* Print banner. */
	{
		printf("Small Hyper Visor (SHV)\n");
#ifdef __amd64__
		printf("Subarch: amd64\n");
#elif __i386__
#if I386_PAE
		printf("Subarch: i386 (PAE)\n");
#else							/* !I386_PAE */
		printf("Subarch: i386 (not PAE)\n");
#endif							/* I386_PAE */
#endif
	}

	/* Parse command line. */
	if (mbi->flags & MBI_CMDLINE) {
		const char *cmdline = (const char *)(uintptr_t) mbi->cmdline;
		printf("Multiboot command line: %s\n", cmdline);
		parse_cmdline(cmdline);
	} else {
		printf("No multiboot command line\n");
	}

	/* Print options. */
	{
		printf("g_shv_opt: 0x%llx\n", g_shv_opt);
		printf("g_nmi_opt: 0x%llx\n", g_nmi_opt);
		printf("g_nmi_exp: 0x%llx\n", g_nmi_exp);
	}

	/* Set up page table. */
	{
		g_cr3 = shv_page_table_init();
		g_cr4 = read_cr4();

#ifdef __i386__
#if I386_PAE
		/* Enable CR4.PAE. */
		ASSERT((cpuid_edx(1U, 0U) & (1U << 6)));
		g_cr4 |= CR4_PAE;
#else							/* !I386_PAE */
		/* Enable CR4.PSE, so we can use 4MB pages. */
		ASSERT((cpuid_edx(1U, 0U) & (1U << 3)));
		g_cr4 |= CR4_PSE;
#endif							/* I386_PAE */
#endif							/* !__i386__ */
	}

	/* Initialize SMP. */
	{
		smp_init();
	}

	ASSERT(0 && "Should not reach here");
}

/* This function is called from smp-asm.S, both BSP and AP. */
void kernel_main_smp(VCPU * vcpu)
{
	printf("Hello, %s World %d!\n", "smp", vcpu->id);

#ifdef __i386__
	/* In i386, CR3 and CR4 are not set in assembly code. */
	write_cr3(g_cr3);
	write_cr4(g_cr4);

	/* Enable paging. */
	write_cr0(read_cr0() | CR0_PG);
#endif							/* !__i386__ */

	/* Barrier */
	{
		static u32 count = 0;
		lock_incl(&count);
		while (count < g_midtable_numentries) {
			cpu_relax();
		}
	}

	/* Initialize GDT, IDT, etc. */
	{
		init_gdt(vcpu);
		init_idt();
	}

	/* Transfer control to SHV */
	{
		extern void shv_main(VCPU * vcpu);
		shv_main(vcpu);
	}

	ASSERT(0 && "Should not reach here");
}

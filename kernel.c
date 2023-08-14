/* Template from https://wiki.osdev.org/Bare_Bones */

#include <xmhf.h>

/* This function is called from boot.S, only BSP. */
void kernel_main(void)
{
	/* Initialize terminal interface. */
	{
		extern void dbg_x86_vgamem_init(void);
		dbg_x86_vgamem_init();
	}

	/* Print banner. */
	{
		printf("Small Hyper Visor (SHV)\n");
#ifdef __amd64__
		printf("Subarch: amd64\n");
#elif __i386__
		printf("Subarch: i386\n");
#endif
		printf("SHV Options: 0x%x\n", SHV_OPT);
	}

	/* Set up page table. */
	{
		g_cr3 = shv_page_table_init();
		g_cr4 = read_cr4();

#ifdef __i386__
		/* Enable CR4.PSE, so we can use 4MB pages. */
		HALT_ON_ERRORCOND((cpuid_edx(1U, 0U) & (1U << 3)));
		g_cr4 |= CR4_PSE;
#endif /* !__i386__ */
	}

	/* Initialize SMP. */
	{
		smp_init();
	}

	HALT_ON_ERRORCOND(0 && "Should not reach here");
}

/* This function is called from smp-asm.S, both BSP and AP. */
void kernel_main_smp(VCPU *vcpu)
{
	printf("Hello, %s World %d!\n", "smp", vcpu->id);

#ifdef __i386__
	/* In i386, CR3 and CR4 are not set in assembly code. */
	write_cr3(g_cr3);
	write_cr4(g_cr4);

	/* Enable paging. */
	write_cr0(read_cr0() | CR0_PG);
#endif /* !__i386__ */

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

	/* Transfer control to LHV */
	{
		extern void lhv_main(VCPU *vcpu);
		lhv_main(vcpu);
	}

	HALT_ON_ERRORCOND(0 && "Should not reach here");
}

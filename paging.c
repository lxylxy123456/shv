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

#ifdef __amd64__
volatile u64 shv_pml4t[P4L_NPLM4T * P4L_NEPT] ALIGNED_PAGE;
volatile u64 shv_pdpt[P4L_NPDPT * P4L_NEPT] ALIGNED_PAGE;
volatile u64 shv_pdt[P4L_NPDT * P4L_NEPT] ALIGNED_PAGE;
#elif defined(__i386__)
#if I386_PAE
volatile u64 shv_pdpt[PAE_NPDPTE] __attribute__((aligned(32)));
volatile u64 shv_pdt[PAE_NPDT * PAE_NEPT] ALIGNED_PAGE;
#else							/* !I386_PAE */
volatile u32 shv_pdt[P32_NPDT * P32_NEPT] ALIGNED_PAGE;
#endif							/* I386_PAE */
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */

uintptr_t shv_page_table_init(void)
{
#ifdef __amd64__
	for (u64 i = 0, paddr = (uintptr_t) shv_pdpt; i < P4L_NPDPT; i++) {
		if (i < 1) {
			ASSERT((0x60ULL | shv_pml4t[i]) == (0x63ULL | paddr));
		} else {
			shv_pml4t[i] = 0x3ULL | paddr;
		}
		paddr += PAGE_SIZE_4K;
	}
	for (u64 i = 0, paddr = (uintptr_t) shv_pdt; i < P4L_NPDT; i++) {
		if (i < 4) {
			ASSERT((0x60ULL | shv_pdpt[i]) == (0x63ULL | paddr));
		} else {
			shv_pdpt[i] = 0x3ULL | paddr;
		}
		paddr += PAGE_SIZE_4K;
	}
	for (u64 i = 0, paddr = 0; i < P4L_NPT; i++, paddr += PA_PAGE_SIZE_2M) {
		if (i < 2048) {
			ASSERT((0x60ULL | shv_pdt[i]) == (0xe3ULL | paddr));
		} else {
			shv_pdt[i] = 0x83ULL | paddr;
		}
	}
	return (uintptr_t) shv_pml4t;
#elif defined(__i386__)
#if I386_PAE
	for (u64 i = 0, paddr = (uintptr_t) shv_pdt; i < PAE_NPDPTE; i++) {
		shv_pdpt[i] = 0x1U | paddr;
		paddr += PAGE_SIZE_4K;
	}
	for (u64 i = 0, paddr = 0; i < PAE_NPDT * PAE_NEPT;
		 i++, paddr += PA_PAGE_SIZE_2M) {
		shv_pdt[i] = 0x83U | paddr;
	}
	return (uintptr_t) shv_pdpt;
#else							/* !I386_PAE */
	for (u32 i = 0, paddr = 0; i < P32_NPDT * P32_NEPT;
		 i++, paddr += PA_PAGE_SIZE_4M) {
		shv_pdt[i] = 0x83U | paddr;
	}
	  return (uintptr_t) shv_pdt ;
#endif							/* I386_PAE */
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
}

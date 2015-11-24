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

#include <xmhf.h>
#include <xmhf-debug.h>

#include <xmhfgeec.h>

#include <geec_prime.h>
#include <geec_sentinel.h>
#include <uapi_slabmempgtbl.h>
#include <xc_init.h>

/*@
	behavior txttpm:
		assumes (paddr == 0xfee00000 || paddr == 0xfec00000);
		ensures (\result == (_PAGE_RW | _PAGE_PSE | _PAGE_PRESENT | _PAGE_PCD));

	behavior other:
		assumes !(paddr == 0xfee00000 || paddr == 0xfec00000);
		ensures (\result == (_PAGE_RW | _PAGE_PSE | _PAGE_PRESENT));

	complete behaviors;
	disjoint behaviors;
@*/
static u64 _gp_s1_bspstack_getflagsforspa(u32 paddr){
	if(paddr == 0xfee00000 || paddr == 0xfec00000)
                return (_PAGE_RW | _PAGE_PSE | _PAGE_PRESENT | _PAGE_PCD);
	else
		return (_PAGE_RW | _PAGE_PSE | _PAGE_PRESENT);
}

#if 0
void gp_s1_bspstack(void){
	u32 i, j;
	u64 flags;

	//clear PDPT
	memset(&_xcprimeon_init_pdpt, 0, sizeof(_xcprimeon_init_pdpt));

	for(i=0; i < PAE_PTRS_PER_PDPT; i++){
		_xcprimeon_init_pdpt[i] = pae_make_pdpe(&_xcprimeon_init_pdt[i][0], (_PAGE_PRESENT));

		for(j=0; j < PAE_PTRS_PER_PDT; j++){
		    flags = _gp_s1_bspstack_getflagsforspa((i*(PAGE_SIZE_2M * PAE_PTRS_PER_PDT)) + (PAGE_SIZE_2M * j));

		    _xcprimeon_init_pdt[i][j] = pae_make_pde_big(((i*(PAGE_SIZE_2M * PAE_PTRS_PER_PDT)) + (PAGE_SIZE_2M * j)), flags);
		}
	}


	gp_s1_bspstkactivate();
}
#endif // 0


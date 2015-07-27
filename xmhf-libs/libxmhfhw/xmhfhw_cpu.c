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

//xmhfhw_cpu - base CPU functions
//author: amit vasudevan (amitvasudevan@acm.org)

#include <xmhf.h>
#include <xmhf-hwm.h>
#include <xmhfhw.h>
#include <xmhf-debug.h>




int fls(int mask)
{
    return (mask == 0 ? mask : (int)bsrl((u32)mask) + 1);
}



u32 get_cpu_vendor_or_die(void) {
	    u32 dummy;
	    u32 vendor_dword1, vendor_dword2, vendor_dword3;

 CASM_FUNCCALL(xmhfhw_cpu_cpuid,0, &dummy, &vendor_dword1, &vendor_dword3, &vendor_dword2);
	    if(vendor_dword1 == AMD_STRING_DWORD1 && vendor_dword2 == AMD_STRING_DWORD2
	       && vendor_dword3 == AMD_STRING_DWORD3)
		return CPU_VENDOR_AMD;
	    else if(vendor_dword1 == INTEL_STRING_DWORD1 && vendor_dword2 == INTEL_STRING_DWORD2
		    && vendor_dword3 == INTEL_STRING_DWORD3)
		return CPU_VENDOR_INTEL;
	    else
		HALT();

	    return 0; // never reached
	}


//*
//returns true if CPU has support for XSAVE/XRSTOR
bool xmhf_baseplatform_arch_x86_cpuhasxsavefeature(void){
	u32 eax, ebx, ecx, edx;

	//bit 26 of ECX is 1 in CPUID function 0x00000001 if
	//XSAVE/XRSTOR feature is available

 CASM_FUNCCALL(xmhfhw_cpu_cpuid,0x00000001, &eax, &ebx, &ecx, &edx);

	if((ecx & (1UL << 26)))
		return true;
	else
		return false;

}


//*
//get CPU vendor
u32 xmhf_baseplatform_arch_x86_getcpuvendor(void){
	u32 reserved, vendor_dword1, vendor_dword2, vendor_dword3;
	u32 cpu_vendor;

 CASM_FUNCCALL(xmhfhw_cpu_cpuid,0, &reserved, &vendor_dword1, &vendor_dword3, &vendor_dword2);

	if(vendor_dword1 == AMD_STRING_DWORD1 && vendor_dword2 == AMD_STRING_DWORD2
			&& vendor_dword3 == AMD_STRING_DWORD3)
		cpu_vendor = CPU_VENDOR_AMD;
	else if(vendor_dword1 == INTEL_STRING_DWORD1 && vendor_dword2 == INTEL_STRING_DWORD2
			&& vendor_dword3 == INTEL_STRING_DWORD3)
		cpu_vendor = CPU_VENDOR_INTEL;
	else{
		cpu_vendor = CPU_VENDOR_UNKNOWN;
		//_XDPRINTF_("%s: unrecognized x86 CPU (0x%08x:0x%08x:0x%08x). HALT!\n",
		//	__FUNCTION__, vendor_dword1, vendor_dword2, vendor_dword3);
		//HALT();
	}

	return cpu_vendor;
}

//*
u32 xmhf_baseplatform_arch_getcpuvendor(void){
	return xmhf_baseplatform_arch_x86_getcpuvendor();
}

uint64_t read_pub_config_reg(uint32_t reg)
{
    return CASM_FUNCCALL(read_config_reg,TXT_PUB_CONFIG_REGS_BASE, reg);
}

void write_pub_config_reg(uint32_t reg, uint64_t val)
{
 CASM_FUNCCALL(write_config_reg,TXT_PUB_CONFIG_REGS_BASE, reg, val);
}

uint64_t read_priv_config_reg(uint32_t reg)
{
    return CASM_FUNCCALL(read_config_reg,TXT_PRIV_CONFIG_REGS_BASE, reg);
}

void write_priv_config_reg(uint32_t reg, uint64_t val)
{
 CASM_FUNCCALL(write_config_reg,TXT_PRIV_CONFIG_REGS_BASE, reg, val);
}

bool txt_is_launched(void)
{
    txt_sts_t sts;

    //sts._raw = read_pub_config_reg(TXTCR_STS);
    unpack_txt_sts_t(&sts, read_pub_config_reg(TXTCR_STS));

    return sts.senter_done_sts;
}







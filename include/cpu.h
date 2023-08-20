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

__attribute__((noreturn))
static inline void cpu_halt(void)
{
	while (1) {
		asm volatile("hlt");
	}
}

static inline void cpu_relax(void)
{
	asm volatile("pause");
}

static inline u8 inb(u16 port)
{
	u8 ans;
	asm volatile("inb %1, %0" : "=a"(ans) : "d"(port));
	return ans;
}

static inline void outb(u16 port, u8 val)
{
	asm volatile("outb %1, %0" : : "d"(port), "a"(val));
}

static inline ulong_t read_cr0(void)
{
	ulong_t ans;
	asm volatile("mov %%cr0, %0" : "=r"(ans));
	return ans;
}

static inline void write_cr0(ulong_t val)
{
	asm volatile("mov %0, %%cr0" : : "r"(val));
}

static inline ulong_t read_cr3(void)
{
	ulong_t ans;
	asm volatile("mov %%cr3, %0" : "=r"(ans));
	return ans;
}

static inline void write_cr3(ulong_t val)
{
	asm volatile("mov %0, %%cr3" : : "r"(val));
}

static inline ulong_t read_cr4(void)
{
	ulong_t ans;
	asm volatile("mov %%cr4, %0" : "=r"(ans));
	return ans;
}

static inline void write_cr4(ulong_t val)
{
	asm volatile("mov %0, %%cr4" : : "r"(val));
}

static inline u16 read_cs(void)
{
	u16 ans;
	asm volatile("mov %%cs, %0" : "=g"(ans));
	return ans;
}

static inline u16 read_ds(void)
{
	u16 ans;
	asm volatile("mov %%ds, %0" : "=g"(ans));
	return ans;
}

static inline u16 read_es(void)
{
	u16 ans;
	asm volatile("mov %%es, %0" : "=g"(ans));
	return ans;
}

static inline u16 read_fs(void)
{
	u16 ans;
	asm volatile("mov %%fs, %0" : "=g"(ans));
	return ans;
}

static inline u16 read_gs(void)
{
	u16 ans;
	asm volatile("mov %%gs, %0" : "=g"(ans));
	return ans;
}

static inline u16 read_ss(void)
{
	u16 ans;
	asm volatile("mov %%ss, %0" : "=g"(ans));
	return ans;
}

static inline u16 read_tr(void)
{
	u16 ans;
	asm volatile("str %0" : "=g"(ans));
	return ans;
}

static inline u64 rdmsr64(u32 msr)
{
	union {
		struct {
			u32 eax;
			u32 edx;
		};
		u64 eaxedx;
	} ans;
	asm volatile("rdmsr" : "=a"(ans.eax), "=d"(ans.edx) : "c"(msr));
	return ans.eaxedx;
}

static inline void wrmsr64(u32 msr, u64 val)
{
	union {
		struct {
			u32 eax;
			u32 edx;
		};
		u64 eaxedx;
	} ans = { .eaxedx=val };
	asm volatile("wrmsr" : : "a"(ans.eax), "d"(ans.edx), "c"(msr));
}

static inline void cpuid(u32 func, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx)
{
	asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) :
				 "a"(func), "c"(0));
}

static inline void cpuid_raw(u32 *eax, u32 *ebx, u32 *ecx, u32 *edx)
{
	asm volatile("cpuid" : "+a"(*eax), "=b"(*ebx), "+c"(*ecx), "=d"(*edx));
}

static inline u32 cpuid_eax(u32 eax, u32 ecx)
{
	asm volatile("cpuid" : "+a"(eax) : "c"(ecx) : "ebx", "edx");
	return eax;
}

static inline u32 cpuid_ebx(u32 eax, u32 ecx)
{
	u32 ebx;
	asm volatile("cpuid" : "=b"(ebx) : "a"(eax), "c"(ecx) : "edx");
	return ebx;
}

static inline u32 cpuid_ecx(u32 eax, u32 ecx)
{
	asm volatile("cpuid" : "+c"(ecx) : "a"(eax) : "ebx", "edx");
	return ecx;
}

static inline u32 cpuid_edx(u32 eax, u32 ecx)
{
	u32 edx;
	asm volatile("cpuid" : "=d"(edx) : "a"(eax), "c"(ecx) : "ebx");
	return edx;
}

static inline void lock_incl(volatile u32 *num)
{
	asm volatile("lock incl %0" : "+m"(*num));
}

static inline u64 rdtsc(void)
{
	u32 eax, edx;
	asm volatile("rdtsc" : "=a"(eax), "=d"(edx));
	return ((u64)edx << 32) | eax;
}

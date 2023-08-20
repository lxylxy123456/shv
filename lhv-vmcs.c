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

// TODO: optimize by caching

/* Write 16-bit VMCS field, never fails */
void __vmx_vmwrite16(u16 encoding, u16 value) {
	ASSERT((encoding >> 12) == 0UL);
	ASSERT(__vmx_vmwrite(encoding, value));
}

/* Write 64-bit VMCS field, never fails */
void __vmx_vmwrite64(u16 encoding, u64 value) {
	ASSERT((encoding >> 12) == 2UL);
	ASSERT((encoding & 0x1) == 0x0);
#ifdef __amd64__
	ASSERT(__vmx_vmwrite(encoding, value));
#elif defined(__i386__)
	ASSERT(__vmx_vmwrite(encoding, value));
	ASSERT(__vmx_vmwrite(encoding + 1, value >> 32));
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */
}

/* Write 32-bit VMCS field, never fails */
void __vmx_vmwrite32(u16 encoding, u32 value) {
	ASSERT((encoding >> 12) == 4UL);
	ASSERT(__vmx_vmwrite(encoding, value));
}

/* Write natural width (NW) VMCS field, never fails */
void __vmx_vmwriteNW(u16 encoding, ulong_t value) {
	ASSERT((encoding >> 12) == 6UL);
	ASSERT(__vmx_vmwrite(encoding, value));
}

/* Read 16-bit VMCS field, return whether succeed */
bool __vmx_vmread16_safe(u16 encoding, u16 *result)
{
	unsigned long value;
	ASSERT((encoding >> 12) == 0UL);
	if (!__vmx_vmread(encoding, &value)) {
		return false;
	}
	ASSERT(value == (unsigned long)(u16)value);
	*result = value;
	return true;
}

/* Read 16-bit VMCS field, never fails */
u16 __vmx_vmread16(u16 encoding) {
	u16 value;
	ASSERT(__vmx_vmread16_safe(encoding, &value));
	return value;
}

/* Read 64-bit VMCS field, return whether succeed */
bool __vmx_vmread64_safe(u16 encoding, u64 *result)
{
#ifdef __amd64__
	unsigned long value;
	ASSERT((encoding >> 12) == 2UL);
	if (!__vmx_vmread(encoding, &value)) {
		return false;
	}
	*result = value;
	return true;
#elif defined(__i386__)
	union {
		struct {
			unsigned long low, high;
		};
		u64 full;
	} ans;
	_Static_assert(sizeof(u32) == sizeof(unsigned long), "incorrect size");
	ASSERT((encoding >> 12) == 2UL);
	ASSERT((encoding & 0x1) == 0x0);
	if (!__vmx_vmread(encoding, &ans.low)) {
		return false;
	}
	/* Since reading low succeeds, assume reading high will succeed. */
	ASSERT(__vmx_vmread(encoding + 1, &ans.high));
	*result = ans.full;
	return true;
#else /* !defined(__i386__) && !defined(__amd64__) */
    #error "Unsupported Arch"
#endif /* !defined(__i386__) && !defined(__amd64__) */
}

/* Read 64-bit VMCS field, never fails */
u64 __vmx_vmread64(u16 encoding)
{
	u64 value;
	ASSERT(__vmx_vmread64_safe(encoding, &value));
	return value;
}

/* Read 32-bit VMCS field, return whether succeed */
bool __vmx_vmread32_safe(u16 encoding, u32 *result)
{
	unsigned long value;
	ASSERT((encoding >> 12) == 4UL);
	if (!__vmx_vmread(encoding, &value)) {
		return false;
	}
	ASSERT(value == (unsigned long)(u32)value);
	*result = value;
	return true;
}

/* Read 32-bit VMCS field, never fails */
u32 __vmx_vmread32(u16 encoding)
{
	u32 value;
	ASSERT(__vmx_vmread32_safe(encoding, &value));
	return value;
}

/* Read natural width (NW) VMCS field, return whether succeed */
bool __vmx_vmreadNW_safe(u16 encoding, ulong_t *result)
{
	unsigned long value;
	ASSERT((encoding >> 12) == 6UL);
	if (!__vmx_vmread(encoding, &value)) {
		return false;
	}
	ASSERT(value == (unsigned long)(ulong_t)value);
	*result = value;
	return true;
}

/* Read natural width (NW) VMCS field, never fails */
ulong_t __vmx_vmreadNW(u16 encoding)
{
	ulong_t value;
	ASSERT(__vmx_vmreadNW_safe(encoding, &value));
	return value;
}

void vmcs_vmwrite(VCPU *vcpu, ulong_t encoding, ulong_t value)
{
	(void) vcpu;
	// printf("CPU(0x%02x): vmwrite(0x%04lx, 0x%08lx)\n", vcpu->id, encoding, value);
	ASSERT(__vmx_vmwrite(encoding, value));
}

void vmcs_vmwrite64(VCPU *vcpu, ulong_t encoding, u64 value)
{
	(void) vcpu;
	__vmx_vmwrite64(encoding, value);
}

ulong_t vmcs_vmread(VCPU *vcpu, ulong_t encoding)
{
	unsigned long value;
	(void) vcpu;
	ASSERT(__vmx_vmread(encoding, &value));
	// printf("CPU(0x%02x): 0x%08lx = vmread(0x%04lx)\n", vcpu->id, value, encoding);
	return value;
}

u64 vmcs_vmread64(VCPU *vcpu, ulong_t encoding)
{
	(void) vcpu;
	return __vmx_vmread64(encoding);
}

void vmcs_print(VCPU *vcpu)
{
#define DECLARE_FIELD_16(encoding, name, ...) \
	{ \
		u16 value; \
		if (__vmx_vmread16_safe(encoding, &value)) { \
			printf("CPU(0x%02x): vmread(0x%04x) = %04hx\n", vcpu->id, \
				   encoding, value); \
		} else { \
			printf("CPU(0x%02x): vmread(0x%04x) = unavailable\n", vcpu->id, \
				   encoding); \
		} \
	}
#define DECLARE_FIELD_64(encoding, name, ...) \
	{ \
		u64 value; \
		if (__vmx_vmread64_safe(encoding, &value)) { \
			printf("CPU(0x%02x): vmread(0x%04x) = %016llx\n", vcpu->id, \
				   encoding, value); \
		} else { \
			printf("CPU(0x%02x): vmread(0x%04x) = unavailable\n", vcpu->id, \
				   encoding); \
		} \
	}
#define DECLARE_FIELD_32(encoding, name, ...) \
	{ \
		u32 value; \
		if (__vmx_vmread32_safe(encoding, &value)) { \
			printf("CPU(0x%02x): vmread(0x%04x) = %08x\n", vcpu->id, \
				   encoding, value); \
		} else { \
			printf("CPU(0x%02x): vmread(0x%04x) = unavailable\n", vcpu->id, \
				   encoding); \
		} \
	}
#define DECLARE_FIELD_NW(encoding, name, ...) \
	{ \
		ulong_t value; \
		if (__vmx_vmreadNW_safe(encoding, &value)) { \
			printf("CPU(0x%02x): vmread(0x%04x) = %08lx\n", vcpu->id, \
				   encoding, value); \
		} else { \
			printf("CPU(0x%02x): vmread(0x%04x) = unavailable\n", vcpu->id, \
				   encoding); \
		} \
	}
#include <_vmx_vmcs_fields.h>
#undef DECLARE_FIELD_16
#undef DECLARE_FIELD_64
#undef DECLARE_FIELD_32
#undef DECLARE_FIELD_NW
}

void vmcs_dump(VCPU *vcpu, int verbose)
{
	#define DECLARE_FIELD(encoding, name)								\
		do {															\
			if ((encoding & 0x6000) == 0x0000) {						\
				vcpu->vmcs.name = __vmx_vmread16(encoding);				\
			} else if ((encoding & 0x6000) == 0x2000) {					\
				vcpu->vmcs.name = __vmx_vmread64(encoding);				\
			} else if ((encoding & 0x6000) == 0x4000) {					\
				vcpu->vmcs.name = __vmx_vmread32(encoding);				\
			} else {													\
				ASSERT((encoding & 0x6000) == 0x6000);		\
				vcpu->vmcs.name = __vmx_vmreadNW(encoding);				\
			}															\
			if (!verbose) {												\
				break;													\
			}															\
			if (sizeof(vcpu->vmcs.name) == 4) {							\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%08x\n",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else if (sizeof(vcpu->vmcs.name) == 8) {					\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%016llx\n",	\
						vcpu->id, vcpu->vmcs.name);						\
			} else if (sizeof(vcpu->vmcs.name) == 2) {					\
				printf("CPU(0x%02x): vcpu->vmcs." #name "=0x%04x\n",	\
						vcpu->id, (u32) vcpu->vmcs.name);				\
			} else {													\
				ASSERT(0);									\
			}															\
		} while (0);
	#include <_vmx_vmcs_fields.h>
	#undef DECLARE_FIELD
}

void vmcs_load(VCPU *vcpu)
{
	#define DECLARE_FIELD(encoding, name)								\
		do {															\
			if ((encoding & 0x6000) == 0x0000) {						\
				__vmx_vmwrite16(encoding, vcpu->vmcs.name);				\
			} else if ((encoding & 0x6000) == 0x2000) {					\
				__vmx_vmwrite64(encoding, vcpu->vmcs.name);				\
			} else if ((encoding & 0x6000) == 0x4000) {					\
				__vmx_vmwrite32(encoding, vcpu->vmcs.name);				\
			} else {													\
				ASSERT((encoding & 0x6000) == 0x6000);		\
				__vmx_vmwriteNW(encoding, vcpu->vmcs.name);				\
			}															\
		} while (0);
	#include <_vmx_vmcs_fields.h>
	#undef DECLARE_FIELD
}

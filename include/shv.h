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

#ifndef _SHV_H_
#define _SHV_H_

#ifndef __ASSEMBLY__

typedef struct {
	int left;
	int top;
	int width;
	int height;
	char color;
} console_vc_t;

/* Begin of bit definitions for SHV_OPT */
#define SHV_USE_MSR_LOAD			0x0000000000000001ULL
#define SHV_NO_EFLAGS_IF			0x0000000000000002ULL
#define SHV_USE_EPT					0x0000000000000004ULL
#define SHV_USE_SWITCH_EPT			0x0000000000000008ULL	/* Need 0x4 */
#define SHV_USE_VPID				0x0000000000000010ULL
#define SHV_USE_UNRESTRICTED_GUEST	0x0000000000000020ULL	/* Need 0x4 */
#define SHV_USER_MODE				0x0000000000000040ULL	/* Need !0x2 */
#define SHV_USE_VMXOFF				0x0000000000000080ULL
#define SHV_USE_LARGE_PAGE			0x0000000000000100ULL	/* Need 0x4 */
#define SHV_NO_INTERRUPT			0x0000000000000200ULL
#define SHV_USE_MSRBITMAP			0x0000000000000400ULL
#define SHV_NESTED_USER_MODE		0x0000000000000800ULL	/* Need !0x2 */
#define SHV_USE_PS2_MOUSE			0x0000000000001000ULL
/* End of bit definitions for SHV_OPT */

/* shv.c */
void shv_main(VCPU * vcpu);
void handle_shv_syscall(VCPU * vcpu, int vector, struct regs *r);

/* shv-console.c */
void console_cursor_clear(void);
void console_clear(console_vc_t * vc);
char console_get_char(console_vc_t * vc, int x, int y);
void console_put_char(console_vc_t * vc, int x, int y, char c);
void console_get_vc(console_vc_t * vc, int num, int guest);

/* shv-timer.c */
void timer_init(VCPU * vcpu);
void handle_timer_interrupt(VCPU * vcpu, int vector, int guest);

/* shv-pic.c */
void pic_init(void);
int pic_spurious(unsigned char irq);

/* shv-keyboard.c */
void handle_keyboard_interrupt(VCPU * vcpu, int vector, int guest);

/* shv-mouse.c */
void mouse_init(VCPU * vcpu);
void handle_mouse_interrupt(VCPU * vcpu, int vector, int guest);

/* shv-vmx.c */
void shv_vmx_main(VCPU * vcpu);
void vmentry_error(ulong_t is_resume, ulong_t valid);

/* shv-asm.S */
void vmexit_asm(void);			/* Called by hardware only */
void vmlaunch_asm(struct regs *r);	/* Never returns */
void vmresume_asm(struct regs *r);	/* Never returns */

/* shv-ept.c */
extern u8 large_pages[2][PAGE_SIZE_2M] __attribute__((aligned(PAGE_SIZE_2M)));

/*
 * When this is larger than XMHF's VMX_NESTED_MAX_ACTIVE_EPT, should see a lot
 * of EPT cache misses.
 */
#define SHV_EPT_COUNT 2

void shv_ept_init(VCPU * vcpu);
u64 shv_build_ept(VCPU * vcpu, u8 ept_num);

/* shv-vmcs.c */
void __vmx_vmwrite16(u16 encoding, u16 value);
void __vmx_vmwrite64(u16 encoding, u64 value);
void __vmx_vmwrite32(u16 encoding, u32 value);
void __vmx_vmwriteNW(u16 encoding, ulong_t value);
bool __vmx_vmread16_safe(u16 encoding, u16 * result);
u16 __vmx_vmread16(u16 encoding);
bool __vmx_vmread64_safe(u16 encoding, u64 * result);
u64 __vmx_vmread64(u16 encoding);
bool __vmx_vmread32_safe(u16 encoding, u32 * result);
u32 __vmx_vmread32(u16 encoding);
bool __vmx_vmreadNW_safe(u16 encoding, ulong_t * result);
ulong_t __vmx_vmreadNW(u16 encoding);
void vmcs_print_all(VCPU * vcpu);
void vmcs_dump(VCPU * vcpu, int verbose);
void vmcs_load(VCPU * vcpu);

/* shv-guest-asm.S */
void shv_guest_entry(void);
void shv_guest_xcphandler(VCPU * vcpu, struct regs *r, iret_info_t * info);

/* shv-user.c */
typedef struct ureg_t {
	struct regs r;
	uintptr_t eip;
	uintptr_t cs;
	uintptr_t eflags;
	uintptr_t esp;
	uintptr_t ss;
} ureg_t;
void enter_user_mode(VCPU * vcpu, ulong_t arg);
void user_main(VCPU * vcpu, ulong_t arg);

/* shv-user-asm.S */
void enter_user_mode_asm(ureg_t * ureg, uintptr_t * esp0);
void exit_user_mode_asm(uintptr_t esp0);

/* LAPIC */
#define LAPIC_DEFAULT_BASE    0xfee00000
#define IOAPIC_DEFAULT_BASE   0xfec00000
#define LAPIC_EOI              0x0B0	/* EOI */
#define LAPIC_SVR              0x0F0	/* Spurious Interrupt Vector */
#define LAPIC_LVT_TIMER        0x320	/* Local Vector Table 0 (TIMER) */
#define LAPIC_TIMER_INIT       0x380	/* Timer Initial Count */
#define LAPIC_TIMER_CUR        0x390	/* Timer Current Count */
#define LAPIC_TIMER_DIV        0x3E0	/* Timer Divide Configuration */
#define LAPIC_ENABLE      0x00000100	/* Unit Enable */

static inline u32 read_lapic(u32 reg)
{
	return *(volatile u32 *)(uintptr_t) (LAPIC_DEFAULT_BASE + reg);
}

static inline void write_lapic(u32 reg, u32 val)
{
	*(volatile u32 *)(uintptr_t) (LAPIC_DEFAULT_BASE + reg) = val;
}

#endif							/* !__ASSEMBLY__ */

#endif							/* _SHV_H_ */

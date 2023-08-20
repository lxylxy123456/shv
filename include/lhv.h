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

#ifndef _LHV_H_
#define _LHV_H_

#ifndef __ASSEMBLY__

typedef struct {
	int left;
	int top;
	int width;
	int height;
	char color;
} console_vc_t;

/* Begin of bit definitions for SHV_OPT */
#define LHV_USE_MSR_LOAD			0x0000000000000001ULL
#define LHV_NO_EFLAGS_IF			0x0000000000000002ULL
#define LHV_USE_EPT					0x0000000000000004ULL
#define LHV_USE_SWITCH_EPT			0x0000000000000008ULL	/* Need 0x4 */
#define LHV_USE_VPID				0x0000000000000010ULL
#define LHV_USE_UNRESTRICTED_GUEST	0x0000000000000020ULL	/* Need 0x4 */
#define LHV_USER_MODE				0x0000000000000040ULL	/* Need !0x2 */
#define LHV_USE_VMXOFF				0x0000000000000080ULL
#define LHV_USE_LARGE_PAGE			0x0000000000000100ULL	/* Need 0x4 */
#define LHV_NO_INTERRUPT			0x0000000000000200ULL
#define LHV_USE_MSRBITMAP			0x0000000000000400ULL
#define LHV_NESTED_USER_MODE		0x0000000000000800ULL	/* Need !0x2 */
#define LHV_USE_PS2_MOUSE			0x0000000000001000ULL
/* End of bit definitions for SHV_OPT */

/* lhv.c */
void lhv_main(VCPU *vcpu);
void handle_lhv_syscall(VCPU *vcpu, int vector, struct regs *r);

/* lhv-console.c */
void console_cursor_clear(void);
void console_clear(console_vc_t *vc);
char console_get_char(console_vc_t *vc, int x, int y);
void console_put_char(console_vc_t *vc, int x, int y, char c);
void console_get_vc(console_vc_t *vc, int num, int guest);

/* lhv-timer.c */
void timer_init(VCPU *vcpu);
void handle_timer_interrupt(VCPU *vcpu, int vector, int guest);

/* lhv-pic.c */
void pic_init(void);
int pic_spurious(unsigned char irq);

/* lhv-keyboard.c */
void handle_keyboard_interrupt(VCPU *vcpu, int vector, int guest);

/* lhv-mouse.c */
void mouse_init(VCPU *vcpu);
void handle_mouse_interrupt(VCPU *vcpu, int vector, int guest);

/* lhv-vmx.c */
void lhv_vmx_main(VCPU *vcpu);
void vmentry_error(ulong_t is_resume, ulong_t valid);

/* lhv-asm.S */
void vmexit_asm(void);				/* Called by hardware only */
void vmlaunch_asm(struct regs *r);	/* Never returns */
void vmresume_asm(struct regs *r);	/* Never returns */

/* lhv-ept.c */
extern u8 large_pages[2][512 * 4096] __attribute__((aligned(512 * 4096)));

/*
 * When this is larger than XMHF's VMX_NESTED_MAX_ACTIVE_EPT, should see a lot
 * of EPT cache misses.
 */
#define LHV_EPT_COUNT 2

void lhv_ept_init(VCPU *vcpu);
u64 lhv_build_ept(VCPU *vcpu, u8 ept_num);

/* lhv-vmcs.c */
// TODO: remove these, function, follow XMHF64 style
void vmcs_vmwrite(VCPU *vcpu, ulong_t encoding, ulong_t value);
void vmcs_vmwrite64(VCPU *vcpu, ulong_t encoding, u64 value);
ulong_t vmcs_vmread(VCPU *vcpu, ulong_t encoding);
u64 vmcs_vmread64(VCPU *vcpu, ulong_t encoding);
void vmcs_dump(VCPU *vcpu, int verbose);
void vmcs_load(VCPU *vcpu);

void __vmx_vmwrite16(u16 encoding, u16 value);
void __vmx_vmwrite64(u16 encoding, u64 value);
void __vmx_vmwrite32(u16 encoding, u32 value);
void __vmx_vmwriteNW(u16 encoding, ulong_t value);
u16 __vmx_vmread16(u16 encoding);
u64 __vmx_vmread64(u16 encoding);
u32 __vmx_vmread32(u16 encoding);
ulong_t __vmx_vmreadNW(u16 encoding);

/* lhv-guest-asm.S */
void lhv_guest_entry(void);
void lhv_guest_xcphandler(VCPU * vcpu, struct regs *r, iret_info_t * info);

/* lhv-user.c */
typedef struct ureg_t {
	struct regs r;
	uintptr_t eip;
	uintptr_t cs;
	uintptr_t eflags;
	uintptr_t esp;
	uintptr_t ss;
} ureg_t;
void enter_user_mode(VCPU *vcpu, ulong_t arg);
void user_main(VCPU *vcpu, ulong_t arg);

/* lhv-user-asm.S */
void enter_user_mode_asm(ureg_t *ureg, uintptr_t *esp0);
void exit_user_mode_asm(uintptr_t esp0);

/* LAPIC */
#define LAPIC_DEFAULT_BASE    0xfee00000
#define IOAPIC_DEFAULT_BASE   0xfec00000
#define LAPIC_EOI              0x0B0     /* EOI */
#define LAPIC_SVR              0x0F0     /* Spurious Interrupt Vector */
#define LAPIC_LVT_TIMER        0x320     /* Local Vector Table 0 (TIMER) */
#define LAPIC_TIMER_INIT       0x380     /* Timer Initial Count */
#define LAPIC_TIMER_CUR        0x390     /* Timer Current Count */
#define LAPIC_TIMER_DIV        0x3E0     /* Timer Divide Configuration */
#define LAPIC_ENABLE      0x00000100     /* Unit Enable */

static inline u32 read_lapic(u32 reg) {
	return *(volatile u32 *)(uintptr_t)(LAPIC_DEFAULT_BASE + reg);
}

static inline void write_lapic(u32 reg, u32 val) {
	*(volatile u32 *)(uintptr_t)(LAPIC_DEFAULT_BASE + reg) = val;
}

#endif /* !__ASSEMBLY__ */

#endif /* _LHV_H_ */

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
#include <shv.h>

void shv_main(VCPU * vcpu)
{
	console_vc_t vc     ;
	console_get_vc(&vc, vcpu->idx, 0);
	console_clear(&vc);
	if (vcpu->isbsp) {
		console_cursor_clear();
		pic_init();
		// asm volatile ("int $0xf8");
		if (0) {
			int *a = (int *)0xf0f0f0f0f0f0f0f0;
			printf("%d", *a);
		}
	}
	timer_init(vcpu);
	ASSERT(vc.height >= 2);
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < 2; j++) {
#ifndef __DEBUG_VGA__
			ASSERT(console_get_char(&vc, i, j) == ' ');
#endif							/* !__DEBUG_VGA__ */
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	/* Demonstrate disabling paging in hypervisor */
	if (SHV_OPT & SHV_USE_UNRESTRICTED_GUEST) {
#ifdef __amd64__
		extern void shv_disable_enable_paging(char *);
		shv_disable_enable_paging("SHV hypervisor can disable paging\n");
#elif defined(__i386__)
		ulong_t cr0 = read_cr0();
		write_cr0(cr0 & 0x7fffffffUL);
		printf("SHV hypervisor can disable paging\n");
		write_cr0(cr0);
#else							/* !defined(__i386__) && !defined(__amd64__) */
#error "Unsupported Arch"
#endif							/* !defined(__i386__) && !defined(__amd64__) */
	}

	if (!(SHV_OPT & SHV_NO_EFLAGS_IF)) {
		/* Set EFLAGS.IF */
		asm volatile ("sti");
	}

	if (SHV_OPT & SHV_USE_PS2_MOUSE) {
		if (vcpu->isbsp) {
			mouse_init(vcpu);
		}
	}

	/* Start VT related things */
	shv_vmx_main(vcpu);

	ASSERT(0 && "Should not reach here");
}

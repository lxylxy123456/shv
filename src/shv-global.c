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

u32 g_midtable_numentries;
PCPU g_cpumap[MAX_PCPU_ENTRIES];
MIDTAB g_midtable[MAX_VCPU_ENTRIES];
VCPU g_vcpus[MAX_VCPU_ENTRIES];
u8 g_cpu_stack[MAX_VCPU_ENTRIES][SHV_STACK_SIZE];
uintptr_t g_cr3;
uintptr_t g_cr4;
#ifdef __amd64__
u32 g_smp_lret_stack[2];
#endif							/* __amd64__ */

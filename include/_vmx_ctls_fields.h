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

/*
 * Macros are defined as DECLARE_CTLS
 * The arguments are:
 * * macro: macro name for bit location
 * * bit: bit location value
 * * func: function name
 * * field: field name in vmx_ctls_t
 */

/* External-interrupt exiting */
DECLARE_CTLS(PINBASED_EXTERNAL_INTERRUPT_EXITING,
			 0,
			 external_interrupt_exiting,
			 pinbased_ctls)

/* NMI exiting */
DECLARE_CTLS(PINBASED_NMI_EXITING,
			 3,
			 nmi_exiting,
			 pinbased_ctls)

/* Virtual NMIs */
DECLARE_CTLS(PINBASED_VIRTUAL_NMIS,
			 5,
			 virtual_nmis,
			 pinbased_ctls)

/* Activate VMX-preemption timer */
DECLARE_CTLS(PINBASED_ACTIVATE_VMX_PREEMPTION_TIMER,
			 6,
			 activate_vmx_preemption_timer,
			 pinbased_ctls)

/* Process posted interrupts */
DECLARE_CTLS(PINBASED_PROCESS_POSTED_INTERRUPTS,
			 7,
			 process_posted_interrupts,
			 pinbased_ctls)

/* Interrupt-window exiting */
DECLARE_CTLS(PROCBASED_INTERRUPT_WINDOW_EXITING,
			 2,
			 interrupt_window_exiting,
			 procbased_ctls)

/* Use TSC offsetting */
DECLARE_CTLS(PROCBASED_USE_TSC_OFFSETTING,
			 3,
			 use_tsc_offsetting,
			 procbased_ctls)

/* HLT exiting */
DECLARE_CTLS(PROCBASED_HLT_EXITING,
			 7,
			 hlt_exiting,
			 procbased_ctls)

/* INVLPG exiting */
DECLARE_CTLS(PROCBASED_INVLPG_EXITING,
			 9,
			 invlpg_exiting,
			 procbased_ctls)

/* MWAIT exiting */
DECLARE_CTLS(PROCBASED_MWAIT_EXITING,
			 10,
			 mwait_exiting,
			 procbased_ctls)

/* RDPMC exiting */
DECLARE_CTLS(PROCBASED_RDPMC_EXITING,
			 11,
			 rdpmc_exiting,
			 procbased_ctls)

/* RDTSC exiting */
DECLARE_CTLS(PROCBASED_RDTSC_EXITING,
			 12,
			 rdtsc_exiting,
			 procbased_ctls)

/* CR3-load exiting */
DECLARE_CTLS(PROCBASED_CR3_LOAD_EXITING,
			 15,
			 cr3_load_exiting,
			 procbased_ctls)

/* CR3-store exiting */
DECLARE_CTLS(PROCBASED_CR3_STORE_EXITING,
			 16,
			 cr3_store_exiting,
			 procbased_ctls)

/* Activate tertiary controls */
DECLARE_CTLS(PROCBASED_ACTIVATE_TERTIARY_CONTROLS,
			 17,
			 activate_tertiary_controls,
			 procbased_ctls)

/* CR8-load exiting */
DECLARE_CTLS(PROCBASED_CR8_LOAD_EXITING,
			 19,
			 cr8_load_exiting,
			 procbased_ctls)

/* CR8-store exiting */
DECLARE_CTLS(PROCBASED_CR8_STORE_EXITING,
			 20,
			 cr8_store_exiting,
			 procbased_ctls)

/* Use TPR shadow */
DECLARE_CTLS(PROCBASED_USE_TPR_SHADOW,
			 21,
			 use_tpr_shadow,
			 procbased_ctls)

/* NMI-window exiting */
DECLARE_CTLS(PROCBASED_NMI_WINDOW_EXITING,
			 22,
			 nmi_window_exiting,
			 procbased_ctls)

/* MOV-DR exiting */
DECLARE_CTLS(PROCBASED_MOV_DR_EXITING,
			 23,
			 mov_dr_exiting,
			 procbased_ctls)

/* Unconditional I/O exiting */
DECLARE_CTLS(PROCBASED_UNCONDITIONAL_IO_EXITING,
			 24,
			 unconditional_io_exiting,
			 procbased_ctls)

/* Use I/O bitmaps */
DECLARE_CTLS(PROCBASED_USE_IO_BITMAPS,
			 25,
			 use_io_bitmaps,
			 procbased_ctls)

/* Monitor trap flag */
DECLARE_CTLS(PROCBASED_MONITOR_TRAP_FLAG,
			 27,
			 monitor_trap_flag,
			 procbased_ctls)

/* Use MSR bitmaps */
DECLARE_CTLS(PROCBASED_USE_MSR_BITMAPS,
			 28,
			 use_msr_bitmaps,
			 procbased_ctls)

/* MONITOR exiting */
DECLARE_CTLS(PROCBASED_MONITOR_EXITING,
			 29,
			 monitor_exiting,
			 procbased_ctls)

/* PAUSE exiting */
DECLARE_CTLS(PROCBASED_PAUSE_EXITING,
			 30,
			 pause_exiting,
			 procbased_ctls)

/* Activate secondary controls */
DECLARE_CTLS(PROCBASED_ACTIVATE_SECONDARY_CONTROLS,
			 31,
			 activate_secondary_controls,
			 procbased_ctls)

/* Virtualize APIC accesses */
DECLARE_CTLS(SECPROCBASED_VIRTUALIZE_APIC_ACCESS,
			 0,
			 virtualize_apic_access,
			 procbased_ctls2)

/* Enable EPT */
DECLARE_CTLS(SECPROCBASED_ENABLE_EPT,
			 1,
			 enable_ept,
			 procbased_ctls2)

/* Descriptor-table exiting */
DECLARE_CTLS(SECPROCBASED_DESCRIPTOR_TABLE_EXITING,
			 2,
			 descriptor_table_exiting,
			 procbased_ctls2)

/* Enable RDTSCP */
DECLARE_CTLS(SECPROCBASED_ENABLE_RDTSCP,
			 3,
			 enable_rdtscp,
			 procbased_ctls2)

/* Virtualize x2APIC mode */
DECLARE_CTLS(SECPROCBASED_VIRTUALIZE_X2APIC_MODE,
			 4,
			 virtualize_x2apic_mode,
			 procbased_ctls2)

/* Enable VPID */
DECLARE_CTLS(SECPROCBASED_ENABLE_VPID,
			 5,
			 enable_vpid,
			 procbased_ctls2)

/* WBINVD exiting */
DECLARE_CTLS(SECPROCBASED_WBINVD_EXITING,
			 6,
			 wbinvd_exiting,
			 procbased_ctls2)

/* Unrestricted guest */
DECLARE_CTLS(SECPROCBASED_UNRESTRICTED_GUEST,
			 7,
			 unrestricted_guest,
			 procbased_ctls2)

/* APIC-register virtualization */
DECLARE_CTLS(SECPROCBASED_APIC_REGISTER_VIRTUALIZATION,
			 8,
			 apic_register_virtualization,
			 procbased_ctls2)

/* Virtual-interrupt delivery */
DECLARE_CTLS(SECPROCBASED_VIRTUAL_INTERRUPT_DELIVERY,
			 9,
			 virtual_interrupt_delivery,
			 procbased_ctls2)

/* PAUSE-loop exiting */
DECLARE_CTLS(SECPROCBASED_PAUSE_LOOP_EXITING,
			 10,
			 pause_loop_exiting,
			 procbased_ctls2)

/* RDRAND exiting */
DECLARE_CTLS(SECPROCBASED_RDRAND_EXITING,
			 11,
			 rdrand_exiting,
			 procbased_ctls2)

/* Enable INVPCID */
DECLARE_CTLS(SECPROCBASED_ENABLE_INVPCID,
			 12,
			 enable_invpcid,
			 procbased_ctls2)

/* Enable VM functions */
DECLARE_CTLS(SECPROCBASED_ENABLE_VM_FUNCTIONS,
			 13,
			 enable_vm_functions,
			 procbased_ctls2)

/* VMCS shadowing */
DECLARE_CTLS(SECPROCBASED_VMCS_SHADOWING,
			 14,
			 vmcs_shadowing,
			 procbased_ctls2)

/* Enable ENCLS exiting */
DECLARE_CTLS(SECPROCBASED_ENABLE_ENCLS_EXITING,
			 15,
			 enable_encls_exiting,
			 procbased_ctls2)

/* RDSEED exiting */
DECLARE_CTLS(SECPROCBASED_RDSEED_EXITING,
			 16,
			 rdseed_exiting,
			 procbased_ctls2)

/* Enable PML */
DECLARE_CTLS(SECPROCBASED_ENABLE_PML,
			 17,
			 enable_pml,
			 procbased_ctls2)

/* EPT-violation #VE */
DECLARE_CTLS(SECPROCBASED_EPT_VIOLATION_VE,
			 18,
			 ept_violation_ve,
			 procbased_ctls2)

/* Conceal VMX from PT */
DECLARE_CTLS(SECPROCBASED_CONCEAL_VMX_FROM_PT,
			 19,
			 conceal_vmx_from_pt,
			 procbased_ctls2)

/* Enable XSAVES/XRSTORS */
DECLARE_CTLS(SECPROCBASED_ENABLE_XSAVES_XRSTORS,
			 20,
			 enable_xsaves_xrstors,
			 procbased_ctls2)

/* Mode-based execute control for EPT */
DECLARE_CTLS(SECPROCBASED_MODE_BASED_EXECUTE_CONTROL_FOR_EPT,
			 22,
			 mode_based_execute_control_for_ept,
			 procbased_ctls2)

/* Sub-page write permissions for EPT */
DECLARE_CTLS(SECPROCBASED_SUB_PAGE_WRITE_PERMISSIONS_FOR_EPT,
			 23,
			 sub_page_write_permissions_for_ept,
			 procbased_ctls2)

/* Intel PT uses guest physical addresses */
DECLARE_CTLS(SECPROCBASED_INTEL_PT_USES_GUEST_PHYSICAL_ADDRESSES,
			 24,
			 intel_pt_uses_guest_physical_addresses,
			 procbased_ctls2)

/* Use TSC scaling */
DECLARE_CTLS(SECPROCBASED_USE_TSC_SCALING,
			 25,
			 use_tsc_scaling,
			 procbased_ctls2)

/* Enable user wait and pause */
DECLARE_CTLS(SECPROCBASED_ENABLE_USER_WAIT_AND_PAUSE,
			 26,
			 enable_user_wait_and_pause,
			 procbased_ctls2)

/* Enable ENCLV exiting */
DECLARE_CTLS(SECPROCBASED_ENABLE_ENCLV_EXITING,
			 28,
			 enable_enclv_exiting,
			 procbased_ctls2)

/* Save debug controls */
DECLARE_CTLS(VMEXIT_SAVE_DEBUG_CONTROLS,
			 2,
			 vmexit_save_debug_controls,
			 exit_ctls)

/* Host address-space size */
DECLARE_CTLS(VMEXIT_HOST_ADDRESS_SPACE_SIZE,
			 9,
			 vmexit_host_address_space_size,
			 exit_ctls)

/* Load IA32_PERF_GLOBAL_CTRL */
DECLARE_CTLS(VMEXIT_LOAD_IA32_PERF_GLOBAL_CTRL,
			 12,
			 vmexit_load_ia32_perf_global_ctrl,
			 exit_ctls)

/* Acknowledge interrupt on exit */
DECLARE_CTLS(VMEXIT_ACKNOWLEDGE_INTERRUPT_ON_EXIT,
			 15,
			 vmexit_acknowledge_interrupt_on_exit,
			 exit_ctls)

/* Save IA32_PAT */
DECLARE_CTLS(VMEXIT_SAVE_IA32_PAT,
			 18,
			 vmexit_save_ia32_pat,
			 exit_ctls)

/* Load IA32_PAT */
DECLARE_CTLS(VMEXIT_LOAD_IA32_PAT,
			 19,
			 vmexit_load_ia32_pat,
			 exit_ctls)

/* Save IA32_EFER */
DECLARE_CTLS(VMEXIT_SAVE_IA32_EFER,
			 20,
			 vmexit_save_ia32_efer,
			 exit_ctls)

/* Load IA32_EFER */
DECLARE_CTLS(VMEXIT_LOAD_IA32_EFER,
			 21,
			 vmexit_load_ia32_efer,
			 exit_ctls)

/* Save VMX-preemption timer value */
DECLARE_CTLS(VMEXIT_SAVE_VMX_PREEMPTION_TIMER_VALUE,
			 22,
			 vmexit_save_vmx_preemption_timer_value,
			 exit_ctls)

/* Clear IA32_BNDCFGS */
DECLARE_CTLS(VMEXIT_CLEAR_IA32_BNDCFGS,
			 23,
			 vmexit_clear_ia32_bndcfgs,
			 exit_ctls)

/* Conceal VMX from PT */
DECLARE_CTLS(VMEXIT_CONCEAL_VMX_FROM_PT,
			 24,
			 vmexit_conceal_vmx_from_pt,
			 exit_ctls)

/* Clear IA32_RTIT_CTL */
DECLARE_CTLS(VMEXIT_CLEAR_IA32_RTIT_CTL,
			 25,
			 vmexit_clear_ia32_rtit_ctl,
			 exit_ctls)

/* Load CET state */
DECLARE_CTLS(VMEXIT_LOAD_CET_STATE,
			 28,
			 vmexit_load_cet_state,
			 exit_ctls)

/* Load PKRS */
DECLARE_CTLS(VMEXIT_LOAD_PKRS,
			 29,
			 vmexit_load_pkrs,
			 exit_ctls)

/* Load debug controls */
DECLARE_CTLS(VMENTRY_LOAD_DEBUG_CONTROLS,
			 2,
			 vmentry_load_debug_controls,
			 entry_ctls)

/* IA-32e mode guest */
DECLARE_CTLS(VMENTRY_IA_32E_MODE_GUEST,
			 9,
			 vmentry_ia_32e_mode_guest,
			 entry_ctls)

/* Entry to SMM */
DECLARE_CTLS(VMENTRY_ENTRY_TO_SMM,
			 10,
			 vmentry_entry_to_smm,
			 entry_ctls)

/* Deactivate dual-monitor treatment */
DECLARE_CTLS(VMENTRY_DEACTIVATE_DUAL_MONITOR_TREATMENT,
			 11,
			 vmentry_deactivate_dual_monitor_treatment,
			 entry_ctls)

/* Load IA32_PERF_GLOBAL_CTRL */
DECLARE_CTLS(VMENTRY_LOAD_IA32_PERF_GLOBAL_CTRL,
			 13,
			 vmentry_load_ia32_perf_global_ctrl,
			 entry_ctls)

/* Load IA32_PAT */
DECLARE_CTLS(VMENTRY_LOAD_IA32_PAT,
			 14,
			 vmentry_load_ia32_pat,
			 entry_ctls)

/* Load IA32_EFER */
DECLARE_CTLS(VMENTRY_LOAD_IA32_EFER,
			 15,
			 vmentry_load_ia32_efer,
			 entry_ctls)

/* Load IA32_BNDCFGS */
DECLARE_CTLS(VMENTRY_LOAD_IA32_BNDCFGS,
			 16,
			 vmentry_load_ia32_bndcfgs,
			 entry_ctls)

/* Conceal VMX from PT */
DECLARE_CTLS(VMENTRY_CONCEAL_VMX_FROM_PT,
			 17,
			 vmentry_conceal_vmx_from_pt,
			 entry_ctls)

/* Load IA32_RTIT_CTL */
DECLARE_CTLS(VMENTRY_LOAD_IA32_RTIT_CTL,
			 18,
			 vmentry_load_ia32_rtit_ctl,
			 entry_ctls)

/* Load CET state */
DECLARE_CTLS(VMENTRY_LOAD_CET_STATE,
			 20,
			 vmentry_load_cet_state,
			 entry_ctls)

/* Load PKRS */
DECLARE_CTLS(VMENTRY_LOAD_PKRS,
			 22,
			 vmentry_load_pkrs,
			 entry_ctls)

#undef DECLARE_CTLS

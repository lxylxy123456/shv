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

#ifndef _SHV_OPTS_H_
#define _SHV_OPTS_H_

/*
 * SHV_OPT is used to configure miscellaneous behavior of SHV. Most
 * configuration options control which test to enable. We use this to avoid
 * adding too many configurations in configure.ac.
 */

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
#define SHV_NO_VGA_ART				0x0000000000002000ULL	/* Need !0x20 */
/* End of bit definitions for SHV_OPT */

/*
 * NMI_OPT is used to configure NMI testing SHV. Bit 0 of NMI_OPT enables NMI
 * testing. Once NMI testing is enabled, most bits in SHV_OPT are ignored. For
 * i > 0, Bit i enables NMI test i (see shv-nmi.c).
 */

/* Begin of bit definitions for NMI_OPT */
#define SHV_NMI_ENABLE				0x0000000000000001ULL
#define SHV_NMI_EXPERIMENT_01		0x0000000000000002ULL
#define SHV_NMI_EXPERIMENT_02		0x0000000000000004ULL
#define SHV_NMI_EXPERIMENT_03		0x0000000000000008ULL
#define SHV_NMI_EXPERIMENT_04		0x0000000000000010ULL
#define SHV_NMI_EXPERIMENT_05		0x0000000000000020ULL
#define SHV_NMI_EXPERIMENT_06		0x0000000000000040ULL
#define SHV_NMI_EXPERIMENT_07		0x0000000000000080ULL
#define SHV_NMI_EXPERIMENT_08		0x0000000000000100ULL
#define SHV_NMI_EXPERIMENT_09		0x0000000000000200ULL
#define SHV_NMI_EXPERIMENT_10		0x0000000000000400ULL
#define SHV_NMI_EXPERIMENT_11		0x0000000000000800ULL
#define SHV_NMI_EXPERIMENT_12		0x0000000000001000ULL
#define SHV_NMI_EXPERIMENT_13		0x0000000000002000ULL
#define SHV_NMI_EXPERIMENT_14		0x0000000000004000ULL
#define SHV_NMI_EXPERIMENT_15		0x0000000000008000ULL
#define SHV_NMI_EXPERIMENT_16		0x0000000000010000ULL
#define SHV_NMI_EXPERIMENT_17		0x0000000000020000ULL
#define SHV_NMI_EXPERIMENT_18		0x0000000000040000ULL
#define SHV_NMI_EXPERIMENT_19		0x0000000000080000ULL
#define SHV_NMI_EXPERIMENT_20		0x0000000000100000ULL
#define SHV_NMI_EXPERIMENT_21		0x0000000000200000ULL
#define SHV_NMI_EXPERIMENT_22		0x0000000000400000ULL
#define SHV_NMI_EXPERIMENT_23		0x0000000000800000ULL
#define SHV_NMI_EXPERIMENT_24		0x0000000001000000ULL
#define SHV_NMI_EXPERIMENT_25		0x0000000002000000ULL
#define SHV_NMI_EXPERIMENT_26		0x0000000004000000ULL
#define SHV_NMI_EXPERIMENT_27		0x0000000008000000ULL
#define SHV_NMI_EXPERIMENT_28		0x0000000010000000ULL
#define SHV_NMI_EXPERIMENT_29		0x0000000020000000ULL
#define SHV_NMI_EXPERIMENT_30		0x0000000040000000ULL
#define SHV_NMI_EXPERIMENT_31		0x0000000080000000ULL
#define SHV_NMI_EXPERIMENT_32		0x0000000100000000ULL
#define SHV_NMI_EXPERIMENT_33		0x0000000200000000ULL
#define SHV_NMI_EXPERIMENT_34		0x0000000400000000ULL
#define SHV_NMI_EXPERIMENT_35		0x0000000800000000ULL
#define SHV_NMI_EXPERIMENT_36		0x0000001000000000ULL
#define SHV_NMI_EXPERIMENT_37		0x0000002000000000ULL
#define SHV_NMI_EXPERIMENT_38		0x0000004000000000ULL
#define SHV_NMI_EXPERIMENT_39		0x0000008000000000ULL
#define SHV_NMI_EXPERIMENT_40		0x0000010000000000ULL
#define SHV_NMI_EXPERIMENT_41		0x0000020000000000ULL
#define SHV_NMI_EXPERIMENT_42		0x0000040000000000ULL
#define SHV_NMI_EXPERIMENT_43		0x0000080000000000ULL
#define SHV_NMI_EXPERIMENT_44		0x0000100000000000ULL
#define SHV_NMI_EXPERIMENT_45		0x0000200000000000ULL
#define SHV_NMI_EXPERIMENT_46		0x0000400000000000ULL
#define SHV_NMI_EXPERIMENT_47		0x0000800000000000ULL
#define SHV_NMI_EXPERIMENT_48		0x0001000000000000ULL
#define SHV_NMI_EXPERIMENT_49		0x0002000000000000ULL
#define SHV_NMI_EXPERIMENT_50		0x0004000000000000ULL
#define SHV_NMI_EXPERIMENT_51		0x0008000000000000ULL
#define SHV_NMI_EXPERIMENT_52		0x0010000000000000ULL
#define SHV_NMI_EXPERIMENT_53		0x0020000000000000ULL
#define SHV_NMI_EXPERIMENT_54		0x0040000000000000ULL
#define SHV_NMI_EXPERIMENT_55		0x0080000000000000ULL
#define SHV_NMI_EXPERIMENT_56		0x0100000000000000ULL
#define SHV_NMI_EXPERIMENT_57		0x0200000000000000ULL
#define SHV_NMI_EXPERIMENT_58		0x0400000000000000ULL
#define SHV_NMI_EXPERIMENT_59		0x0800000000000000ULL
#define SHV_NMI_EXPERIMENT_60		0x1000000000000000ULL
#define SHV_NMI_EXPERIMENT_61		0x2000000000000000ULL
#define SHV_NMI_EXPERIMENT_62		0x4000000000000000ULL
#define SHV_NMI_EXPERIMENT_63		0x8000000000000000ULL
/* End of bit definitions for NMI_OPT */

#endif							/* _SHV_OPTS_H_ */

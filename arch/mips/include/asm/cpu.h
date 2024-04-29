/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * cpu.h: Values of the PRId register used to match up
 *        various MIPS cpu types.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 2004  Maciej W. Rozycki
 */
#ifndef _ASM_CPU_H
#define _ASM_CPU_H

/* Assigned Company values for bits 23:16 of the PRId Register
   (CP0 register 15, select 0).  As of the MIPS32 and MIPS64 specs from
   MTI, the PRId register is defined in this (backwards compatible)
   way:

  +----------------+----------------+----------------+----------------+
  | Company Options| Company ID     | Processor ID   | Revision       |
  +----------------+----------------+----------------+----------------+
   31            24 23            16 15             8 7

   I don't have docs for all the previous processors, but my impression is
   that bits 16-23 have been 0 for all MIPS processors before the MIPS32/64
   spec.
*/

#define PRID_COMP_LEGACY	0x000000
#define PRID_COMP_MIPS		0x010000
#define PRID_COMP_BROADCOM	0x020000
#define PRID_COMP_INGENIC	0xd00000
#define PRID_COMP_INGENIC2	0xe10000

/*
 * Assigned Processor ID (implementation) values for bits 15:8 of the PRId
 * register.  In order to detect a certain CPU type exactly eventually
 * additional registers may need to be examined.
 */

#define PRID_IMP_MASK		0xff00

/*
 * These are valid when 23:16 == PRID_COMP_LEGACY
 */

#define PRID_IMP_GS232	0x4200

#define PRID_IMP_UNKNOWN	0xff00

/*
 * These are the PRID's for when 23:16 == PRID_COMP_MIPS
 */

#define PRID_IMP_QEMU_GENERIC	0x0000
#define PRID_IMP_4KC		0x8000
#define PRID_IMP_5KC		0x8100
#define PRID_IMP_20KC		0x8200
#define PRID_IMP_4KEC		0x8400
#define PRID_IMP_4KSC		0x8600
#define PRID_IMP_25KF		0x8800
#define PRID_IMP_5KE		0x8900
#define PRID_IMP_4KECR2		0x9000
#define PRID_IMP_4KEMPR2	0x9100
#define PRID_IMP_4KSD		0x9200
#define PRID_IMP_24K		0x9300
#define PRID_IMP_34K		0x9500
#define PRID_IMP_24KE		0x9600
#define PRID_IMP_74K		0x9700
#define PRID_IMP_1004K		0x9900
#define PRID_IMP_1074K		0x9a00
#define PRID_IMP_M14KC		0x9c00
#define PRID_IMP_M14KEC		0x9e00

/*
 * These are the PRID's for when 23:16 == PRID_COMP_BROADCOM
 */

#define PRID_IMP_BMIPS3300	0x9000

/*
 * These are the PRID's for when 23:16 == PRID_COMP_INGENIC
 */

#define PRID_IMP_JZRISC		0x0200

/*
 * Particular Revision values for bits 7:0 of the PRId register.
 */

#define PRID_REV_MASK		0x00ff

/*
 * Definitions for 7:0 on legacy processors
 */

#define PRID_REV_LOONGSON1B	0x0020

/*
 * Older processors used to encode processor version and revision in two
 * 4-bit bitfields, the 4K seems to simply count up and even newer MTI cores
 * have switched to use the 8-bits as 3:3:2 bitfield with the last field as
 * the patch number.  *ARGH*
 */
#define PRID_REV_ENCODE_44(ver, rev)					\
	((ver) << 4 | (rev))
#define PRID_REV_ENCODE_332(ver, rev, patch)				\
	((ver) << 5 | (rev) << 2 | (patch))

/*
 * FPU implementation/revision register (CP1 control register 0).
 *
 * +---------------------------------+----------------+----------------+
 * | 0                               | Implementation | Revision       |
 * +---------------------------------+----------------+----------------+
 *  31                             16 15             8 7              0
 */

#define FPIR_IMP_NONE		0x0000

enum cpu_type_enum {
	CPU_UNKNOWN,

	/*
	 * MIPS32 class processors
	 */
	CPU_4KC, CPU_4KEC, CPU_4KSC, CPU_24K, CPU_34K, CPU_1004K, CPU_74K,
	CPU_M14KC, CPU_M14KEC, CPU_1074K, CPU_BMIPS3300, CPU_JZRISC, CPU_GS232,

	/*
	 * MIPS64 class processors
	 */
	CPU_5KC, CPU_5KE, CPU_20KC, CPU_25KF,

	CPU_QEMU_GENERIC,

	CPU_LAST
};

/*
 * ISA Level encodings
 *
 */
#define MIPS_CPU_ISA_I		0x00000001
#define MIPS_CPU_ISA_II		0x00000002
#define MIPS_CPU_ISA_III	0x00000004
#define MIPS_CPU_ISA_IV		0x00000008
#define MIPS_CPU_ISA_V		0x00000010
#define MIPS_CPU_ISA_M32R1	0x00000020
#define MIPS_CPU_ISA_M32R2	0x00000040
#define MIPS_CPU_ISA_M64R1	0x00000080
#define MIPS_CPU_ISA_M64R2	0x00000100

#define MIPS_CPU_ISA_32BIT (MIPS_CPU_ISA_I | MIPS_CPU_ISA_II | \
	MIPS_CPU_ISA_M32R1 | MIPS_CPU_ISA_M32R2 )
#define MIPS_CPU_ISA_64BIT (MIPS_CPU_ISA_III | MIPS_CPU_ISA_IV | \
	MIPS_CPU_ISA_V | MIPS_CPU_ISA_M64R1 | MIPS_CPU_ISA_M64R2)

/*
 * CPU Option encodings
 */
#define MIPS_CPU_TLB		0x00000001 /* CPU has TLB */
#define MIPS_CPU_4KEX		0x00000002 /* "R4K" exception model */
#define MIPS_CPU_3K_CACHE	0x00000004 /* R3000-style caches */
#define MIPS_CPU_4K_CACHE	0x00000008 /* R4000-style caches */
#define MIPS_CPU_TX39_CACHE	0x00000010 /* TX3900-style caches */
#define MIPS_CPU_FPU		0x00000020 /* CPU has FPU */
#define MIPS_CPU_32FPR		0x00000040 /* 32 dbl. prec. FP registers */
#define MIPS_CPU_COUNTER	0x00000080 /* Cycle count/compare */
#define MIPS_CPU_WATCH		0x00000100 /* watchpoint registers */
#define MIPS_CPU_DIVEC		0x00000200 /* dedicated interrupt vector */
#define MIPS_CPU_VCE		0x00000400 /* virt. coherence conflict possible */
#define MIPS_CPU_CACHE_CDEX_P	0x00000800 /* Create_Dirty_Exclusive CACHE op */
#define MIPS_CPU_CACHE_CDEX_S	0x00001000 /* ... same for seconary cache ... */
#define MIPS_CPU_MCHECK		0x00002000 /* Machine check exception */
#define MIPS_CPU_EJTAG		0x00004000 /* EJTAG exception */
#define MIPS_CPU_NOFPUEX	0x00008000 /* no FPU exception */
#define MIPS_CPU_LLSC		0x00010000 /* CPU has ll/sc instructions */
#define MIPS_CPU_INCLUSIVE_CACHES	0x00020000 /* P-cache subset enforced */
#define MIPS_CPU_PREFETCH	0x00040000 /* CPU has usable prefetch */
#define MIPS_CPU_VINT		0x00080000 /* CPU supports MIPSR2 vectored interrupts */
#define MIPS_CPU_VEIC		0x00100000 /* CPU supports MIPSR2 external interrupt controller mode */
#define MIPS_CPU_ULRI		0x00200000 /* CPU has ULRI feature */
#define MIPS_CPU_CP2		0x00400000 /* CPU has CP2 */

/*
 * CPU ASE encodings
 */
#define MIPS_ASE_MIPS16		0x00000001 /* code compression */
#define MIPS_ASE_MDMX		0x00000002 /* MIPS digital media extension */
#define MIPS_ASE_MIPS3D		0x00000004 /* MIPS-3D */
#define MIPS_ASE_SMARTMIPS	0x00000008 /* SmartMIPS */
#define MIPS_ASE_DSP		0x00000010 /* Signal Processing ASE */
#define MIPS_ASE_MIPSMT		0x00000020 /* CPU supports MIPS MT */

#endif /* _ASM_CPU_H */

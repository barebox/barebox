/*
 * Copyright (C) 2011 Tobias Klauser <tklauser@distanz.ch>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_NIOS2_ELF_H
#define _ASM_NIOS2_ELF_H

/* Relocation types */
#define R_NIOS2_NONE		0
#define R_NIOS2_S16		1
#define R_NIOS2_U16		2
#define R_NIOS2_PCREL16		3
#define R_NIOS2_CALL26		4
#define R_NIOS2_IMM5		5
#define R_NIOS2_CACHE_OPX	6
#define R_NIOS2_IMM6		7
#define R_NIOS2_IMM8		8
#define R_NIOS2_HI16		9
#define R_NIOS2_LO16		10
#define R_NIOS2_HIADJ16		11
#define R_NIOS2_BFD_RELOC_32	12
#define R_NIOS2_BFD_RELOC_16	13
#define R_NIOS2_BFD_RELOC_8	14
#define R_NIOS2_GPREL		15
#define R_NIOS2_GNU_VTINHERIT	16
#define R_NIOS2_GNU_VTENTRY	17
#define R_NIOS2_UJMP		18
#define R_NIOS2_CJMP		19
#define R_NIOS2_CALLR		20
#define R_NIOS2_ALIGN		21
/* Keep this the last entry.  */
#define R_NIOS2_NUM		22

#include <asm/ptrace.h>

typedef unsigned long elf_greg_t;

#define ELF_NGREG (sizeof(struct pt_regs) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef unsigned long elf_fpregset_t;

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x)->e_machine == EM_ALTERA_NIOS2)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS  ELFCLASS32
#define ELF_DATA   ELFDATA2LSB
#define ELF_ARCH   EM_ALTERA_NIOS2

#define ELF_PLAT_INIT(_r, load_addr)

#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE 4096

/* This is the location that an ET_DYN program is loaded if exec'ed.  Typical
   use of this is to invoke "./ld.so someprog" to test out a new version of
   the loader.  We need to make sure that it is out of the way of the program
   that it will "exec", and that there is sufficient room for the brk.  */

#define ELF_ET_DYN_BASE         0xD0000000UL

/* regs is struct pt_regs, pr_reg is elf_gregset_t (which is
   now struct_user_regs, they are different) */

#ifdef CONFIG_MMU
#define ELF_CORE_COPY_REGS(pr_reg, regs)                                             \
	do {                                                                         \
		pr_reg[0]  = regs->r8;                                               \
		pr_reg[1]  = regs->r9;                                               \
		pr_reg[2]  = regs->r10;                                              \
		pr_reg[3]  = regs->r11;                                              \
		pr_reg[4]  = regs->r12;                                              \
		pr_reg[5]  = regs->r13;                                              \
		pr_reg[6]  = regs->r14;                                              \
		pr_reg[7]  = regs->r15;                                              \
		pr_reg[8]  = regs->r1;                                               \
		pr_reg[9]  = regs->r2;                                               \
		pr_reg[10] = regs->r3;                                               \
		pr_reg[11] = regs->r4;                                               \
		pr_reg[12] = regs->r5;                                               \
		pr_reg[13] = regs->r6;                                               \
		pr_reg[14] = regs->r7                                                \
		pr_reg[15] = regs->orig_r2;                                          \
		pr_reg[16] = regs->ra;                                               \
		pr_reg[17] = regs->fp;                                               \
		pr_reg[18] = regs->sp;                                               \
		pr_reg[19] = regs->gp;                                               \
		pr_reg[20] = regs->estatus;                                          \
		pr_reg[21] = regs->ea;                                               \
		pr_reg[22] = regs->orig_r7;                                          \
			{                                                            \
			struct switch_stack *sw = ((struct switch_stack *)regs) - 1; \
			pr_reg[23] = sw->r16;                                        \
			pr_reg[24] = sw->r17;                                        \
			pr_reg[25] = sw->r18;                                        \
			pr_reg[26] = sw->r19;                                        \
			pr_reg[27] = sw->r20;                                        \
			pr_reg[28] = sw->r21;                                        \
			pr_reg[29] = sw->r22;                                        \
			pr_reg[30] = sw->r23;                                        \
			pr_reg[31] = sw->fp;                                         \
			pr_reg[32] = sw->gp;                                         \
			pr_reg[33] = sw->ra;                                         \
			}                                                            \
	} while (0)
#else
#define ELF_CORE_COPY_REGS(pr_reg, regs)                                             \
	do {                                                                         \
		pr_reg[0] = regs->r1;                                                \
		pr_reg[1] = regs->r2;                                                \
		pr_reg[2] = regs->r3;                                                \
		pr_reg[3] = regs->r4;                                                \
		pr_reg[4] = regs->r5;                                                \
		pr_reg[5] = regs->r6;                                                \
		pr_reg[6] = regs->r7;                                                \
		pr_reg[7] = regs->r8;                                                \
		pr_reg[8] = regs->r9;                                                \
		pr_reg[9] = regs->r10;                                               \
		pr_reg[10] = regs->r11;                                              \
		pr_reg[11] = regs->r12;                                              \
		pr_reg[12] = regs->r13;                                              \
		pr_reg[13] = regs->r14;                                              \
		pr_reg[14] = regs->r15;                                              \
		pr_reg[23] = regs->sp;                                               \
		pr_reg[26] = regs->estatus;                                          \
		{                                                                    \
			struct switch_stack *sw = ((struct switch_stack *)regs) - 1; \
			pr_reg[15] = sw->r16;                                        \
			pr_reg[16] = sw->r17;                                        \
			pr_reg[17] = sw->r18;                                        \
			pr_reg[18] = sw->r19;                                        \
			pr_reg[19] = sw->r20;                                        \
			pr_reg[20] = sw->r21;                                        \
			pr_reg[21] = sw->r22;                                        \
			pr_reg[22] = sw->r23;                                        \
			pr_reg[24] = sw->fp;                                         \
			pr_reg[25] = sw->gp;                                         \
		}                                                                    \
	} while (0)

#endif /* CONFIG_MMU */

/* This yields a mask that user programs can use to figure out what
   instruction set this cpu supports.  */

#define ELF_HWCAP (0)

/* This yields a string that ld.so will use to load implementation
   specific libraries for optimization.  This is more specific in
   intent than poking at uname or /proc/cpuinfo.  */

#define ELF_PLATFORM  (NULL)

#define SET_PERSONALITY(ex) set_personality(PER_LINUX_32BIT)

#endif

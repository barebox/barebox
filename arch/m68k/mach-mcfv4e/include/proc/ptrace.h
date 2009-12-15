/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Declaration and defines for M68k register frames
 */
#ifndef __ASM_PROC_PTRACE_H
#define __ASM_PROC_PTRACE_H

#define TRACE_FLAG      0x8000
#define SVR_MODE        0x2000
#define MODE_MASK	0x2000
#define MASTER_FLAG     0x1000
#define IRQ_MASK        0x0700
#define CC_MASK         0x00FF

#define CC_X_BIT        0x0010
#define CC_N_BIT        0x0008
#define CC_Z_BIT        0x0004
#define CC_V_BIT        0x0002
#define CC_C_BIT        0x0001

#define PCMASK          0x0

#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the
   stack during a system call. */

struct pt_regs {
	long uregs[37];
};
#define M68K_sp    uregs[37]
#define M68K_sr    uregs[36]
#define M68K_pc    uregs[35]
#define M68K_fpiar uregs[34]
#define M68K_fpsr  uregs[33]
#define M68K_fpcr  uregs[32]

#define M68K_fp7   uregs[30]
#define M68K_fp6   uregs[28]
#define M68K_fp5   uregs[26]
#define M68K_fp4   uregs[24]
#define M68K_fp3   uregs[22]
#define M68K_fp2   uregs[20]
#define M68K_fp1   uregs[18]
#define M68K_fp0   uregs[16]

#define M68K_a7    uregs[15]
#define M68K_a6    uregs[14]
#define M68K_a5    uregs[13]
#define M68K_a4    uregs[12]
#define M68K_a3    uregs[11]
#define M68K_a2    uregs[10]
#define M68K_a1    uregs[ 9]
#define M68K_a0    uregs[ 8]
#define M68K_d7    uregs[ 7]
#define M68K_d6    uregs[ 6]
#define M68K_d5    uregs[ 5]
#define M68K_d4    uregs[ 4]
#define M68K_d3    uregs[ 3]
#define M68K_d2    uregs[ 2]
#define M68K_d1    uregs[ 1]
#define M68K_d0    uregs[ 0]


#ifdef __KERNEL__

#define user_mode(regs)	\
	(((regs)->M68K_sr & SVR_MODE) == 0)

#define processor_mode(regs) \
	((regs)->M68K_sr & SVR_MODE)

#define interrupts_enabled(regs) \
	(!((regs)->M68K_sr & IRQ_MASK))

#define condition_codes(regs) \
	((regs)->M68K_sr & CC_MASK)

/* Are the current registers suitable for user mode?
 * (used to maintain security in signal handlers)
 */
static inline int valid_user_regs(struct pt_regs *regs)
{
	if ((regs->M68K_sr & SVR_MODE) == 0 &&
	    (regs->M68K_sr & IRQ_MASK) == 7)
		return 1;

	/*
	 * Force SR to something logical...
	 */
	regs->M68K_sr &= ~(CC_MASK);

	return 0;
}

#endif	/* __KERNEL__ */

#endif	/* __ASSEMBLY__ */

#endif

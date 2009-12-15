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
 *  Remains of the pthread stuff...
 * @todo Rework these headers....
 */
#ifndef __ASM_M68K_PTRACE_H
#define __ASM_M68K_PTRACE_H

#define PTRACE_GETREGS		12
#define PTRACE_SETREGS		13
#define PTRACE_GETFPREGS	14
#define PTRACE_SETFPREGS	15

#define PTRACE_SETOPTIONS	21


#include <proc/ptrace.h>

#ifndef __ASSEMBLY__

#ifndef PS_S
#define PS_S  (0x2000)
#define PS_M  (0x1000)
#endif

//#define user_mode(regs) (!((regs)->sr & PS_S))
#define instruction_pointer(regs) ((regs)->M68K_pc)
#define profile_pc(regs)          instruction_pointer(regs)

#ifdef __KERNEL__
extern void show_regs(struct pt_regs *);
#endif

#endif /* __ASSEMBLY__ */

#endif

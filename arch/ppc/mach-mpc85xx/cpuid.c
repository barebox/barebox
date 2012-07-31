/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 *
 * This file is derived from arch/powerpc/cpu/mpc85xx/cpu.c and
 * arch/powerpc/cpu/mpc86xx/cpu.c. Basically this file contains
 * cpu specific common code for 85xx/86xx processors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <mach/immap_85xx.h>

struct cpu_type cpu_type_list[] = {
	CPU_TYPE_ENTRY(P2020, P2020, 2),
	CPU_TYPE_ENTRY(P2020, P2020_E, 2),
};

struct cpu_type cpu_type_unknown = CPU_TYPE_ENTRY(Unknown, Unknown, 1);

struct cpu_type *identify_cpu(u32 ver)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(cpu_type_list); i++) {
		if (cpu_type_list[i].soc_ver == ver)
			return &cpu_type_list[i];
	}
	return &cpu_type_unknown;
}

int fsl_cpu_numcores(void)
{
	void __iomem *pic = (void __iomem *)MPC8xxx_PIC_ADDR;
	struct cpu_type *cpu;
	uint svr;
	uint ver;
	int tmp;

	svr = get_svr();
	ver = SVR_SOC_VER(svr);
	cpu = identify_cpu(ver);

	/* better to query feature reporting register than just assume 1 */
	if (cpu == &cpu_type_unknown) {
		tmp = in_be32(pic + MPC85xx_PIC_FRR_OFFSET);
		tmp = (tmp & MPC8xxx_PICFRR_NCPU_MASK) >>
			MPC8xxx_PICFRR_NCPU_SHIFT;
		tmp += 1;
	} else {
		tmp = cpu->num_cores;
	}

	return tmp;
}

/*
 * Copyright 2012 GE Intelligent Platforms, Inc
 * Copyright 2004,2007-2011 Freescale Semiconductor, Inc.
 * (C) Copyright 2002, 2003 Motorola Inc.
 * Xianghua Xiao (X.Xiao@motorola.com)
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
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
#include <asm/fsl_ddr_sdram.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>

void __noreturn reset_cpu(unsigned long addr)
{
	void __iomem *regs = (void __iomem *)MPC85xx_GUTS_ADDR;

	/* Everything after the first generation of PQ3 parts has RSTCR */
	out_be32(regs + MPC85xx_GUTS_RSTCR_OFFSET, 0x2);  /* HRESET_REQ */
	udelay(100);

	while (1)
		;
}

long int initdram(int board_type)
{
	phys_size_t dram_size = 0;

	dram_size = fixed_sdram();

	dram_size = e500_setup_ddr_tlbs(dram_size / 0x100000);
	dram_size *= 0x100000;

	return dram_size;
}

/*
 * Return the memory size based on the configuration registers.
 */
phys_size_t fsl_get_effective_memsize(void)
{
	void __iomem *regs = (void __iomem *)(MPC85xx_DDR_ADDR);
	phys_size_t sdram_size;
	uint san , ean;
	uint reg;
	int ix;

	sdram_size = 0;

	for (ix = 0; ix < CFG_CHIP_SELECTS_PER_CTRL; ix++) {
		if (in_be32(regs + DDR_OFF(CS0_CONFIG) + (ix * 8)) &
				SDRAM_CFG_MEM_EN) {
			reg = in_be32(regs + DDR_OFF(CS0_BNDS) + (ix * 8));
			/* start address */
			san = (reg & 0x0fff00000) >>  16;
			/* end address   */
			ean = (reg & 0x00000fff);
			sdram_size =  ((ean - san + 1) << 24);
		}
	}

	return sdram_size;
}

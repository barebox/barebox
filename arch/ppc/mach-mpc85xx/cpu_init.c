/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * Copyright 2007-2011 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2003 Motorola Inc.
 * Modified by Xianghua Xiao, X.Xiao@motorola.com
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

#include <common.h>
#include <init.h>
#include <asm/processor.h>
#include <asm/fsl_law.h>
#include <mach/mpc85xx.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>

static void fsl_setup_ccsrbar(void)
{
	u32 temp;
	u32 mas0, mas1, mas2, mas3, mas7;
	u32 *ccsr_virt = (u32 *)(CFG_CCSRBAR + 0x1000);

	mas0 = MAS0_TLBSEL(0) | MAS0_ESEL(1);
	mas1 = MAS1_VALID | MAS1_TID(0) | MAS1_TS | MAS1_TSIZE(BOOKE_PAGESZ_4K);
	mas2 = FSL_BOOKE_MAS2(CFG_CCSRBAR + 0x1000, MAS2_I|MAS2_G);
	mas3 = FSL_BOOKE_MAS3(CFG_CCSRBAR_DEFAULT, 0, MAS3_SW|MAS3_SR);
	mas7 = FSL_BOOKE_MAS7(CFG_CCSRBAR_DEFAULT);

	e500_write_tlb(mas0, mas1, mas2, mas3, mas7);

	temp = in_be32(ccsr_virt);
	out_be32(ccsr_virt, CFG_CCSRBAR_PHYS >> 12);
	temp = in_be32((u32 *)CFG_CCSRBAR);
}

int fsl_l2_cache_init(void)
{
	void __iomem *l2cache = (void __iomem *)MPC85xx_L2_ADDR;
	uint cache_ctl;
	uint svr, ver;
	u32 l2siz_field;

	svr = get_svr();
	ver = SVR_SOC_VER(svr);

	asm("msync;isync");
	cache_ctl = in_be32(l2cache + MPC85xx_L2_CTL_OFFSET);

	l2siz_field = (cache_ctl >> 28) & 0x3;

	switch (l2siz_field) {
	case 0x0:
		return -1;
		break;
	case 0x1:
		cache_ctl = 0xc0000000; /* set L2E=1, L2I=1, L2SRAM=0 */
		break;
	case 0x2:
		/* set L2E=1, L2I=1, & L2SRAM=0 */
		cache_ctl = 0xc0000000;
		break;
	case 0x3:
		/* set L2E=1, L2I=1, & L2SRAM=0 */
		cache_ctl = 0xc0000000;
		break;
	}

	if (!(in_be32(l2cache + MPC85xx_L2_CTL_OFFSET) & MPC85xx_L2CTL_L2E)) {
		asm("msync;isync");
		/* invalidate & enable */
		out_be32(l2cache + MPC85xx_L2_CTL_OFFSET, cache_ctl);
		asm("msync;isync");
	}

	return 0;
}

void cpu_init_early_f(void)
{
	u32 mas0, mas1, mas2, mas3, mas7;

	mas0 = MAS0_TLBSEL(0) | MAS0_ESEL(0);
	mas1 = MAS1_VALID | MAS1_TID(0) | MAS1_TS | MAS1_TSIZE(BOOKE_PAGESZ_4K);
	mas2 = FSL_BOOKE_MAS2(CFG_CCSRBAR, MAS2_I|MAS2_G);
	mas3 = FSL_BOOKE_MAS3(CFG_CCSRBAR_PHYS, 0, MAS3_SW|MAS3_SR);
	mas7 = FSL_BOOKE_MAS7(CFG_CCSRBAR_PHYS);

	e500_write_tlb(mas0, mas1, mas2, mas3, mas7);

	/* set up CCSR if we want it moved */
	if (CFG_CCSRBAR_DEFAULT != CFG_CCSRBAR_PHYS)
		fsl_setup_ccsrbar();

	fsl_init_laws();
	e500_invalidate_tlb(0);
	e500_init_tlbs();
}

static int cpu_init_r(void)
{
	e500_disable_tlb(14);
	e500_disable_tlb(15);

	return 0;
}
core_initcall(cpu_init_r);

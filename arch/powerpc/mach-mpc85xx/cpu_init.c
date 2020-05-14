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
#include <asm/cache.h>
#include <asm/fsl_law.h>
#include <asm/fsl_ifc.h>
#include <mach/mpc85xx.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>

/* NOR workaround for P1010 erratum A003399 */
#if defined(CONFIG_FSL_ERRATUM_P1010_A003549)
#define SRAM_BASE_ADDR 0x100
void setup_ifc(void)
{
	u32 mas0, mas1, mas2, mas3, mas7;
	phys_addr_t flash_phys = CFG_FLASH_BASE_PHYS;

	/*
	 * Adjust the TLB we were running out of to match the phys addr of the
	 * chip select we are adjusting and will return to.
	 */
	flash_phys += (~CFG_IFC_AMASK0) + 1 - 4*1024*1024;
	mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(15);
	mas1 = MAS1_VALID | MAS1_TID(0) | MAS1_TS | MAS1_IPROT |
			MAS1_TSIZE(BOOKE_PAGESZ_4M);
	mas2 = FSL_BOOKE_MAS2(CONFIG_TEXT_BASE, MAS2_I|MAS2_G);
	mas3 = FSL_BOOKE_MAS3(flash_phys, 0, MAS3_SW|MAS3_SR|MAS3_SX);
	mas7 = FSL_BOOKE_MAS7(flash_phys);
	mtspr(MAS0, mas0);
	mtspr(MAS1, mas1);
	mtspr(MAS2, mas2);
	mtspr(MAS3, mas3);
	mtspr(MAS7, mas7);
	asm volatile("isync;msync;tlbwe;isync");

#if defined(PPC_E500_DEBUG_TLB)
	/*
	 * TLB entry for debuggging in AS1
	 * Create temporary TLB entry in AS0 to handle debug exception
	 * As on debug exception MSR is cleared i.e. Address space is
	 * changed to 0. A TLB entry (in AS0) is required to handle
	 * debug exception generated * in AS1.
	 *
	 * TLB entry is created for IVPR + IVOR15 to map on valid OP
	 * code address because flash's physical address is going to
	 * change as CFG_FLASH_BASE_PHYS.
	 */
	mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(PPC_E500_DEBUG_TLB);
	mas1 = MAS1_VALID | MAS1_TID(0) | MAS1_IPROT |
	MAS1_TSIZE(BOOKE_PAGESZ_4M);
	mas2 = FSL_BOOKE_MAS2(CONFIG_TEXT_BASE, MAS2_I|MAS2_G);
	mas3 = FSL_BOOKE_MAS3(flash_phys, 0, MAS3_SW|MAS3_SR|MAS3_SX);
	mas7 = FSL_BOOKE_MAS7(flash_phys);

	mtspr(MAS0, mas0);
	mtspr(MAS1, mas1);
	mtspr(MAS2, mas2);
	mtspr(MAS3, mas3);
	mtspr(MAS7, mas7);

	asm volatile("isync;msync;tlbwe;isync");
#endif
	set_ifc_cspr(0, CFG_IFC_CSPR0);
	set_ifc_csor(0, CFG_IFC_CSOR0);
	set_ifc_amask(0, CFG_IFC_AMASK0);
}

void fsl_erratum_ifc_a003399(void)
{
	u32 mas0, mas1, mas2, mas3, mas7;
	void __iomem *l2cache = IOMEM(MPC85xx_L2_ADDR);
	void (*setup_ifc_sram)(void) = (void *)SRAM_BASE_ADDR;
	u32 *dst, *src, ix;

	/* TLB for SRAM */
	mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(9);
	mas1 = MAS1_VALID | MAS1_TID(0) | MAS1_TS |
			MAS1_TSIZE(BOOKE_PAGESZ_1M);
	mas2 = FSL_BOOKE_MAS2(SRAM_BASE_ADDR, MAS2_I);
	mas3 = FSL_BOOKE_MAS3(SRAM_BASE_ADDR, 0,
			MAS3_SX | MAS3_SW | MAS3_SR);
	mas7 = FSL_BOOKE_MAS7(0);
	e500_write_tlb(mas0, mas1, mas2, mas3, mas7);

	out_be32(l2cache + MPC85xx_L2_L2SRBAR0_OFFSET, SRAM_BASE_ADDR);
	out_be32(l2cache + MPC85xx_L2_L2ERRDIS_OFFSET,
		(MPC85xx_L2ERRDIS_MBECC | MPC85xx_L2ERRDIS_SBECC));
	out_be32(l2cache + MPC85xx_L2_CTL_OFFSET,
		(MPC85xx_L2CTL_L2E | MPC85xx_L2CTL_L2SRAM_ENTIRE));
	/*
	 * Copy the code in setup_ifc to L2SRAM. Do a word copy
	 * because NOR Flash on P1010 does not support byte
	 * access (Erratum IFC-A002769)
	 */
	dst = (u32 *) SRAM_BASE_ADDR;
	src = (u32 *) setup_ifc;
	for (ix = 0; ix < 1024; ix++)
		*dst++ = *src++;
	setup_ifc_sram();

	clrbits_be32(l2cache + MPC85xx_L2_CTL_OFFSET,
		(MPC85xx_L2CTL_L2E | MPC85xx_L2CTL_L2SRAM_ENTIRE));
	out_be32(l2cache + MPC85xx_L2_L2SRBAR0_OFFSET, 0x0);
}
#else
void fsl_erratum_ifc_a003399(void) {}
#endif

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

#if defined(CONFIG_FSL_ERRATUM_P1010_A003549)
void fsl_erratum_p1010_a003549(void)
{
	void __iomem *guts = IOMEM(MPC85xx_GUTS_ADDR);

	setbits_be32(guts + MPC85xx_GUTS_PMUXCR_OFFSET,
			MPC85xx_PMUXCR_LCLK_IFC_CS3);
}
#else
void fsl_erratum_p1010_a003549(void) {}
#endif

void cpu_init_early_f(void)
{
	u32 mas0, mas1, mas2, mas3, mas7;

	mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(13);
	mas1 = MAS1_VALID | MAS1_TID(0) | MAS1_TS |
			MAS1_TSIZE(BOOKE_PAGESZ_1M);
	mas2 = FSL_BOOKE_MAS2(CFG_CCSRBAR, MAS2_I | MAS2_G);
	mas3 = FSL_BOOKE_MAS3(CFG_CCSRBAR_PHYS, 0, MAS3_SW | MAS3_SR);
	mas7 = FSL_BOOKE_MAS7(CFG_CCSRBAR_PHYS);

	e500_write_tlb(mas0, mas1, mas2, mas3, mas7);

	fsl_erratum_p1010_a003549();
	fsl_init_laws();
	fsl_erratum_ifc_a003399();

	e500_invalidate_tlb(1);
	e500_init_tlbs();
}

static int cpu_init_r(void)
{
	e500_disable_tlb(14);
	e500_disable_tlb(15);

	return 0;
}
core_initcall(cpu_init_r);

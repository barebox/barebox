/*
 *
 * (c) 2011 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
#include <mach/imx-regs.h>
#include <mach/esdctl.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/system.h>
#include <asm-generic/sections.h>
#include <asm-generic/memory_layout.h>

#ifdef CONFIG_NAND_IMX_BOOT
static void __bare_init __naked insdram(void)
{
	uint32_t r;

	/* setup a stack to be able to call imx_nand_load_image() */
	r = STACK_BASE + STACK_SIZE - 12;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));

	imx_nand_load_image(_text, barebox_image_size);

	board_init_lowlevel_return();
}
#endif

static inline void __bare_init  setup_sdram(uint32_t base, uint32_t esdctl,
		uint32_t esdcfg)
{
	uint32_t esdctlreg = ESDCTL0;
	uint32_t esdcfgreg = ESDCFG0;

	if (base == 0x90000000) {
		esdctlreg += 8;
		esdcfgreg += 8;
	}

	esdctl |= ESDCTL0_SDE;

	writel(esdcfg, esdcfgreg);
	writel(esdctl | ESDCTL0_SMODE_PRECHARGE, esdctlreg);
	writel(0, base + 1024);
	writel(esdctl | ESDCTL0_SMODE_AUTO_REFRESH, esdctlreg);
	readb(base);
	readb(base);
	writel(esdctl | ESDCTL0_SMODE_LOAD_MODE, esdctlreg);
	writeb(0, base + 0x33);
	writel(esdctl, esdctlreg);
}

void __bare_init __naked reset(void)
{
	uint32_t r;
#ifdef CONFIG_NAND_IMX_BOOT
	unsigned int *trg, *src;
#endif

	common_reset();

	/* AIPS setup - Only setup MPROTx registers. The PACR default values are good.
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, 0x43f00000);
	writel(0x77777777, 0x43f00004);
	writel(0x77777777, 0x53f00000);
	writel(0x77777777, 0x53f00004);

	/* MAX (Multi-Layer AHB Crossbar Switch) setup
	 * MPR - priority for MX25 is (SDHC2/SDMA)>USBOTG>RTIC>IAHB>DAHB
	 */
	writel(0x00043210, 0x43f04000);
	writel(0x00043210, 0x43f04100);
	writel(0x00043210, 0x43f04200);
	writel(0x00043210, 0x43f04300);
	writel(0x00043210, 0x43f04400);
	/* SGPCR - always park on last master */
	writel(0x10, 0x43f04010);
	writel(0x10, 0x43f04110);
	writel(0x10, 0x43f04210);
	writel(0x10, 0x43f04310);
	writel(0x10, 0x43f04410);
	/* MGPCR - restore default values */
	writel(0x0, 0x43f04800);
	writel(0x0, 0x43f04900);
	writel(0x0, 0x43f04a00);
	writel(0x0, 0x43f04b00);
	writel(0x0, 0x43f04c00);

	/* Configure M3IF registers
	 * M3IF Control Register (M3IFCTL) for MX25
	 * MRRP[0] = LCDC           on priority list (1 << 0)  = 0x00000001
	 * MRRP[1] = MAX1       not on priority list (0 << 1)  = 0x00000000
	 * MRRP[2] = MAX0       not on priority list (0 << 2)  = 0x00000000
	 * MRRP[3] = USB HOST   not on priority list (0 << 3)  = 0x00000000
	 * MRRP[4] = SDMA       not on priority list (0 << 4)  = 0x00000000
	 * MRRP[5] = SD/ATA/FEC not on priority list (0 << 5)  = 0x00000000
	 * MRRP[6] = SCMFBC     not on priority list (0 << 6)  = 0x00000000
	 * MRRP[7] = CSI        not on priority list (0 << 7)  = 0x00000000
	 *                                                       ----------
	 *                                                       0x00000001
	 */
	writel(0x1, 0xb8003000);

	/* configure ARM clk */
	writel(0x20034000, MX25_CCM_BASE_ADDR + CCM_CCTL);

	/* enable all the clocks */
	writel(0x1fffffff, MX25_CCM_BASE_ADDR + CCM_CGCR0);
	writel(0xffffffff, MX25_CCM_BASE_ADDR + CCM_CGCR1);
	writel(0x000fdfff, MX25_CCM_BASE_ADDR + CCM_CGCR2);

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0x80000000 && r < 0x90000000)
		board_init_lowlevel_return();

	/* set to 3.3v SDRAM */
	writel(0x800, MX25_IOMUXC_BASE_ADDR + 0x454);

	writel(ESDMISC_RST, ESDMISC);

	while (!(readl(ESDMISC) & (1 << 31)));

#define ESDCTLVAL	(ESDCTL0_ROW13 | ESDCTL0_COL9 |	ESDCTL0_DSIZ_15_0 | \
			 ESDCTL0_REF4 | ESDCTL0_PWDT_PRECHARGE_PWDN | ESDCTL0_BL)
#define ESDCFGVAL	(ESDCFGx_tRP_3 | ESDCFGx_tMRD_2 | ESDCFGx_tRAS_6 | \
			 ESDCFGx_tRRD_2 | ESDCFGx_tCAS_3 | ESDCFGx_tRCD_3 | \
			 ESDCFGx_tRC_9)

	setup_sdram(0x80000000, ESDCTLVAL, ESDCFGVAL);
	setup_sdram(0x90000000, ESDCTLVAL, ESDCFGVAL);

#ifdef CONFIG_NAND_IMX_BOOT
	/* skip NAND boot if not running from NFC space */
	r = get_pc();
	if (r < MX25_NFC_BASE_ADDR || r > MX25_NFC_BASE_ADDR + 0x800)
		board_init_lowlevel_return();

	src = (unsigned int *)MX25_NFC_BASE_ADDR;
	trg = (unsigned int *)_text;

	/* Move ourselves out of NFC SRAM */
	for (i = 0; i < 0x800 / sizeof(int); i++)
		*trg++ = *src++;

	/* Jump to SDRAM */
	r = (unsigned int)&insdram;
	__asm__ __volatile__("mov pc, %0" : : "r"(r));
#else
	board_init_lowlevel_return();
#endif
}

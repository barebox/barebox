/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
#include <mach/imx27-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <asm/cache-l2x0.h>
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

#define ESDCTL0_VAL (ESDCTL0_SDE | ESDCTL0_ROW13 | ESDCTL0_COL10)

void __bare_init __naked reset(void)
{
	uint32_t r;
	int i;
#ifdef CONFIG_NAND_IMX_BOOT
	unsigned int *trg, *src;
#endif

	common_reset();

	/* ahb lite ip interface */
	writel(0x20040304, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR0);
	writel(0xDFFBFCFB, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR1);
	writel(0x00000000, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR0);
	writel(0xFFFFFFFF, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR1);

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0xa0000000 && r < 0xb0000000)
		board_init_lowlevel_return();

	/*
	 * DDR on CSD0
	 */
	/* Enable DDR SDRAM operation */
	writel(0x00000008, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC);

	/* Set the driving strength   */
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(3));
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(5));
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(6));
	writel(0x00005005, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(7));
	writel(0x15555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(8));

	/* Initial reset */
	writel(0x00000004, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC);
	writel(0x006ac73a, MX27_ESDCTL_BASE_ADDR + IMX_ESDCFG0);

	/* precharge CSD0 all banks */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0x00000000, 0xA0000F00);	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	for (i = 0; i < 8; i++)
		writel(0, 0xa0000f00);

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	writeb(0xda, 0xa0000033);
	writeb(0xff, 0xa1000000);
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 |
			ESDCTL0_BL | ESDCTL0_SMODE_NORMAL,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

#ifdef CONFIG_NAND_IMX_BOOT
	/* skip NAND boot if not running from NFC space */
	r = get_pc();
	if (r < MX27_NFC_BASE_ADDR || r > MX27_NFC_BASE_ADDR + 0x800)
		board_init_lowlevel_return();

	src = (unsigned int *)MX27_NFC_BASE_ADDR;
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


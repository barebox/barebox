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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <init.h>
#include <mach/imx-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <asm/cache-l2x0.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm-generic/sections.h>
#include <asm-generic/memory_layout.h>

#ifdef CONFIG_NAND_IMX_BOOT
static void __bare_init __naked insdram(void)
{
	uint32_t r;

	PCCR1 |= PCCR1_NFC_BAUDEN;

	/* setup a stack to be able to call imx_nand_load_image() */
	r = STACK_BASE + STACK_SIZE - 12;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));

	imx_nand_load_image(_text, barebox_image_size);

	board_init_lowlevel_return();
}
#endif

#define ESDCTL0_VAL (ESDCTL0_SDE | ESDCTL0_ROW13 | ESDCTL0_COL10)

void __bare_init __naked board_init_lowlevel(void)
{
	uint32_t r;
	int i;
#ifdef CONFIG_NAND_IMX_BOOT
	unsigned int *trg, *src;
#endif
	/* ahb lite ip interface */
	AIPI1_PSR0 = 0x20040304;
	AIPI1_PSR1 = 0xDFFBFCFB;
	AIPI2_PSR0 = 0x00000000;
	AIPI2_PSR1 = 0xFFFFFFFF;

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0xa0000000 && r < 0xb0000000)
		board_init_lowlevel_return();

	/*
	 * DDR on CSD0
	 */
	writel(0x00000008, ESDMISC); /* Enable DDR SDRAM operation */

	DSCR(3) = 0x55555555; /* Set the driving strength   */
	DSCR(5) = 0x55555555;
	DSCR(6) = 0x55555555;
	DSCR(7) = 0x00005005;
	DSCR(8) = 0x15555555;

	writel(0x00000004, ESDMISC); /* Initial reset */
	writel(0x006ac73a, ESDCFG0);

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE, ESDCTL0); /* precharge CSD0 all banks */
	writel(0x00000000, 0xA0000F00);	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH, ESDCTL0);

	for (i = 0; i < 8; i++)
		writel(0, 0xa0000f00);

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE, ESDCTL0);

	writeb(0xda, 0xa0000033);
	writeb(0xff, 0xa1000000);
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 |
			ESDCTL0_BL | ESDCTL0_SMODE_NORMAL, ESDCTL0);

#ifdef CONFIG_NAND_IMX_BOOT
	/* skip NAND boot if not running from NFC space */
	r = get_pc();
	if (r < IMX_NFC_BASE || r > IMX_NFC_BASE + 0x800)
		board_init_lowlevel_return();

	src = (unsigned int *)IMX_NFC_BASE;
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


/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (c) 2010 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
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
#include <mach/imx25-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/sections.h>
#include <asm-generic/memory_layout.h>
#include <asm/system.h>

void __bare_init __naked barebox_arm_reset_vector(void)
{
	uint32_t r;
	register uint32_t loops = 0x20000;

	arm_cpu_lowlevel_init();

	/* restart the MPLL and wait until it's stable */
	writel(readl(MX25_CCM_BASE_ADDR + MX25_CCM_CCTL) | (1 << 27),
			MX25_CCM_BASE_ADDR + MX25_CCM_CCTL);
	while (readl(MX25_CCM_BASE_ADDR + MX25_CCM_CCTL) & (1 << 27)) {};

	/* Configure dividers and ARM clock source
	 * 	ARM @ 400 MHz
	 * 	AHB @ 133 MHz
	 */
	writel(0x20034000, MX25_CCM_BASE_ADDR + MX25_CCM_CCTL);

	/* Enable UART1 / FEC / */
/*	writel(0x1FFFFFFF, MX25_CCM_BASE_ADDR + CCM_CGCR0);
	writel(0xFFFFFFFF, MX25_CCM_BASE_ADDR + CCM_CGCR1);
	writel(0x000FDFFF, MX25_CCM_BASE_ADDR + CCM_CGCR2);*/

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
	writel(0x00002143, 0x43f04000);
	writel(0x00002143, 0x43f04100);
	writel(0x00002143, 0x43f04200);
	writel(0x00002143, 0x43f04300);
	writel(0x00002143, 0x43f04400);
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

	/* Speed up NAND controller by adjusting the NFC divider */
	r = readl(MX25_CCM_BASE_ADDR + MX25_CCM_PCDR2);
	r &= ~0xf;
	r |= 0x1;
	writel(r, MX25_CCM_BASE_ADDR + MX25_CCM_PCDR2);

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0x80000000 && r < 0x90000000)
		goto out;

	/* Init Mobile DDR */
	writel(0x0000000E, MX25_ESDCTL_BASE_ADDR + IMX_ESDMISC);
	writel(0x00000004, MX25_ESDCTL_BASE_ADDR + IMX_ESDMISC);
	__asm__ volatile ("1:\n"
			"subs %0, %1, #1\n"
			"bne 1b":"=r" (loops):"0" (loops));

	writel(0x0029572B, MX25_ESDCTL_BASE_ADDR + IMX_ESDCFG0);
	writel(0x92210000, MX25_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(0xda, MX25_CSD0_BASE_ADDR + 0x400);
	writel(0xA2210000, MX25_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(0xda, MX25_CSD0_BASE_ADDR);
	writeb(0xda, MX25_CSD0_BASE_ADDR);
	writel(0xB2210000, MX25_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(0xda, MX25_CSD0_BASE_ADDR + 0x33);
	writeb(0xda, MX25_CSD0_BASE_ADDR + 0x1000000);
	writel(0x82216080, MX25_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

#ifdef CONFIG_NAND_IMX_BOOT
	/* setup a stack to be able to call imx25_barebox_boot_nand_external() */
	arm_setup_stack(STACK_BASE + STACK_SIZE - 12);

	imx25_barebox_boot_nand_external();
#endif
out:
	imx25_barebox_entry(0);
}

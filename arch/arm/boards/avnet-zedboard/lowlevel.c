/*
 *
 * (c) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
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
#include <io.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/zynq7000-regs.h>

#define DCI_DONE	(1 << 13)
#define PLL_ARM_LOCK	(1 << 0)
#define PLL_DDR_LOCK	(1 << 1)
#define PLL_IO_LOCK	(1 << 2)

void __naked barebox_arm_reset_vector(void)
{
	/* open sesame */
	writel(0x0000DF0D, ZYNQ_SLCR_UNLOCK);

	/* turn on LD9 */
	writel(0x00000200, 0xF800071C);
	writel(0x00000080, 0xE000A204);
	writel(0x00000080, 0xE000A000);

	/* ps7_clock_init_data */
	writel(0x1F000200, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_CLK_CTRL);
	writel(0x00F00701, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DCI_CLK_CTRL);
	writel(0x00002803, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_UART_CLK_CTRL);
	writel(0x00000A03, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DBG_CLK_CTRL);
	writel(0x00000501, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_PCAP_CLK_CTRL);
	writel(0x00000000, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_TOPSW_CLK_CTRL);
	writel(0x00100A00, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_FPGA0_CLK_CTRL);
	writel(0x00100700, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_FPGA1_CLK_CTRL);
	writel(0x00101400, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_FPGA2_CLK_CTRL);
	writel(0x00101400, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_FPGA3_CLK_CTRL);
	/* 6:2:1 mode */
	writel(0x00000001, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_CLK_621_TRUE);
	writel(0x01FC044D, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_APER_CLK_CTRL);

	/* configure the PLLs */
	/* ARM PLL */
	writel(0x00028008, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL);
	writel(0x000FA220, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CFG);
	writel(0x00028010, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL);
	writel(0x00028011, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL);
	writel(0x00028010, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL);

	while (!(readl(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_PLL_STATUS) & PLL_ARM_LOCK))
		;
	writel(0x00028000, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_ARM_PLL_CTRL);

	/* DDR PLL */
	/* set to bypass mode */
	writel(0x0001A018, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_PLL_CTRL);
	/* assert reset */
	writel(0x0001A019, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_PLL_CTRL);
	/* set feedback divs */
	writel(0x00020019, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_PLL_CTRL);
	writel(0x0012C220, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_PLL_CFG);
	/* set ddr2xclk and ddr3xclk: 3,2 */
	writel(0x0C200003, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_CLK_CTRL);
	/* deassert reset */
	writel(0x00020018, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_PLL_CTRL);
	/* wait pll lock */
	while (!(readl(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_PLL_STATUS) & PLL_DDR_LOCK))
		;
	/* remove bypass mode */
	writel(0x00020008, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_DDR_PLL_CTRL);

	/* IO PLL */
	writel(0x0001E008, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL);
	writel(0x001452C0, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CFG);
	writel(0x0001E010, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL);
	writel(0x0001E011, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL);
	writel(0x0001E010, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL);

	while (!(readl(ZYNQ_CLOCK_CTRL_BASE + ZYNQ_PLL_STATUS) & PLL_IO_LOCK))
		;
	writel(0x0001E000, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_IO_PLL_CTRL);

	/*
	 * INP_TYPE[1:2] = {off, vref, diff, lvcmos}
	 * DCI_UPDATE[3], TERM_EN[4]
	 * DCI_TYPE[5:6] = {off, drive, res, term}
	 * IBUF_DISABLE_MODE[7] = {ibuf, ibuf_disable}
	 * TERM_DISABLE_MODE[8] = {always, dynamic}
	 * OUTPUT_EN[9:10] = {ibuf, res, res, obuf}
	 * PULLUP_EN[11]
	 */
	writel(0x00000600, ZYNQ_DDRIOB_ADDR0);
	writel(0x00000600, ZYNQ_DDRIOB_ADDR1);
	writel(0x00000672, ZYNQ_DDRIOB_DATA0);
	writel(0x00000672, ZYNQ_DDRIOB_DATA1);
	writel(0x00000674, ZYNQ_DDRIOB_DIFF0);
	writel(0x00000674, ZYNQ_DDRIOB_DIFF1);
	writel(0x00000600, ZYNQ_DDRIOB_CLOCK);
	/*
	 * Drive_P[0:6], Drive_N[7:13]
	 * Slew_P[14:18], Slew_N[19:23]
	 * GTL[24:26], RTerm[27:31]
	 */
	writel(0x00D6861C, ZYNQ_DDRIOB_DRIVE_SLEW_ADDR);
	writel(0x00F9861C, ZYNQ_DDRIOB_DRIVE_SLEW_DATA);
	writel(0x00F9861C, ZYNQ_DDRIOB_DRIVE_SLEW_DIFF);
	writel(0x00D6861C, ZYNQ_DDRIOB_DRIVE_SLEW_CLOCK);
	/*
	 * VREF_INT_EN[0]
	 * VREF_SEL[1:4] = {0001=0.6V, 0100=0.75V, 1000=0.9V}
	 * VREF_EXT_EN[5:6] = {dis/dis, dis/en, en/dis, en/en}
	 * RES[7:8], REFIO_EN[9]
	 */
	/* FIXME: Xilinx sets this to internal, but Zedboard should support
	   external VRef, too */
	writel(0x00000E09, ZYNQ_DDRIOB_DDR_CTRL);
	/*
	 * RESET[0], ENABLE[1]
	 * NREF_OPT1[6:7], NREF_OPT2[8:10], NREF_OPT4[11:13]
	 * PREF_OPT1[14:15], PREF_OPT2[17:19], UPDATE_CONTROL[20]
	 */
	writel(0x00000021, ZYNQ_DDRIOB_DCI_CTRL);
	writel(0x00000020, ZYNQ_DDRIOB_DCI_CTRL);
	writel(0x00100823, ZYNQ_DDRIOB_DCI_CTRL);

	while (!(readl(ZYNQ_DDRIOB_DCI_STATUS) & DCI_DONE))
		;

	writel(0x0E00E07F, 0xF8007000);

	/* ps7_ddr_init_data */
	writel(0x00000080, 0XF8006000);
	writel(0x00081081, 0XF8006004);
	writel(0x03C0780F, 0XF8006008);
	writel(0x02001001, 0XF800600C);
	writel(0x00014001, 0XF8006010);
	writel(0x0004159B, 0XF8006014);
	writel(0x452460D2, 0XF8006018);
	writel(0x720238E5, 0XF800601C);
	writel(0x272872D0, 0XF8006020);
	writel(0x0000003C, 0XF8006024);
	writel(0x00002007, 0XF8006028);
	writel(0x00000008, 0XF800602C);
	writel(0x00040930, 0XF8006030);
	writel(0x00010694, 0XF8006034);
	writel(0x00000000, 0XF8006038);
	writel(0x00000777, 0XF800603C);
	writel(0xFFF00000, 0XF8006040);
	writel(0x0FF66666, 0XF8006044);
	writel(0x0003C248, 0XF8006048);
	writel(0x77010800, 0XF8006050);
	writel(0x00000101, 0XF8006058);
	writel(0x00005003, 0XF800605C);
	writel(0x0000003E, 0XF8006060);
	writel(0x00020000, 0XF8006064);
	writel(0x00284141, 0XF8006068);
	writel(0x00001610, 0XF800606C);
	writel(0x00008000, 0XF80060A0);
	writel(0x10200802, 0XF80060A4);
	writel(0x0690CB73, 0XF80060A8);
	writel(0x000001FE, 0XF80060AC);
	writel(0x1CFFFFFF, 0XF80060B0);
	writel(0x00000200, 0XF80060B4);
	writel(0x00200066, 0XF80060B8);
	writel(0x00000000, 0XF80060BC);
	writel(0x00000000, 0XF80060C4);
	writel(0x00000000, 0XF80060C8);
	writel(0x00000000, 0XF80060DC);
	writel(0x00000000, 0XF80060F0);
	writel(0x00000008, 0XF80060F4);
	writel(0x00000000, 0XF8006114);
	writel(0x40000001, 0XF8006118);
	writel(0x40000001, 0XF800611C);
	writel(0x40000001, 0XF8006120);
	writel(0x40000001, 0XF8006124);
	writel(0x00033C03, 0XF800612C);
	writel(0x00034003, 0XF8006130);
	writel(0x0002F400, 0XF8006134);
	writel(0x00030400, 0XF8006138);
	writel(0x00000035, 0XF8006140);
	writel(0x00000035, 0XF8006144);
	writel(0x00000035, 0XF8006148);
	writel(0x00000035, 0XF800614C);
	writel(0x00000083, 0XF8006154);
	writel(0x00000083, 0XF8006158);
	writel(0x0000007F, 0XF800615C);
	writel(0x00000078, 0XF8006160);
	writel(0x00000124, 0XF8006168);
	writel(0x00000125, 0XF800616C);
	writel(0x00000112, 0XF8006170);
	writel(0x00000116, 0XF8006174);
	writel(0x000000C3, 0XF800617C);
	writel(0x000000C3, 0XF8006180);
	writel(0x000000BF, 0XF8006184);
	writel(0x000000B8, 0XF8006188);
	writel(0x10040080, 0XF8006190);
	writel(0x0001FC82, 0XF8006194);
	writel(0x00000000, 0XF8006204);
	writel(0x000803FF, 0XF8006208);
	writel(0x000803FF, 0XF800620C);
	writel(0x000803FF, 0XF8006210);
	writel(0x000803FF, 0XF8006214);
	writel(0x000003FF, 0XF8006218);
	writel(0x000003FF, 0XF800621C);
	writel(0x000003FF, 0XF8006220);
	writel(0x000003FF, 0XF8006224);
	writel(0x00000000, 0XF80062A8);
	writel(0x00000000, 0XF80062AC);
	writel(0x00005125, 0XF80062B0);
	writel(0x000012A8, 0XF80062B4);
	writel(0x00000081, 0XF8006000);

	/* poor mans pinctrl */
	writel(0x000002E0, ZYNQ_MIO_BASE + 0xC0);
	writel(0x000002E1, ZYNQ_MIO_BASE + 0xC4);
	/* UART1 pinmux */
	writel(0x000002E1, ZYNQ_MIO_BASE + 0xC8);
	writel(0x000002E0, ZYNQ_MIO_BASE + 0xCC);

	/* poor mans clkctrl */
	writel(0x00001403, ZYNQ_CLOCK_CTRL_BASE + ZYNQ_UART_CLK_CTRL);

	/* GEM0 */
	writel(0x00000001, 0xf8000138);
	writel(0x00500801, 0xf8000140);
	writel(0x00000302, 0xf8000740);
	writel(0x00000302, 0xf8000744);
	writel(0x00000302, 0xf8000748);
	writel(0x00000302, 0xf800074C);
	writel(0x00000302, 0xf8000750);
	writel(0x00000302, 0xf8000754);
	writel(0x00001303, 0xf8000758);
	writel(0x00001303, 0xf800075C);
	writel(0x00001303, 0xf8000760);
	writel(0x00001303, 0xf8000764);
	writel(0x00001303, 0xf8000768);
	writel(0x00001303, 0xf800076C);
	writel(0x00001280, 0xf80007D0);
	writel(0x00001280, 0xf80007D4);

	writel(0x00000001, 0xf8000B00);

	/* lock up. secure, secure */
	writel(0x0000767B, ZYNQ_SLCR_LOCK);

	arm_cpu_lowlevel_init();
	barebox_arm_entry(0, SZ_512M, 0);
}

/*
 * i.MX6 DDR controller calibration functions
 * Based on Freescale code
 *
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <mach/imx6-mmdc.h>
#include <mach/imx6-regs.h>
#include <mach/imx6.h>

int mmdc_do_write_level_calibration(void)
{
	u32 esdmisc_val, zq_val;
	int errorcount = 0;
	u32 val;
	u32 ddr_mr1 = 0x4;

	/* disable DDR logic power down timer */
	val = readl((P0_IPS + MDPDC));
	val &= 0xffff00ff;
	writel(val, (P0_IPS + MDPDC)),

	/* disable Adopt power down timer */
	val = readl((P0_IPS + MAPSR));
	val |= 0x1;
	writel(val, (P0_IPS + MAPSR));

	pr_debug("Start write leveling calibration \n");

	/*
	 * disable auto refresh and ZQ calibration
	 * before proceeding with Write Leveling calibration
	 */
	esdmisc_val = readl(P0_IPS + MDREF);
	writel(0x0000C000, (P0_IPS + MDREF));
	zq_val = readl(P0_IPS + MPZQHWCTRL);
	writel(zq_val & ~(0x3), (P0_IPS + MPZQHWCTRL));

	/*
	 * Configure the external DDR device to enter write leveling mode
	 * through Load Mode Register command
	 * Register setting:
	 * Bits[31:16] MR1 value (0x0080 write leveling enable)
	 * Bit[9] set WL_EN to enable MMDC DQS output
	 * Bits[6:4] set CMD bits for Load Mode Register programming
	 * Bits[2:0] set CMD_BA to 0x1 for DDR MR1 programming
	 */
	writel(0x00808231, P0_IPS + MDSCR);

	/* Activate automatic calibration by setting MPWLGCR[HW_WL_EN] */
	writel(0x00000001, P0_IPS + MPWLGCR);

	/* Upon completion of this process the MMDC de-asserts the MPWLGCR[HW_WL_EN] */
	while (readl(P0_IPS + MPWLGCR) & 0x00000001);

	/* check for any errors: check both PHYs for x64 configuration, if x32, check only PHY0 */
	if ((readl(P0_IPS + MPWLGCR) & 0x00000F00) ||
			(readl(P1_IPS + MPWLGCR) & 0x00000F00)) {
		errorcount++;
	}

	pr_debug("Write leveling calibration completed\n");

	/*
	 * User should issue MRS command to exit write leveling mode
	 * through Load Mode Register command
	 * Register setting:
	 * Bits[31:16] MR1 value "ddr_mr1" value from initialization
	 * Bit[9] clear WL_EN to disable MMDC DQS output
	 * Bits[6:4] set CMD bits for Load Mode Register programming
	 * Bits[2:0] set CMD_BA to 0x1 for DDR MR1 programming
	 */
	writel(((ddr_mr1 << 16)+0x8031), P0_IPS + MDSCR);

	/* re-enable to auto refresh and zq cal */
	writel(esdmisc_val, (P0_IPS + MDREF));
	writel(zq_val, (P0_IPS + MPZQHWCTRL));

	pr_debug("MMDC_MPWLDECTRL0 after write level cal: 0x%08X\n",
			readl(P0_IPS + MPWLDECTRL0));
	pr_debug("MMDC_MPWLDECTRL1 after write level cal: 0x%08X\n",
			readl(P0_IPS + MPWLDECTRL1));
	pr_debug("MMDC_MPWLDECTRL0 after write level cal: 0x%08X\n",
			readl(P1_IPS + MPWLDECTRL0));
	pr_debug("MMDC_MPWLDECTRL1 after write level cal: 0x%08X\n",
			readl(P1_IPS + MPWLDECTRL1));

	/* enable DDR logic power down timer */
	val = readl((P0_IPS + MDPDC));
	val |= 0x00005500;
	writel(val, (P0_IPS + MDPDC));

	/* enable Adopt power down timer: */
	val = readl(P0_IPS + MAPSR);
	val &= 0xfffffff7;
	writel(val, (P0_IPS + MAPSR));

	/* clear CON_REQ */
	writel(0, (P0_IPS + MDSCR));

	return 0;
}

static void modify_dg_result(void __iomem *reg_st0, void __iomem *reg_st1,
		void __iomem *reg_ctrl)
{
	u32 dg_tmp_val, dg_dl_abs_offset, dg_hc_del, val_ctrl;

	/*
	 * DQS gating absolute offset should be modified from reflecting (HW_DG_LOWx + HW_DG_UPx)/2
	 * to reflecting (HW_DG_UPx - 0x80)
	 */

	val_ctrl = readl(reg_ctrl);
	val_ctrl &= 0xf0000000;

	dg_tmp_val = ((readl(reg_st0) & 0x07ff0000) >> 16) - 0xc0;
	dg_dl_abs_offset = dg_tmp_val & 0x7f;
	dg_hc_del = (dg_tmp_val & 0x780) << 1;

	val_ctrl |= dg_dl_abs_offset + dg_hc_del;

	dg_tmp_val = ((readl(reg_st1) & 0x07ff0000) >> 16) - 0xc0;
	dg_dl_abs_offset = dg_tmp_val & 0x7f;
	dg_hc_del = (dg_tmp_val & 0x780) << 1;

	val_ctrl |= (dg_dl_abs_offset + dg_hc_del) << 16;

	writel(val_ctrl, reg_ctrl);
}

static void mmdc_precharge_all(int cs0_enable, int cs1_enable)
{
	/*
	 * Issue the Precharge-All command to the DDR device for both chip selects
	 * Note, CON_REQ bit should also remain set
	 * If only using one chip select, then precharge only the desired chip select
	 */
	if (cs0_enable)
		writel(0x04008050, P0_IPS + MDSCR);
	if (cs1_enable)
		writel(0x04008058, P0_IPS + MDSCR);
}

static void mmdc_force_delay_measurement(int data_bus_size)
{
	writel(0x800, P0_IPS + MPMUR);

	if (data_bus_size == 0x2)
		writel(0x800, P1_IPS + MPMUR);
}

static void mmdc_reset_read_data_fifos(void)
{
	uint32_t v;

	/*
	 * Reset the read data FIFOs (two resets); only need to issue reset to PHY0 since in x64
	 * mode, the reset will also go to PHY1
	 * read data FIFOs reset1
	 */
	v = readl(P0_IPS + MPDGCTRL0);
	v |= 0x80000000;
	writel(v, P0_IPS + MPDGCTRL0);

	while (readl((P0_IPS + MPDGCTRL0)) & 0x80000000);

	/* read data FIFOs reset2 */
	v = readl(P0_IPS + MPDGCTRL0);
	v |= 0x80000000;
	writel(v, P0_IPS + MPDGCTRL0);

	while (readl((P0_IPS + MPDGCTRL0)) & 0x80000000);
}

int mmdc_do_dqs_calibration(void)
{
	u32 esdmisc_val, v;
	int g_error_write_cal;
	int temp_ref;
	int cs0_enable_initial;
	int cs1_enable_initial;
	int PDDWord = 0x00ffff00;
	int errorcount = 0;
	int cs0_enable;
	int cs1_enable;
	int data_bus_size;

	/* check to see which chip selects are enabled */
	cs0_enable_initial = (readl(P0_IPS + MDCTL) & 0x80000000) >> 31;
	cs1_enable_initial = (readl(P0_IPS + MDCTL) & 0x40000000) >> 30;

	/* disable DDR logic power down timer */
	v = readl(P0_IPS + MDPDC);
	v &= ~0xff00;
	writel(v, P0_IPS + MDPDC);

	/* disable Adopt power down timer */
	v = readl(P0_IPS + MAPSR);
	v |= 0x1;
	writel(v, P0_IPS + MAPSR);

	/* set DQS pull ups */
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS4) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS4);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS5) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS5);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS6) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS6);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS7) | 0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS7);

	esdmisc_val = readl(P0_IPS + MDMISC);

	/* set RALAT and WALAT to max */
	v = readl(P0_IPS + MDMISC);
	v |= (1 << 6) | (1 << 7) | (1 << 8) | (1 << 16) | (1 << 17);
	writel(v, P0_IPS + MDMISC);

	/*
	 * disable auto refresh
	 * before proceeding with calibration
	 */
	temp_ref = readl(P0_IPS + MDREF);
	writel(0x0000C000, P0_IPS + MDREF);

	/*
	 * per the ref manual, issue one refresh cycle MDSCR[CMD]= 0x2,
	 * this also sets the CON_REQ bit.
	 */
	if (cs0_enable_initial)
		writel(0x00008020, P0_IPS + MDSCR);
	if (cs1_enable_initial)
		writel(0x00008028, P0_IPS + MDSCR);

	/* poll to make sure the con_ack bit was asserted */
	while (!(readl(P0_IPS + MDSCR) & 0x00004000)) ;

	/*
	 * check MDMISC register CALIB_PER_CS to see which CS calibration is
	 * targeted to (under normal cases, it should be cleared as this is the
	 * default value, indicating calibration is directed to CS0). Disable
	 * the other chip select not being target for calibration to avoid any
	 * potential issues This will get re-enabled at end of calibration.
	 */
	v = readl(P0_IPS + MDCTL);
	if ((readl(P0_IPS + MDMISC) & 0x00100000) == 0)
		v &= ~(1 << 30); /* clear SDE_1 */
	else
		v &= ~(1 << 31); /* clear SDE_0 */
	writel(v, P0_IPS + MDCTL);

	/*
	 * check to see which chip selects are now enabled for the remainder
	 * of the calibration.
	 */
	cs0_enable = (readl(P0_IPS + MDCTL) & 0x80000000) >> 31;
	cs1_enable = (readl(P0_IPS + MDCTL) & 0x40000000) >> 30;

	/* check to see what is the data bus size */
	data_bus_size = (readl(P0_IPS + MDCTL) & 0x30000) >> 16;

	mmdc_precharge_all(cs0_enable, cs1_enable);

	/* Write the pre-defined value into MPPDCMPR1 */
	writel(PDDWord, P0_IPS + MPPDCMPR1);

	/*
	 * Issue a write access to the external DDR device by setting the bit SW_DUMMY_WR (bit 0)
	 * in the MPSWDAR0 and then poll this bit until it clears to indicate completion of the
	 * write access.
	 */
	v = readl(P0_IPS + MPSWDAR);
	v |= (1 << 0);
	writel(v, P0_IPS + MPSWDAR);

	while (readl(P0_IPS + MPSWDAR) & 0x00000001);

	/*
	 * Set the RD_DL_ABS# bits to their default values (will be calibrated later in
	 * the read delay-line calibration)
	 * Both PHYs for x64 configuration, if x32, do only PHY0
	 */
	writel(0x40404040, P0_IPS + MPRDDLCTL);
	if (data_bus_size == 0x2)
		writel(0x40404040, P1_IPS + MPRDDLCTL);

	/* Force a measurement, for previous delay setup to take effect */
	mmdc_force_delay_measurement(data_bus_size);

	/*
	 * Read DQS Gating calibration
	 */

	pr_debug("Starting DQS gating calibration...\n");

	mmdc_reset_read_data_fifos();

	/*
	 * Start the automatic read DQS gating calibration process by asserting
	 * MPDGCTRL0[HW_DG_EN] and MPDGCTRL0[DG_CMP_CYC] and then poll
	 * MPDGCTRL0[HW_DG_EN]] until this bit clears to indicate completion.
	 * Also, ensure that MPDGCTRL0[HW_DG_ERR] is clear to indicate no errors
	 * were seen during calibration. Set bit 30: chooses option to wait 32
	 * cycles instead of 16 before comparing read data
	 */
	v = readl(P0_IPS + MPDGCTRL0);
	v |= (1 << 30);
	writel(v, P0_IPS + MPDGCTRL0);

	/* Set bit 28 to start automatic read DQS gating calibration */
	v |= (1 << 28);
	writel(v, P0_IPS + MPDGCTRL0);

	/*
	 * Poll for completion
	 * MPDGCTRL0[HW_DG_EN] should be 0
	 */
	while (readl(P0_IPS + MPDGCTRL0) & 0x10000000);

	/*
	 * Check to see if any errors were encountered during calibration
	 * (check MPDGCTRL0[HW_DG_ERR])
	 * check both PHYs for x64 configuration, if x32, check only PHY0
	 */
	if (data_bus_size == 0x2) {
		if ((readl(P0_IPS + MPDGCTRL0) & 0x00001000) ||
				(readl(P1_IPS + MPDGCTRL0) & 0x00001000))
			errorcount++;
	} else {
		if (readl(P0_IPS + MPDGCTRL0) & 0x00001000)
			errorcount++;
	}

	/*
	 * DQS gating absolute offset should be modified from reflecting
	 * (HW_DG_LOWx + HW_DG_UPx)/2 to reflecting (HW_DG_UPx - 0x80)
	 */
	modify_dg_result(P0_IPS + MPDGHWST0,
			P0_IPS + MPDGHWST1,
			P0_IPS + MPDGCTRL0);

	modify_dg_result(P0_IPS + MPDGHWST2,
			P0_IPS + MPDGHWST3,
			P0_IPS + MPDGCTRL1);

	if (data_bus_size == 0x2) {
		modify_dg_result(P1_IPS + MPDGHWST0,
				P1_IPS + MPDGHWST1,
				P1_IPS + MPDGCTRL0);
		modify_dg_result(P1_IPS + MPDGHWST2,
				P1_IPS + MPDGHWST3,
				P1_IPS + MPDGCTRL1);
	}

	pr_debug("DQS gating calibration completed.\n");

	/*
	 * Read delay Calibration
	 */

	pr_debug("Starting read calibration...\n");

	mmdc_reset_read_data_fifos();

	mmdc_precharge_all(cs0_enable, cs1_enable);

	/*
	 * Read delay-line calibration
	 * Start the automatic read calibration process by asserting MPRDDLHWCTL[ HW_RD_DL_EN]
	 */
	writel(0x00000030, P0_IPS + MPRDDLHWCTL);

	/*
	 * poll for completion
	 * MMDC indicates that the write data calibration had finished by setting
	 * MPRDDLHWCTL[HW_RD_DL_EN] = 0
	 * Also, ensure that no error bits were set
	 */
	while (readl(P0_IPS + MPRDDLHWCTL) & 0x00000010) ;

	/* check both PHYs for x64 configuration, if x32, check only PHY0 */
	if (data_bus_size == 0x2) {
		if ((readl(P0_IPS + MPRDDLHWCTL) & 0x0000000f) ||
				(readl(P1_IPS + MPRDDLHWCTL) & 0x0000000f)) {
			errorcount++;
		}
	} else {
		if (readl(P0_IPS + MPRDDLHWCTL) & 0x0000000f) {
			errorcount++;
		}
	}

	pr_debug("Read calibration completed\n");

	/*
	 * Write delay Calibration
	 */

	pr_debug("Starting write calibration...\n");

	mmdc_reset_read_data_fifos();

	mmdc_precharge_all(cs0_enable, cs1_enable);

	/*
	 * Set the WR_DL_ABS# bits to their default values
	 * Both PHYs for x64 configuration, if x32, do only PHY0
	 */
	writel(0x40404040, P0_IPS + MPWRDLCTL);
	if (data_bus_size == 0x2)
		writel(0x40404040, P1_IPS + MPWRDLCTL);

	mmdc_force_delay_measurement(data_bus_size);

	/* Start the automatic write calibration process by asserting MPWRDLHWCTL0[HW_WR_DL_EN] */
	writel(0x00000030, P0_IPS + MPWRDLHWCTL);

	/*
	 * poll for completion
	 * MMDC indicates that the write data calibration had finished by setting
	 * MPWRDLHWCTL[HW_WR_DL_EN] = 0
	 * Also, ensure that no error bits were set
	 */
	while (readl(P0_IPS + MPWRDLHWCTL) & 0x00000010) ;

	/* check both PHYs for x64 configuration, if x32, check only PHY0 */
	if (data_bus_size == 0x2) {
		if ((readl(P0_IPS + MPWRDLHWCTL) & 0x0000000f) ||
				(readl(P1_IPS + MPWRDLHWCTL) & 0x0000000f)) {
			errorcount++;
			g_error_write_cal = 1; // set the g_error_write_cal
		}
	} else {
		if (readl(P0_IPS + MPWRDLHWCTL) & 0x0000000f) {
			errorcount++;
			g_error_write_cal = 1; // set the g_error_write_cal
		}
	}

	pr_debug("Write calibration completed\n");

	mmdc_reset_read_data_fifos();

	pr_debug("\n");

	/* enable DDR logic power down timer */
	v = readl(P0_IPS + MDPDC) | 0x00005500;
	writel(v, P0_IPS + MDPDC);

	/* enable Adopt power down timer */
	v = readl(P0_IPS + MAPSR) & 0xfffffff7;
	writel(v, P0_IPS + MAPSR);

	/* restore MDMISC value (RALAT, WALAT) */
	writel(esdmisc_val, P1_IPS + MDMISC);

	/* clear DQS pull ups */
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS4) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS4);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS5) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS5);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS6) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS6);
	v = readl(IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS7) & ~0x7000;
	writel(v, IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS7);

	v = readl(P0_IPS + MDCTL);

	/* re-enable SDE (chip selects) if they were set initially */
	if (cs1_enable_initial == 1)
		v |= (1 << 30); /* set SDE_1 */

	if (cs0_enable_initial == 1)
		v |= (1 << 31); /* set SDE_0 */

	writel(v, P0_IPS + MDCTL);

	/* re-enable to auto refresh */
	writel(temp_ref, P0_IPS + MDREF);

	/* clear the MDSCR (including the con_req bit) */
	writel(0x0, P0_IPS + MDSCR); /* CS0 */

	/* poll to make sure the con_ack bit is clear */
	while (readl(P0_IPS + MDSCR) & 0x00004000) ;

	return errorcount;
}

void mmdc_print_calibration_results(void)
{
	int data_bus_size;

	data_bus_size = (readl(P0_IPS + MDCTL) & 0x30000) >> 16;

	printf("MMDC registers updated from calibration \n");
	printf("\nRead DQS Gating calibration\n");
	printf("MPDGCTRL0 PHY0 (0x021b083c) = 0x%08X\n", readl(P0_IPS + MPDGCTRL0));
	printf("MPDGCTRL1 PHY0 (0x021b0840) = 0x%08X\n", readl(P0_IPS + MPDGCTRL1));
	if (data_bus_size == 0x2) {
		printf("MPDGCTRL0 PHY1 (0x021b483c) = 0x%08X\n", readl(P1_IPS + MPDGCTRL0));
		printf("MPDGCTRL1 PHY1 (0x021b4840) = 0x%08X\n", readl(P1_IPS + MPDGCTRL1));
	}
	printf("\nRead calibration\n");
	printf("MPRDDLCTL PHY0 (0x021b0848) = 0x%08X\n", readl(P0_IPS + MPRDDLCTL));
	if (data_bus_size == 0x2)
		printf("MPRDDLCTL PHY1 (0x021b4848) = 0x%08X\n", readl(P1_IPS + MPRDDLCTL));
	printf("\nWrite calibration\n");
	printf("MPWRDLCTL PHY0 (0x021b0850) = 0x%08X\n", readl(P0_IPS + MPWRDLCTL));
	if (data_bus_size == 0x2)
		printf("MPWRDLCTL PHY1 (0x021b4850) = 0x%08X\n", readl(P1_IPS + MPWRDLCTL));
	printf("\n");
	/*
	 * registers below are for debugging purposes
	 * these print out the upper and lower boundaries captured during read DQS gating calibration
	 */
	printf("Status registers, upper and lower bounds, for read DQS gating. \n");
	printf("MPDGHWST0 PHY0 (0x021b087c) = 0x%08X\n", readl(P0_IPS + MPDGHWST0));
	printf("MPDGHWST1 PHY0 (0x021b0880) = 0x%08X\n", readl(P0_IPS + MPDGHWST1));
	printf("MPDGHWST2 PHY0 (0x021b0884) = 0x%08X\n", readl(P0_IPS + MPDGHWST2));
	printf("MPDGHWST3 PHY0 (0x021b0888) = 0x%08X\n", readl(P0_IPS + MPDGHWST3));
	if (data_bus_size == 0x2) {
		printf("MPDGHWST0 PHY1 (0x021b487c) = 0x%08X\n", readl(P1_IPS + MPDGHWST0));
		printf("MPDGHWST1 PHY1 (0x021b4880) = 0x%08X\n", readl(P1_IPS + MPDGHWST1));
		printf("MPDGHWST2 PHY1 (0x021b4884) = 0x%08X\n", readl(P1_IPS + MPDGHWST2));
		printf("MPDGHWST3 PHY1 (0x021b4888) = 0x%08X\n", readl(P1_IPS + MPDGHWST3));
	}
}

#ifdef MMDC_SOFTWARE_CALIBRATION

static void mmdc_set_dqs(u32 value)
{
	value |= value << 8 | value << 16 | value << 24;

	writel(value, P0_IPS + MPRDDLCTL);

	if (data_bus_size == 0x2)
		writel(value, P1_IPS + MPRDDLCTL);
}

static void mmdc_set_wr_delay(u32 value)
{
	value |= value << 8 | value << 16 | value << 24;

	writel(value, P0_IPS + MPWRDLCTL);

	if (data_bus_size == 0x2)
		writel(value, P1_IPS + MPWRDLCTL);
}

static void mmdc_issue_write_access(void __iomem *base)
{
	u32 v;

	/*
	 * Issue a write access to the external DDR device by setting the bit SW_DUMMY_WR (bit 0)
	 * in the MPSWDAR0 and then poll this bit until it clears to indicate completion of the
	 * write access.
	 */

	v = readl(P0_IPS + MPSWDAR);
	v |= (1 << 0);
	writel(v, P0_IPS + MPSWDAR);

	while (readl(P0_IPS + MPSWDAR) & 0x00000001);
}

static void mmdc_issue_read_access(void __iomem *base)
{
	/*
	 * Issue a write access to the external DDR device by setting the bit SW_DUMMY_WR (bit 0)
	 * in the MPSWDAR0 and then poll this bit until it clears to indicate completion of the
	 * write access.
	 */

	v = readl(P0_IPS + MPSWDAR);
	v |= (1 << 1);
	writel(v, P0_IPS + MPSWDAR);

	while (readl(P0_IPS + MPSWDAR) & 0x00000002);
}

static int total_lower[2] = { 0x0, 0x0 };
static int total_upper[2] = { 0xff, 0xff };

static int mmdc_is_valid(void __iomem *base, int delay, int rd)
{
	u32 val;

	if (rd)
		mmdc_set_dqs(delay);
	else
		mmdc_set_wr_delay(delay);

	mmdc_force_delay_measurement();

	mdelay(1);

	if (!rd)
		mmdc_issue_write_access(base);

	mmdc_issue_read_access(base);

	val = readl(base + MPSWDAR);

	if ((val & 0x3c) == 0x3c)
		return 1;
	else
		return 0;
#ifdef MMDC_SOFWARE_CALIB_COMPARE_RESULTS
	if ((val & 0x3c) == 0x3c) {
		if (lower < 0)
			lower = i;
	}

	if ((val & 0x3c) != 0x3c) {
		if (lower > 0 && upper < 0)
			upper = i;
	}

	pr_debug("0x%02x: compare: 0x%08x ", i, readl(P0_IPS + MPSWDAR));
	for (j = 0; j < 8; j++) {
		pr_debug("0x%08x ", readl(P0_IPS + 0x898 + j * 4));
	}
	pr_debug("\n");
#endif
}

static void mmdc_sw_read_calib(int ch, u32 wr_value)
{
	int rd = 1;
	void __iomem *base;
	int i;
	int lower = 0x0, upper = 0x7f;

	if (ch)
		base = (void *)P1_IPS;
	else
		base = (void *)P0_IPS;

	/* 1. Precharge */
	mmdc_precharge_all(cs0_enable, cs1_enable);

	/* 2. Configure pre-defined value */
	writel(wr_value, P0_IPS + MPPDCMPR1);

	/* 3. Issue write access */
	mmdc_issue_write_access(base);

	for (i = 0; i < 100; i++) {
		if (mmdc_is_valid(base, 0x40, rd)) {
			goto middle_passed;
		}
	}

	pr_debug("ch: %d value: 0x%08x middle test failed\n", ch, wr_value);
	return;

middle_passed:
	for (i = 0x40; i < 0x7f; i++) {
		if (!mmdc_is_valid(base, i, rd)) {
			upper = i;
			break;
		}
	}

	for (i = 0; i < 100; i++) {
		if (mmdc_is_valid(base, 0x40, rd)) {
			goto go_on;
		}
	}

	pr_debug("ch: %d value: 0x%08x middle test 1 failed\n", ch, wr_value);
	return;

go_on:
	for (i = 0x40; i >= 0; i--) {
		if (!mmdc_is_valid(base, i, rd)) {
			lower = i;
			break;
		}
	}

	if (lower > total_lower[ch])
		total_lower[ch] = lower;

	if (upper < total_upper[ch])
		total_upper[ch] = upper;

	pr_debug("ch: %d value: 0x%08x lower: %-3d upper: %-3d\n", ch, wr_value, lower, upper);
}

static void mmdc_sw_write_calib(int ch, u32 wr_value)
{
	int rd = 0;
	void __iomem *base;
	int i;
	int lower = 0x0, upper = 0x7f;

	if (ch)
		base = (void *)P1_IPS;
	else
		base = (void *)P0_IPS;

	/* 1. Precharge */
	mmdc_precharge_all(cs0_enable, cs1_enable);

	/* 2. Configure pre-defined value */
	writel(wr_value, P0_IPS + MPPDCMPR1);

	/* 3. Issue write access */
	mmdc_issue_write_access(base);

	for (i = 0; i < 100; i++) {
		if (mmdc_is_valid(base, 0x40, rd)) {
			goto middle_passed;
		}
	}

	pr_debug("ch: %d value: 0x%08x middle test failed\n", ch, wr_value);
	return;

middle_passed:
	for (i = 0x40; i < 0x7f; i++) {
		if (!mmdc_is_valid(base, i, rd)) {
			upper = i;
			break;
		}
	}

	for (i = 0; i < 100; i++) {
		if (mmdc_is_valid(base, 0x40, rd)) {
			goto go_on;
		}
	}

	pr_debug("ch: %d value: 0x%08x middle test 1 failed\n", ch, wr_value);
	return;

go_on:
	for (i = 0x40; i >= 0; i--) {
		if (!mmdc_is_valid(base, i, rd)) {
			lower = i;
			break;
		}
	}

	if (lower > total_lower[ch])
		total_lower[ch] = lower;

	if (upper < total_upper[ch])
		total_upper[ch] = upper;

	pr_debug("ch: %d value: 0x%08x lower: %-3d upper: %-3d\n", ch, wr_value, lower, upper);
}

int mmdc_do_software_calibration(void)
{
	u32 s;
	int ch;

	for (ch = 0; ch < 2; ch++) {
		mmdc_sw_read_calib(ch, 0x00000055);
		mmdc_sw_read_calib(ch, 0x00005500);
		mmdc_sw_read_calib(ch, 0x00550000);
		mmdc_sw_read_calib(ch, 0x55000000);
		mmdc_sw_read_calib(ch, 0x00ffff00);
		mmdc_sw_read_calib(ch, 0xff0000ff);
		mmdc_sw_read_calib(ch, 0x55aaaa55);
		mmdc_sw_read_calib(ch, 0xaa5555aa);

		for (s = 1; s; s <<= 1)
			mmdc_sw_read_calib(ch, s);
	}

	printk("ch0 total lower: %d upper: %d avg: 0x%02x\n",
			total_lower[0], total_upper[0],
			(total_lower[0] + total_upper[0]) / 2);
	printk("ch1 total lower: %d upper: %d avg: 0x%02x\n",
			total_lower[1], total_upper[1],
			(total_lower[1] + total_upper[1]) / 2);

	mmdc_set_dqs(0x40);

	total_lower[0] = 0;
	total_lower[1] = 0;
	total_upper[0] = 0xff;
	total_upper[1] = 0xff;

	for (ch = 0; ch < 2; ch++) {
		mmdc_sw_write_calib(ch, 0x00000055);
		mmdc_sw_write_calib(ch, 0x00005500);
		mmdc_sw_write_calib(ch, 0x00550000);
		mmdc_sw_write_calib(ch, 0x55000000);
		mmdc_sw_write_calib(ch, 0x00ffff00);
		mmdc_sw_write_calib(ch, 0xff0000ff);
		mmdc_sw_write_calib(ch, 0x55aaaa55);
		mmdc_sw_write_calib(ch, 0xaa5555aa);

		for (s = 1; s; s <<= 1)
			mmdc_sw_write_calib(ch, s);
	}

	printk("ch0 total lower: %d upper: %d avg: 0x%02x\n",
			total_lower[0], total_upper[0],
			(total_lower[0] + total_upper[0]) / 2);
	printk("ch1 total lower: %d upper: %d avg: 0x%02x\n",
			total_lower[1], total_upper[1],
			(total_lower[1] + total_upper[1]) / 2);

	return 0;
}

#endif /* MMDC_SOFTWARE_CALIBRATION */

/* Configure MX6SX mmdc iomux */
void mx6sx_dram_iocfg(unsigned width,
		      const struct mx6sx_iomux_ddr_regs *ddr,
		      const struct mx6sx_iomux_grp_regs *grp)
{
	struct mx6sx_iomux_ddr_regs *mx6_ddr_iomux;
	struct mx6sx_iomux_grp_regs *mx6_grp_iomux;

	mx6_ddr_iomux = (struct mx6sx_iomux_ddr_regs *)MX6SX_IOM_DDR_BASE;
	mx6_grp_iomux = (struct mx6sx_iomux_grp_regs *)MX6SX_IOM_GRP_BASE;

	/* DDR IO TYPE */
	writel(grp->grp_ddr_type, &mx6_grp_iomux->grp_ddr_type);
	writel(grp->grp_ddrpke, &mx6_grp_iomux->grp_ddrpke);

	/* CLOCK */
	writel(ddr->dram_sdclk_0, &mx6_ddr_iomux->dram_sdclk_0);

	/* ADDRESS */
	writel(ddr->dram_cas, &mx6_ddr_iomux->dram_cas);
	writel(ddr->dram_ras, &mx6_ddr_iomux->dram_ras);
	writel(grp->grp_addds, &mx6_grp_iomux->grp_addds);

	/* Control */
	writel(ddr->dram_reset, &mx6_ddr_iomux->dram_reset);
	writel(ddr->dram_sdba2, &mx6_ddr_iomux->dram_sdba2);
	writel(ddr->dram_sdcke0, &mx6_ddr_iomux->dram_sdcke0);
	writel(ddr->dram_sdcke1, &mx6_ddr_iomux->dram_sdcke1);
	writel(ddr->dram_odt0, &mx6_ddr_iomux->dram_odt0);
	writel(ddr->dram_odt1, &mx6_ddr_iomux->dram_odt1);
	writel(grp->grp_ctlds, &mx6_grp_iomux->grp_ctlds);

	/* Data Strobes */
	writel(grp->grp_ddrmode_ctl, &mx6_grp_iomux->grp_ddrmode_ctl);
	writel(ddr->dram_sdqs0, &mx6_ddr_iomux->dram_sdqs0);
	writel(ddr->dram_sdqs1, &mx6_ddr_iomux->dram_sdqs1);
	if (width >= 32) {
		writel(ddr->dram_sdqs2, &mx6_ddr_iomux->dram_sdqs2);
		writel(ddr->dram_sdqs3, &mx6_ddr_iomux->dram_sdqs3);
	}

	/* Data */
	writel(grp->grp_ddrmode, &mx6_grp_iomux->grp_ddrmode);
	writel(grp->grp_b0ds, &mx6_grp_iomux->grp_b0ds);
	writel(grp->grp_b1ds, &mx6_grp_iomux->grp_b1ds);
	if (width >= 32) {
		writel(grp->grp_b2ds, &mx6_grp_iomux->grp_b2ds);
		writel(grp->grp_b3ds, &mx6_grp_iomux->grp_b3ds);
	}
	writel(ddr->dram_dqm0, &mx6_ddr_iomux->dram_dqm0);
	writel(ddr->dram_dqm1, &mx6_ddr_iomux->dram_dqm1);
	if (width >= 32) {
		writel(ddr->dram_dqm2, &mx6_ddr_iomux->dram_dqm2);
		writel(ddr->dram_dqm3, &mx6_ddr_iomux->dram_dqm3);
	}
}

/* Configure MX6DQ mmdc iomux */
void mx6dq_dram_iocfg(unsigned width,
		      const struct mx6dq_iomux_ddr_regs *ddr,
		      const struct mx6dq_iomux_grp_regs *grp)
{
	volatile struct mx6dq_iomux_ddr_regs *mx6_ddr_iomux;
	volatile struct mx6dq_iomux_grp_regs *mx6_grp_iomux;

	mx6_ddr_iomux = (struct mx6dq_iomux_ddr_regs *)MX6DQ_IOM_DDR_BASE;
	mx6_grp_iomux = (struct mx6dq_iomux_grp_regs *)MX6DQ_IOM_GRP_BASE;

	/* DDR IO Type */
	mx6_grp_iomux->grp_ddr_type = grp->grp_ddr_type;
	mx6_grp_iomux->grp_ddrpke = grp->grp_ddrpke;

	/* Clock */
	mx6_ddr_iomux->dram_sdclk_0 = ddr->dram_sdclk_0;
	mx6_ddr_iomux->dram_sdclk_1 = ddr->dram_sdclk_1;

	/* Address */
	mx6_ddr_iomux->dram_cas = ddr->dram_cas;
	mx6_ddr_iomux->dram_ras = ddr->dram_ras;
	mx6_grp_iomux->grp_addds = grp->grp_addds;

	/* Control */
	mx6_ddr_iomux->dram_reset = ddr->dram_reset;
	mx6_ddr_iomux->dram_sdcke0 = ddr->dram_sdcke0;
	mx6_ddr_iomux->dram_sdcke1 = ddr->dram_sdcke1;
	mx6_ddr_iomux->dram_sdba2 = ddr->dram_sdba2;
	mx6_ddr_iomux->dram_sdodt0 = ddr->dram_sdodt0;
	mx6_ddr_iomux->dram_sdodt1 = ddr->dram_sdodt1;
	mx6_grp_iomux->grp_ctlds = grp->grp_ctlds;

	/* Data Strobes */
	mx6_grp_iomux->grp_ddrmode_ctl = grp->grp_ddrmode_ctl;
	mx6_ddr_iomux->dram_sdqs0 = ddr->dram_sdqs0;
	mx6_ddr_iomux->dram_sdqs1 = ddr->dram_sdqs1;
	if (width >= 32) {
		mx6_ddr_iomux->dram_sdqs2 = ddr->dram_sdqs2;
		mx6_ddr_iomux->dram_sdqs3 = ddr->dram_sdqs3;
	}
	if (width >= 64) {
		mx6_ddr_iomux->dram_sdqs4 = ddr->dram_sdqs4;
		mx6_ddr_iomux->dram_sdqs5 = ddr->dram_sdqs5;
		mx6_ddr_iomux->dram_sdqs6 = ddr->dram_sdqs6;
		mx6_ddr_iomux->dram_sdqs7 = ddr->dram_sdqs7;
	}

	/* Data */
	mx6_grp_iomux->grp_ddrmode = grp->grp_ddrmode;
	mx6_grp_iomux->grp_b0ds = grp->grp_b0ds;
	mx6_grp_iomux->grp_b1ds = grp->grp_b1ds;
	if (width >= 32) {
		mx6_grp_iomux->grp_b2ds = grp->grp_b2ds;
		mx6_grp_iomux->grp_b3ds = grp->grp_b3ds;
	}
	if (width >= 64) {
		mx6_grp_iomux->grp_b4ds = grp->grp_b4ds;
		mx6_grp_iomux->grp_b5ds = grp->grp_b5ds;
		mx6_grp_iomux->grp_b6ds = grp->grp_b6ds;
		mx6_grp_iomux->grp_b7ds = grp->grp_b7ds;
	}
	mx6_ddr_iomux->dram_dqm0 = ddr->dram_dqm0;
	mx6_ddr_iomux->dram_dqm1 = ddr->dram_dqm1;
	if (width >= 32) {
		mx6_ddr_iomux->dram_dqm2 = ddr->dram_dqm2;
		mx6_ddr_iomux->dram_dqm3 = ddr->dram_dqm3;
	}
	if (width >= 64) {
		mx6_ddr_iomux->dram_dqm4 = ddr->dram_dqm4;
		mx6_ddr_iomux->dram_dqm5 = ddr->dram_dqm5;
		mx6_ddr_iomux->dram_dqm6 = ddr->dram_dqm6;
		mx6_ddr_iomux->dram_dqm7 = ddr->dram_dqm7;
	}
}

/* Configure MX6SDL mmdc iomux */
void mx6sdl_dram_iocfg(unsigned width,
		       const struct mx6sdl_iomux_ddr_regs *ddr,
		       const struct mx6sdl_iomux_grp_regs *grp)
{
	volatile struct mx6sdl_iomux_ddr_regs *mx6_ddr_iomux;
	volatile struct mx6sdl_iomux_grp_regs *mx6_grp_iomux;

	mx6_ddr_iomux = (struct mx6sdl_iomux_ddr_regs *)MX6SDL_IOM_DDR_BASE;
	mx6_grp_iomux = (struct mx6sdl_iomux_grp_regs *)MX6SDL_IOM_GRP_BASE;

	/* DDR IO Type */
	mx6_grp_iomux->grp_ddr_type = grp->grp_ddr_type;
	mx6_grp_iomux->grp_ddrpke = grp->grp_ddrpke;

	/* Clock */
	mx6_ddr_iomux->dram_sdclk_0 = ddr->dram_sdclk_0;
	mx6_ddr_iomux->dram_sdclk_1 = ddr->dram_sdclk_1;

	/* Address */
	mx6_ddr_iomux->dram_cas = ddr->dram_cas;
	mx6_ddr_iomux->dram_ras = ddr->dram_ras;
	mx6_grp_iomux->grp_addds = grp->grp_addds;

	/* Control */
	mx6_ddr_iomux->dram_reset = ddr->dram_reset;
	mx6_ddr_iomux->dram_sdcke0 = ddr->dram_sdcke0;
	mx6_ddr_iomux->dram_sdcke1 = ddr->dram_sdcke1;
	mx6_ddr_iomux->dram_sdba2 = ddr->dram_sdba2;
	mx6_ddr_iomux->dram_sdodt0 = ddr->dram_sdodt0;
	mx6_ddr_iomux->dram_sdodt1 = ddr->dram_sdodt1;
	mx6_grp_iomux->grp_ctlds = grp->grp_ctlds;

	/* Data Strobes */
	mx6_grp_iomux->grp_ddrmode_ctl = grp->grp_ddrmode_ctl;
	mx6_ddr_iomux->dram_sdqs0 = ddr->dram_sdqs0;
	mx6_ddr_iomux->dram_sdqs1 = ddr->dram_sdqs1;
	if (width >= 32) {
		mx6_ddr_iomux->dram_sdqs2 = ddr->dram_sdqs2;
		mx6_ddr_iomux->dram_sdqs3 = ddr->dram_sdqs3;
	}
	if (width >= 64) {
		mx6_ddr_iomux->dram_sdqs4 = ddr->dram_sdqs4;
		mx6_ddr_iomux->dram_sdqs5 = ddr->dram_sdqs5;
		mx6_ddr_iomux->dram_sdqs6 = ddr->dram_sdqs6;
		mx6_ddr_iomux->dram_sdqs7 = ddr->dram_sdqs7;
	}

	/* Data */
	mx6_grp_iomux->grp_ddrmode = grp->grp_ddrmode;
	mx6_grp_iomux->grp_b0ds = grp->grp_b0ds;
	mx6_grp_iomux->grp_b1ds = grp->grp_b1ds;
	if (width >= 32) {
		mx6_grp_iomux->grp_b2ds = grp->grp_b2ds;
		mx6_grp_iomux->grp_b3ds = grp->grp_b3ds;
	}
	if (width >= 64) {
		mx6_grp_iomux->grp_b4ds = grp->grp_b4ds;
		mx6_grp_iomux->grp_b5ds = grp->grp_b5ds;
		mx6_grp_iomux->grp_b6ds = grp->grp_b6ds;
		mx6_grp_iomux->grp_b7ds = grp->grp_b7ds;
	}
	mx6_ddr_iomux->dram_dqm0 = ddr->dram_dqm0;
	mx6_ddr_iomux->dram_dqm1 = ddr->dram_dqm1;
	if (width >= 32) {
		mx6_ddr_iomux->dram_dqm2 = ddr->dram_dqm2;
		mx6_ddr_iomux->dram_dqm3 = ddr->dram_dqm3;
	}
	if (width >= 64) {
		mx6_ddr_iomux->dram_dqm4 = ddr->dram_dqm4;
		mx6_ddr_iomux->dram_dqm5 = ddr->dram_dqm5;
		mx6_ddr_iomux->dram_dqm6 = ddr->dram_dqm6;
		mx6_ddr_iomux->dram_dqm7 = ddr->dram_dqm7;
	}
}

static void __udelay(int us)
{
	volatile int i;

	for (i = 0; i < us * 1000; i++);
}

/*
 * Configure mx6 mmdc registers based on:
 *  - board-specific memory configuration
 *  - board-specific calibration data
 *  - ddr3 chip details
 *
 * The various calculations here are derived from the Freescale
 * i.Mx6DQSDL DDR3 Script Aid spreadsheet (DOC-94917) designed to generate MMDC
 * configuration registers based on memory system and memory chip parameters.
 *
 * The defaults here are those which were specified in the spreadsheet.
 * For details on each register, refer to the IMX6DQRM and/or IMX6SDLRM
 * section titled MMDC initialization
 */
#define MR(val, ba, cmd, cs1) \
	((val << 16) | (1 << 15) | (cmd << 4) | (cs1 << 3) | ba)

#define ROUND(a,b)              (((a) + (b) - 1) & ~((b) - 1))

void mx6_dram_cfg(const struct mx6_ddr_sysinfo *sysinfo,
		  const struct mx6_mmdc_calibration *calib,
		  const struct mx6_ddr3_cfg *ddr3_cfg)
{
	volatile struct mmdc_p_regs *mmdc0;
	volatile struct mmdc_p_regs *mmdc1;

	u32 val;
	u8 tcke, tcksrx, tcksre, txpdll, taofpd, taonpd, trrd;
	u8 todtlon, taxpd, tanpd, tcwl, txp, tfaw, tcl;
	u8 todt_idle_off = 0x4; /* from DDR3 Script Aid spreadsheet */
	u16 trcd, trc, tras, twr, tmrd, trtp, trp, twtr, trfc, txs, txpr;
	u16 cs0_end;
	u16 tdllk = 0x1ff; /* DLL locking time: 512 cycles (JEDEC DDR3) */
	u8 coladdr;
	int clkper; /* clock period in picoseconds */
	int clock; /* clock freq in MHz */
	int cs;
	u16 mem_speed = ddr3_cfg->mem_speed;

	mmdc0 = (struct mmdc_p_regs *)MX6_MMDC_P0_BASE_ADDR;
	mmdc1 = (struct mmdc_p_regs *)MX6_MMDC_P1_BASE_ADDR;

	/* Limit mem_speed for MX6D/MX6Q */
	if (cpu_mx6_is_mx6q() || cpu_mx6_is_mx6d()) {
		if (mem_speed > 1066)
			mem_speed = 1066; /* 1066 MT/s */

		tcwl = 4;
	}
	/* Limit mem_speed for MX6S/MX6DL */
	else {
		if (mem_speed > 800)
			mem_speed = 800;  /* 800 MT/s */

		tcwl = 3;
	}

	clock = mem_speed / 2;
	/*
	 * Data rate of 1066 MT/s requires 533 MHz DDR3 clock, but MX6D/Q supports
	 * up to 528 MHz, so reduce the clock to fit chip specs
	 */
	if (cpu_mx6_is_mx6q() || cpu_mx6_is_mx6d()) {
		if (clock > 528)
			clock = 528; /* 528 MHz */
	}

	clkper = (1000 * 1000) / clock; /* pico seconds */
	todtlon = tcwl;
	taxpd = tcwl;
	tanpd = tcwl;

	switch (ddr3_cfg->density) {
	case 1: /* 1Gb per chip */
		trfc = DIV_ROUND_UP(110000, clkper) - 1;
		txs = DIV_ROUND_UP(120000, clkper) - 1;
		break;
	case 2: /* 2Gb per chip */
		trfc = DIV_ROUND_UP(160000, clkper) - 1;
		txs = DIV_ROUND_UP(170000, clkper) - 1;
		break;
	case 4: /* 4Gb per chip */
		trfc = DIV_ROUND_UP(260000, clkper) - 1;
		txs = DIV_ROUND_UP(270000, clkper) - 1;
		break;
	case 8: /* 8Gb per chip */
		trfc = DIV_ROUND_UP(350000, clkper) - 1;
		txs = DIV_ROUND_UP(360000, clkper) - 1;
		break;
	default:
		/* invalid density */
		pr_err("invalid chip density\n");
		hang();
		break;
	}
	txpr = txs;

	switch (mem_speed) {
	case 800:
		txp = DIV_ROUND_UP(max(3 * clkper, 7500), clkper) - 1;
		tcke = DIV_ROUND_UP(max(3 * clkper, 7500), clkper) - 1;
		if (ddr3_cfg->pagesz == 1) {
			tfaw = DIV_ROUND_UP(40000, clkper) - 1;
			trrd = DIV_ROUND_UP(max(4 * clkper, 10000), clkper) - 1;
		} else {
			tfaw = DIV_ROUND_UP(50000, clkper) - 1;
			trrd = DIV_ROUND_UP(max(4 * clkper, 10000), clkper) - 1;
		}
		break;
	case 1066:
		txp = DIV_ROUND_UP(max(3 * clkper, 7500), clkper) - 1;
		tcke = DIV_ROUND_UP(max(3 * clkper, 5625), clkper) - 1;
		if (ddr3_cfg->pagesz == 1) {
			tfaw = DIV_ROUND_UP(37500, clkper) - 1;
			trrd = DIV_ROUND_UP(max(4 * clkper, 7500), clkper) - 1;
		} else {
			tfaw = DIV_ROUND_UP(50000, clkper) - 1;
			trrd = DIV_ROUND_UP(max(4 * clkper, 10000), clkper) - 1;
		}
		break;
	default:
		pr_err("invalid memory speed\n");
		hang();
		break;
	}
	txpdll = DIV_ROUND_UP(max(10 * clkper, 24000), clkper) - 1;
	tcksre = DIV_ROUND_UP(max(5 * clkper, 10000), clkper);
	taonpd = DIV_ROUND_UP(2000, clkper) - 1;
	tcksrx = tcksre;
	taofpd = taonpd;
	twr  = DIV_ROUND_UP(15000, clkper) - 1;
	tmrd = DIV_ROUND_UP(max(12 * clkper, 15000), clkper) - 1;
	trc  = DIV_ROUND_UP(ddr3_cfg->trcmin, clkper / 10) - 1;
	tras = DIV_ROUND_UP(ddr3_cfg->trasmin, clkper / 10) - 1;
	tcl  = DIV_ROUND_UP(ddr3_cfg->trcd, clkper / 10) - 3;
	trp  = DIV_ROUND_UP(ddr3_cfg->trcd, clkper / 10) - 1;
	twtr = ROUND(max(4 * clkper, 7500) / clkper, 1) - 1;
	trcd = trp;
	trtp = twtr;
	cs0_end = 4 * sysinfo->cs_density - 1;

	debug("density:%d Gb (%d Gb per chip)\n",
	      sysinfo->cs_density, ddr3_cfg->density);
	debug("clock: %dMHz (%d ps)\n", clock, clkper);
	debug("memspd:%d\n", mem_speed);
	debug("tcke=%d\n", tcke);
	debug("tcksrx=%d\n", tcksrx);
	debug("tcksre=%d\n", tcksre);
	debug("taofpd=%d\n", taofpd);
	debug("taonpd=%d\n", taonpd);
	debug("todtlon=%d\n", todtlon);
	debug("tanpd=%d\n", tanpd);
	debug("taxpd=%d\n", taxpd);
	debug("trfc=%d\n", trfc);
	debug("txs=%d\n", txs);
	debug("txp=%d\n", txp);
	debug("txpdll=%d\n", txpdll);
	debug("tfaw=%d\n", tfaw);
	debug("tcl=%d\n", tcl);
	debug("trcd=%d\n", trcd);
	debug("trp=%d\n", trp);
	debug("trc=%d\n", trc);
	debug("tras=%d\n", tras);
	debug("twr=%d\n", twr);
	debug("tmrd=%d\n", tmrd);
	debug("tcwl=%d\n", tcwl);
	debug("tdllk=%d\n", tdllk);
	debug("trtp=%d\n", trtp);
	debug("twtr=%d\n", twtr);
	debug("trrd=%d\n", trrd);
	debug("txpr=%d\n", txpr);
	debug("cs0_end=%d\n", cs0_end);
	debug("ncs=%d\n", sysinfo->ncs);
	debug("Rtt_wr=%d\n", sysinfo->rtt_wr);
	debug("Rtt_nom=%d\n", sysinfo->rtt_nom);
	debug("SRT=%d\n", ddr3_cfg->SRT);
	debug("tcl=%d\n", tcl);
	debug("twr=%d\n", twr);

	/*
	 * board-specific configuration:
	 *  These values are determined empirically and vary per board layout
	 *  see:
	 *   appnote, ddr3 spreadsheet
	 */
	mmdc0->mpwldectrl0 = calib->p0_mpwldectrl0;
	mmdc0->mpwldectrl1 = calib->p0_mpwldectrl1;
	mmdc0->mpdgctrl0 = calib->p0_mpdgctrl0;
	mmdc0->mpdgctrl1 = calib->p0_mpdgctrl1;
	mmdc0->mprddlctl = calib->p0_mprddlctl;
	mmdc0->mpwrdlctl = calib->p0_mpwrdlctl;
	if (sysinfo->dsize > 1) {
		mmdc1->mpwldectrl0 = calib->p1_mpwldectrl0;
		mmdc1->mpwldectrl1 = calib->p1_mpwldectrl1;
		mmdc1->mpdgctrl0 = calib->p1_mpdgctrl0;
		mmdc1->mpdgctrl1 = calib->p1_mpdgctrl1;
		mmdc1->mprddlctl = calib->p1_mprddlctl;
		mmdc1->mpwrdlctl = calib->p1_mpwrdlctl;
	}

	/* Read data DQ Byte0-3 delay */
	mmdc0->mprddqby0dl = 0x33333333;
	mmdc0->mprddqby1dl = 0x33333333;
	if (sysinfo->dsize > 0) {
		mmdc0->mprddqby2dl = 0x33333333;
		mmdc0->mprddqby3dl = 0x33333333;
	}

	if (sysinfo->dsize > 1) {
		mmdc1->mprddqby0dl = 0x33333333;
		mmdc1->mprddqby1dl = 0x33333333;
		mmdc1->mprddqby2dl = 0x33333333;
		mmdc1->mprddqby3dl = 0x33333333;
	}

	/* MMDC Termination: rtt_nom:2 RZQ/2(120ohm), rtt_nom:1 RZQ/4(60ohm) */
	val = (sysinfo->rtt_nom == 2) ? 0x00011117 : 0x00022227;
	mmdc0->mpodtctrl = val;
	if (sysinfo->dsize > 1)
		mmdc1->mpodtctrl = val;

	/* complete calibration */
	val = (1 << 11); /* Force measurement on delay-lines */
	mmdc0->mpmur0 = val;
	if (sysinfo->dsize > 1)
		mmdc1->mpmur0 = val;

	/* Step 1: configuration request */
	mmdc0->mdscr = (u32)(1 << 15); /* config request */

	/* Step 2: Timing configuration */
	mmdc0->mdcfg0 = (trfc << 24) | (txs << 16) | (txp << 13) |
			(txpdll << 9) | (tfaw << 4) | tcl;
	mmdc0->mdcfg1 = (trcd << 29) | (trp << 26) | (trc << 21) |
			(tras << 16) | (1 << 15) /* trpa */ |
			(twr << 9) | (tmrd << 5) | tcwl;
	mmdc0->mdcfg2 = (tdllk << 16) | (trtp << 6) | (twtr << 3) | trrd;
	mmdc0->mdotc = (taofpd << 27) | (taonpd << 24) | (tanpd << 20) |
		       (taxpd << 16) | (todtlon << 12) | (todt_idle_off << 4);
	mmdc0->mdasp = cs0_end; /* CS addressing */

	/* Step 3: Configure DDR type */
	mmdc0->mdmisc = (sysinfo->cs1_mirror << 19) | (sysinfo->walat << 16) |
			(sysinfo->bi_on << 12) | (sysinfo->mif3_mode << 9) |
			(sysinfo->ralat << 6);

	/* Step 4: Configure delay while leaving reset */
	mmdc0->mdor = (txpr << 16) | (sysinfo->sde_to_rst << 8) |
		      (sysinfo->rst_to_cke << 0);

	/* Step 5: Configure DDR physical parameters (density and burst len) */
	coladdr = ddr3_cfg->coladdr;
	if (ddr3_cfg->coladdr == 8)		/* 8-bit COL is 0x3 */
		coladdr += 4;
	else if (ddr3_cfg->coladdr == 12)	/* 12-bit COL is 0x4 */
		coladdr += 1;
	mmdc0->mdctl =  (ddr3_cfg->rowaddr - 11) << 24 |	/* ROW */
			(coladdr - 9) << 20 |			/* COL */
			(1 << 19) |		/* Burst Length = 8 for DDR3 */
			(sysinfo->dsize << 16);		/* DDR data bus size */

	/* Step 6: Perform ZQ calibration */
	val = 0xa1390001; /* one-time HW ZQ calib */
	mmdc0->mpzqhwctrl = val;
	if (sysinfo->dsize > 1)
		mmdc1->mpzqhwctrl = val;

	/* Step 7: Enable MMDC with desired chip select */
	mmdc0->mdctl |= (1 << 31) |			     /* SDE_0 for CS0 */
			((sysinfo->ncs == 2) ? 1 : 0) << 30; /* SDE_1 for CS1 */

	/* Step 8: Write Mode Registers to Init DDR3 devices */
	for (cs = 0; cs < sysinfo->ncs; cs++) {
		/* MR2 */
		val = (sysinfo->rtt_wr & 3) << 9 | (ddr3_cfg->SRT & 1) << 7 |
		      ((tcwl - 3) & 3) << 3;
		debug("MR2 CS%d: 0x%08x\n", cs, (u32)MR(val, 2, 3, cs));
		mmdc0->mdscr = MR(val, 2, 3, cs);
		/* MR3 */
		debug("MR3 CS%d: 0x%08x\n", cs, (u32)MR(0, 3, 3, cs));
		mmdc0->mdscr = MR(0, 3, 3, cs);
		/* MR1 */
		val = ((sysinfo->rtt_nom & 1) ? 1 : 0) << 2 |
		      ((sysinfo->rtt_nom & 2) ? 1 : 0) << 6;
		debug("MR1 CS%d: 0x%08x\n", cs, (u32)MR(val, 1, 3, cs));
		mmdc0->mdscr = MR(val, 1, 3, cs);
		/* MR0 */
		val = ((tcl - 1) << 4) |	/* CAS */
		      (1 << 8)   |		/* DLL Reset */
		      ((twr - 3) << 9) |	/* Write Recovery */
		      (sysinfo->pd_fast_exit << 12); /* Precharge PD PLL on */
		debug("MR0 CS%d: 0x%08x\n", cs, (u32)MR(val, 0, 3, cs));
		mmdc0->mdscr = MR(val, 0, 3, cs);
		/* ZQ calibration */
		val = (1 << 10);
		mmdc0->mdscr = MR(val, 0, 4, cs);
	}

	/* Step 10: Power down control and self-refresh */
	mmdc0->mdpdc = (tcke & 0x7) << 16 |
			5            << 12 |  /* PWDT_1: 256 cycles */
			5            <<  8 |  /* PWDT_0: 256 cycles */
			1            <<  6 |  /* BOTH_CS_PD */
			(tcksrx & 0x7) << 3 |
			(tcksre & 0x7);
	if (!sysinfo->pd_fast_exit)
		mmdc0->mdpdc |= (1 << 7); /* SLOW_PD */
	mmdc0->mapsr = 0x00001006; /* ADOPT power down enabled */

	/* Step 11: Configure ZQ calibration: one-time and periodic 1ms */
	val = 0xa1390003;
	mmdc0->mpzqhwctrl = val;
	if (sysinfo->dsize > 1)
		mmdc1->mpzqhwctrl = val;

	/* Step 12: Configure and activate periodic refresh */
	mmdc0->mdref = (1 << 14) | /* REF_SEL: Periodic refresh cycle: 32kHz */
		       (7 << 11);  /* REFR: Refresh Rate - 8 refreshes */

	/* Step 13: Deassert config request - init complete */
	mmdc0->mdscr = 0x00000000;

	/* wait for auto-ZQ calibration to complete */
	__udelay(100);
}

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
#include <mach/imx6-regs.h>
#include <mach/imx6-mmdc.h>

#define P0_IPS (void __iomem *)MX6_MMDC_P0_BASE_ADDR
#define P1_IPS (void __iomem *)MX6_MMDC_P1_BASE_ADDR

#define MDCTL		0x000
#define MDPDC		0x004
#define MDSCR		0x01c
#define MDMISC		0x018
#define MDREF		0x020
#define MAPSR		0x404
#define MPZQHWCTRL	0x800
#define MPWLGCR		0x808
#define MPWLDECTRL0	0x80c
#define MPWLDECTRL1	0x810
#define MPPDCMPR1	0x88c
#define MPSWDAR		0x894
#define MPRDDLCTL	0x848
#define MPMUR		0x8b8
#define MPDGCTRL0	0x83c
#define MPDGCTRL1	0x840
#define MPRDDLCTL	0x848
#define MPWRDLCTL	0x850
#define MPRDDLHWCTL	0x860
#define MPWRDLHWCTL	0x864
#define MPDGHWST0	0x87c
#define MPDGHWST1	0x880
#define MPDGHWST2	0x884
#define MPDGHWST3	0x888

#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5a8)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5b0)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x524)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x51c)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS4	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x518)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS5	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x50c)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS6	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5b8)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS7	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5c0)

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

	pr_debug("MMDC registers updated from calibration \n");
	pr_debug("\nRead DQS Gating calibration\n");
	pr_debug("MPDGCTRL0 PHY0 (0x021b083c) = 0x%08X\n", readl(P0_IPS + MPDGCTRL0));
	pr_debug("MPDGCTRL1 PHY0 (0x021b0840) = 0x%08X\n", readl(P0_IPS + MPDGCTRL1));
	pr_debug("MPDGCTRL0 PHY1 (0x021b483c) = 0x%08X\n", readl(P1_IPS + MPDGCTRL0));
	pr_debug("MPDGCTRL1 PHY1 (0x021b4840) = 0x%08X\n", readl(P1_IPS + MPDGCTRL1));
	pr_debug("\nRead calibration\n");
	pr_debug("MPRDDLCTL PHY0 (0x021b0848) = 0x%08X\n", readl(P0_IPS + MPRDDLCTL));
	pr_debug("MPRDDLCTL PHY1 (0x021b4848) = 0x%08X\n", readl(P1_IPS + MPRDDLCTL));
	pr_debug("\nWrite calibration\n");
	pr_debug("MPWRDLCTL PHY0 (0x021b0850) = 0x%08X\n", readl(P0_IPS + MPWRDLCTL));
	pr_debug("MPWRDLCTL PHY1 (0x021b4850) = 0x%08X\n", readl(P1_IPS + MPWRDLCTL));
	pr_debug("\n");
	/*
	 * registers below are for debugging purposes
	 * these print out the upper and lower boundaries captured during read DQS gating calibration
	 */
	pr_debug("Status registers, upper and lower bounds, for read DQS gating. \n");
	pr_debug("MPDGHWST0 PHY0 (0x021b087c) = 0x%08X\n", readl(P0_IPS + MPDGHWST0));
	pr_debug("MPDGHWST1 PHY0 (0x021b0880) = 0x%08X\n", readl(P0_IPS + MPDGHWST1));
	pr_debug("MPDGHWST2 PHY0 (0x021b0884) = 0x%08X\n", readl(P0_IPS + MPDGHWST2));
	pr_debug("MPDGHWST3 PHY0 (0x021b0888) = 0x%08X\n", readl(P0_IPS + MPDGHWST3));
	pr_debug("MPDGHWST0 PHY1 (0x021b487c) = 0x%08X\n", readl(P1_IPS + MPDGHWST0));
	pr_debug("MPDGHWST1 PHY1 (0x021b4880) = 0x%08X\n", readl(P1_IPS + MPDGHWST1));
	pr_debug("MPDGHWST2 PHY1 (0x021b4884) = 0x%08X\n", readl(P1_IPS + MPDGHWST2));
	pr_debug("MPDGHWST3 PHY1 (0x021b4888) = 0x%08X\n", readl(P1_IPS + MPDGHWST3));

	return errorcount;
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

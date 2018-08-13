/*
 * Copyright (C) 2014-2016 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <io.h>
#include <debug_ll.h>
#include <mach/generic.h>
#include <mach/arria10-sdram.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-reset-manager.h>


/* FAWBANK - Number of Bank of a given device involved in the FAW period. */
#define ARRIA10_SDR_ACTIVATE_FAWBANK	(0x1)

#define ARRIA10_EMIF_RST	BIT(31)
#define ARRIA10_OCT_CAL_REQ	BIT(30)
#define ARRIA10_OCT_CAL_ACK	31

#define ARRIA10_NIOS_OCT_DONE	BIT(7)
#define ARRIA10_NIOS_OCT_ACK	7

/* Engineering sample silicon */
#define ARRIA10_ES_SILICON_VER	0x00010001

#define DDR_REG_SEQ2CORE        0xFFD0507C
#define DDR_REG_CORE2SEQ        0xFFD05078
#define DDR_REG_GPOUT           0xFFD03010
#define DDR_REG_GPIN            0xFFD03014
#define DDR_MAX_TRIES		0x00100000
#define IO48_MMR_DRAMSTS	0xFFCFA0EC
#define IO48_MMR_NIOS2_RESERVE0	0xFFCFA110
#define IO48_MMR_NIOS2_RESERVE1	0xFFCFA114
#define IO48_MMR_NIOS2_RESERVE2	0xFFCFA118

#define SEQ2CORE_MASK		0xF
#define CORE2SEQ_INT_REQ	0xF
#define SEQ2CORE_INT_RESP_BIT	3

#define DDR_ECC_DMA_SIZE	1500
#define DDR_READ_LATENCY_DELAY	40

#define ARRIA_DDR_CONFIG(A, B, C, R)	((A<<24)|(B<<16)|(C<<8)|R)
/* The followring are the supported configurations */
uint32_t ddr_config[] = {
	/* Chip - Row - Bank - Column Style */
	/* All Types */
	ARRIA_DDR_CONFIG(0, 3, 10, 12),
	ARRIA_DDR_CONFIG(0, 3, 10, 13),
	ARRIA_DDR_CONFIG(0, 3, 10, 14),
	ARRIA_DDR_CONFIG(0, 3, 10, 15),
	ARRIA_DDR_CONFIG(0, 3, 10, 16),
	ARRIA_DDR_CONFIG(0, 3, 10, 17),
	/* LPDDR x16 */
	ARRIA_DDR_CONFIG(0, 3, 11, 14),
	ARRIA_DDR_CONFIG(0, 3, 11, 15),
	ARRIA_DDR_CONFIG(0, 3, 11, 16),
	ARRIA_DDR_CONFIG(0, 3, 12, 15),
	/* DDR4 Only */
	ARRIA_DDR_CONFIG(0, 4, 10, 14),
	ARRIA_DDR_CONFIG(0, 4, 10, 15),
	ARRIA_DDR_CONFIG(0, 4, 10, 16),
	ARRIA_DDR_CONFIG(0, 4, 10, 17),	/* 14 */
	/* Chip - Bank - Row - Column Style */
	ARRIA_DDR_CONFIG(1, 3, 10, 12),
	ARRIA_DDR_CONFIG(1, 3, 10, 13),
	ARRIA_DDR_CONFIG(1, 3, 10, 14),
	ARRIA_DDR_CONFIG(1, 3, 10, 15),
	ARRIA_DDR_CONFIG(1, 3, 10, 16),
	ARRIA_DDR_CONFIG(1, 3, 10, 17),
	ARRIA_DDR_CONFIG(1, 3, 11, 14),
	ARRIA_DDR_CONFIG(1, 3, 11, 15),
	ARRIA_DDR_CONFIG(1, 3, 11, 16),
	ARRIA_DDR_CONFIG(1, 3, 12, 15),
	/* DDR4 Only */
	ARRIA_DDR_CONFIG(1, 4, 10, 14),
	ARRIA_DDR_CONFIG(1, 4, 10, 15),
	ARRIA_DDR_CONFIG(1, 4, 10, 16),
	ARRIA_DDR_CONFIG(1, 4, 10, 17),
};
#define DDR_CONFIG_ELEMENTS	ARRAY_SIZE(ddr_config)

static int match_ddr_conf(uint32_t ddr_conf)
{
	int i;

	for (i = 0; i < DDR_CONFIG_ELEMENTS; i++) {
		if (ddr_conf == ddr_config[i])
			return i;
	}
	return 0;
}

/* Check whether SDRAM is successfully Calibrated */
static int is_sdram_cal_success(void)
{
	return readl(ARRIA10_ECC_HMC_OCP_DDRCALSTAT);
}

static unsigned char ddr_get_bit(uint32_t ereg, unsigned char bit)
{
	unsigned int reg = readl(ereg);

	return (reg & (1 << bit)) ? 1 : 0;
}

static unsigned char ddr_wait_bit(uint32_t ereg, uint32_t bit,
				  uint32_t expected, uint32_t timeout_usec)
{
	unsigned int tmr;

	for (tmr = 0; tmr < timeout_usec; tmr += 100) {
		__udelay(100);
		if (ddr_get_bit(ereg, bit) == expected)
			return 0;
	}

	return 1;
}

static void ddr_delay(uint32_t delay)
{
	int tmr;

	for (tmr = 0; tmr < delay; tmr++)
		__udelay(1000);
}

/*
 * Diagram of OCT Workaround:
 *
 * EMIF Core                     HPS Processor              OCT FSM
 * =================================================================
 *
 * seq2core      ==============>
 * [0x?????????]   OCT Request   [0xFFD0507C]
 *
 * core2seq
 * [0x?????????] <==============
 *                 OCT Ready     [0xFFD05078]
 *
 *                               [0xFFD03010] ============> Request
 *                                             OCT Request
 *
 *                               [0xFFD03014] <============ Ready
 *                                              OCT Ready
 * Signal definitions:
 *
 * seq2core[7] - OCT calibration request (act-high)
 * core2seq[7] - Signals OCT FSM is ready (active high)
 * gpout[31]   - EMIF Reset override (active low)
 * gpout[30]   - OCT calibration request (act-high)
 * gpin[31]    - OCT calibration ready (act-high)
 */

static int ddr_calibration_es_workaround(void)
{
	ddr_delay(500);
	/* Step 1 - Initiating Reset Sequence */
	clrbits_le32(DDR_REG_GPOUT, ARRIA10_EMIF_RST);
	ddr_delay(10);

	/* Step 2 - Clearing registers to EMIF core */
	writel(0, DDR_REG_CORE2SEQ);	/*Clear the HPS->NIOS COM reg.*/

	/* Step 3 - Clearing registers to OCT core */
	clrbits_le32(DDR_REG_GPOUT, ARRIA10_OCT_CAL_REQ);
	ddr_delay(5);

	/* Step 4 - Taking EMIF out of reset */
	setbits_le32(DDR_REG_GPOUT, ARRIA10_EMIF_RST);
	ddr_delay(10);

	/* Step 5 - Waiting for OCT circuitry to come out of reset */
	if (ddr_wait_bit(DDR_REG_GPIN, ARRIA10_OCT_CAL_ACK, 1, 1000000))
		return -1;

	/* Step 6 - Allowing EMIF to proceed with OCT calibration */
	setbits_le32(DDR_REG_CORE2SEQ, ARRIA10_NIOS_OCT_DONE);

	/* Step 7 - Waiting for EMIF request */
	if (ddr_wait_bit(DDR_REG_SEQ2CORE, ARRIA10_NIOS_OCT_ACK, 1, 2000000))
		return -2;

	/* Step 8 - Acknowledging EMIF OCT request */
	clrbits_le32(DDR_REG_CORE2SEQ, ARRIA10_NIOS_OCT_DONE);

	/* Step 9 - Waiting for EMIF response */
	if (ddr_wait_bit(DDR_REG_SEQ2CORE, ARRIA10_NIOS_OCT_ACK, 0, 2000000))
		return -3;

	/* Step 10 - Triggering OCT Calibration */
	setbits_le32(DDR_REG_GPOUT, ARRIA10_OCT_CAL_REQ);

	/* Step 11 - Waiting for OCT response */
	if (ddr_wait_bit(DDR_REG_GPIN, ARRIA10_OCT_CAL_ACK, 0, 1000))
		return -4;

	/* Step 12 - Clearing OCT Request bit */
	clrbits_le32(DDR_REG_GPOUT, ARRIA10_OCT_CAL_REQ);

	/* Step 13 - Waiting for OCT Engine */
	if (ddr_wait_bit(DDR_REG_GPIN, ARRIA10_OCT_CAL_ACK, 1, 200000))
		return -5;

	/* Step 14 - Proceeding with EMIF calibration */
	setbits_le32(DDR_REG_CORE2SEQ, ARRIA10_NIOS_OCT_DONE);

	ddr_delay(100);

	return 0;
}

static int emif_clear(void)
{
	uint32_t s2c;
	uint32_t i = DDR_MAX_TRIES;

	writel(0, DDR_REG_CORE2SEQ);
	do {
		ddr_delay(50);
		s2c = readl(DDR_REG_SEQ2CORE);
	} while ((s2c & SEQ2CORE_MASK) && (--i > 0));

	return !i;
}
static int emif_reset(void)
{
	uint32_t c2s, s2c;

	c2s = readl(DDR_REG_CORE2SEQ);
	s2c = readl(DDR_REG_SEQ2CORE);

	pr_debug("c2s=%08x s2c=%08x nr0=%08x nr1=%08x nr2=%08x dst=%08x\n",
		c2s, s2c, readl(IO48_MMR_NIOS2_RESERVE0),
		readl(IO48_MMR_NIOS2_RESERVE1),
		readl(IO48_MMR_NIOS2_RESERVE2),
		readl(IO48_MMR_DRAMSTS));

	if ((s2c & SEQ2CORE_MASK) && emif_clear()) {
		printf("failed emif_clear()\n");
		return -1;
	}

	writel(CORE2SEQ_INT_REQ, DDR_REG_CORE2SEQ);

	if (ddr_wait_bit(DDR_REG_SEQ2CORE, SEQ2CORE_INT_RESP_BIT, 0, 1000000)) {
		printf("emif_reset failed to see interrupt acknowledge\n");
		return -2;
	} else {
		printf("emif_reset interrupt acknowledged\n");
	}

	if (emif_clear()) {
		printf("emif_clear() failed\n");
		return -3;
	}
	pr_debug("emif_reset interrupt cleared\n");

	pr_debug("nr0=%08x nr1=%08x nr2=%08x\n",
		readl(IO48_MMR_NIOS2_RESERVE0),
		readl(IO48_MMR_NIOS2_RESERVE1),
		readl(IO48_MMR_NIOS2_RESERVE2));

	return 0;
}

static int arria10_ddr_setup(void)
{
	int i, j, retcode, ddr_setup_complete = 0;
	int chip_version = readl(ARRIA10_SYSMGR_SILICONID1);

	/* Try 3 times to do a calibration */
	for (i = 0; (i < 3) && !ddr_setup_complete; i++) {
		/* Only engineering sample needs calibration workaround */
		if (ARRIA10_ES_SILICON_VER == chip_version) {
			retcode = ddr_calibration_es_workaround();
			if (retcode) {
				printf("DDRCAL: Failure: %d\n", retcode);
				continue;
			}
		}

		/* A delay to wait for calibration bit to set */
		for (j = 0; (j < 10) && !ddr_setup_complete; j++) {
			ddr_delay(500);
			ddr_setup_complete = is_sdram_cal_success();
		}

		if (!ddr_setup_complete &&
			(ARRIA10_ES_SILICON_VER != chip_version)) {
			emif_reset();
		}
	}

	if (!ddr_setup_complete) {
		puts_ll("Error: Could Not Calibrate SDRAM\n");
		return -1;
	}

	return 0;
}

/* Function to startup the SDRAM*/
static int arria10_sdram_startup(void)
{
	uint32_t val;

	/* Release NOC ddr scheduler from reset */
	val = readl(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_BRGMODRST);
	val &= ~ARRIA10_RSTMGR_BRGMODRST_DDRSCH;
	writel(val, ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_BRGMODRST);

	/* Bringup the DDR (calibration and configuration) */
	return arria10_ddr_setup();
}

/* Function to initialize SDRAM MMR and NOC DDR scheduler*/
static void arria10_sdram_mmr_init(void)
{
	uint32_t update_value, io48_value;
	union ctrlcfg0_reg ctrlcfg0 =
		(union ctrlcfg0_reg)readl(ARRIA10_IO48_HMC_MMR_CTRLCFG0);
	union ctrlcfg1_reg ctrlcfg1 =
		(union ctrlcfg1_reg)readl(ARRIA10_IO48_HMC_MMR_CTRLCFG1);
	union dramaddrw_reg dramaddrw =
		(union dramaddrw_reg)readl(ARRIA10_IO48_HMC_MMR_DRAMADDRW);
	union caltiming0_reg caltim0 =
		(union caltiming0_reg)readl(ARRIA10_IO48_HMC_MMR_CALTIMING0);
	union caltiming1_reg caltim1 =
		(union caltiming1_reg)readl(ARRIA10_IO48_HMC_MMR_CALTIMING1);
	union caltiming2_reg caltim2 =
		(union caltiming2_reg)readl(ARRIA10_IO48_HMC_MMR_CALTIMING2);
	union caltiming3_reg caltim3 =
		(union caltiming3_reg)readl(ARRIA10_IO48_HMC_MMR_CALTIMING3);
	union caltiming4_reg caltim4 =
		(union caltiming4_reg)readl(ARRIA10_IO48_HMC_MMR_CALTIMING4);
	union caltiming9_reg caltim9 =
		(union caltiming9_reg)readl(ARRIA10_IO48_HMC_MMR_CALTIMING9);
	uint32_t ddrioctl;

	/*
	 * Configure the DDR IO size [0xFFCFB008]
	 * niosreserve0: Used to indicate DDR width &
	 *	bit[7:0] = Number of data bits (0x20 for 32bit)
	 *	bit[8]   = 1 if user-mode OCT is present
	 *	bit[9]   = 1 if warm reset compiled into EMIF Cal Code
	 *	bit[10]  = 1 if warm reset is on during generation in EMIF Cal
	 * niosreserve1: IP ADCDS version encoded as 16 bit value
	 *	bit[2:0] = Variant (0=not special,1=FAE beta, 2=Customer beta,
	 *			    3=EAP, 4-6 are reserved)
	 *	bit[5:3] = Service Pack # (e.g. 1)
	 *	bit[9:6] = Minor Release #
	 *	bit[14:10] = Major Release #
	 */
	if ((readl(ARRIA10_IO48_HMC_MMR_NIOSRESERVE1) >> 6) & 0x1FF) {
		update_value = readl(ARRIA10_IO48_HMC_MMR_NIOSRESERVE0);
		writel(((update_value & 0xFF) >> 5),
		       ARRIA10_ECC_HMC_OCP_DDRIOCTRL);
	}

	ddrioctl = readl(ARRIA10_ECC_HMC_OCP_DDRIOCTRL);

	/* Set the DDR Configuration [0xFFD12400] */
	io48_value = ARRIA_DDR_CONFIG(ctrlcfg1.cfg_addr_order,
				      (dramaddrw.cfg_bank_addr_width +
				       dramaddrw.cfg_bank_group_addr_width),
				      dramaddrw.cfg_col_addr_width,
				      dramaddrw.cfg_row_addr_width);

	update_value = match_ddr_conf(io48_value);
	if (update_value)
		writel(update_value, ARRIA10_NOC_DDR_T_MAIN_SCHEDULER_DDRCONF);

	/*
	 * Configure DDR timing [0xFFD1240C]
	 *  RDTOMISS = tRTP + tRP + tRCD - BL/2
	 *  WRTOMISS = WL + tWR + tRP + tRCD and
	 *    WL = RL + BL/2 + 2 - rd-to-wr ; tWR = 15ns  so...
	 *  First part of equation is in memory clock units so divide by 2
	 *  for HMC clock units. 1066MHz is close to 1ns so use 15 directly.
	 *  WRTOMISS = ((RL + BL/2 + 2 + tWR) >> 1)- rd-to-wr + tRP + tRCD
	 */
	update_value = (caltim2.cfg_rd_to_pch +  caltim4.cfg_pch_to_valid +
			caltim0.cfg_act_to_rdwr -
			(ctrlcfg0.cfg_ctrl_burst_len >> 2));
	io48_value = ((((readl(ARRIA10_IO48_HMC_MMR_DRAMTIMING0) &
			 ARRIA10_IO48_DRAMTIME_MEM_READ_LATENCY) + 2 + 15 +
			(ctrlcfg0.cfg_ctrl_burst_len >> 1)) >> 1) -
		      /* Up to here was in memory cycles so divide by 2 */
		      caltim1.cfg_rd_to_wr + caltim0.cfg_act_to_rdwr +
		      caltim4.cfg_pch_to_valid);

	writel(((caltim0.cfg_act_to_act <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_ACTTOACT_LSB) |
		(update_value <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_RDTOMISS_LSB) |
		(io48_value <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_WRTOMISS_LSB) |
		((ctrlcfg0.cfg_ctrl_burst_len >> 2) <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_BURSTLEN_LSB) |
		(caltim1.cfg_rd_to_wr <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_RDTOWR_LSB) |
		(caltim3.cfg_wr_to_rd <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_WRTORD_LSB) |
		(((ddrioctl == 1) ? 1 : 0) <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DDRTIMING_BWRATIO_LSB)),
	       ARRIA10_NOC_DDR_T_MAIN_SCHEDULER_DDRTIMING);

	/* Configure DDR mode [0xFFD12410] [precharge = 0] */
	writel(((ddrioctl ? 0 : 1) <<
		ARRIA10_NOC_MPU_DDR_T_SCHED_DDRMOD_BWRATIOEXTENDED_LSB),
	       ARRIA10_NOC_DDR_T_MAIN_SCHEDULER_DDRMODE);

	/* Configure the read latency [0xFFD12414] */
	writel(((readl(ARRIA10_IO48_HMC_MMR_DRAMTIMING0) &
		 ARRIA10_IO48_DRAMTIME_MEM_READ_LATENCY) >> 1) +
	       DDR_READ_LATENCY_DELAY,
	       ARRIA10_NOC_DDR_T_MAIN_SCHEDULER_READLATENCY);

	/*
	 * Configuring timing values concerning activate commands
	 * [0xFFD12438] [FAWBANK alway 1 because always 4 bank DDR]
	 */
	writel(((caltim0.cfg_act_to_act_db <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_ACTIVATE_RRD_LSB) |
		(caltim9.cfg_4_act_to_act <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_ACTIVATE_FAW_LSB) |
		(ARRIA10_SDR_ACTIVATE_FAWBANK <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_ACTIVATE_FAWBANK_LSB)),
	       ARRIA10_NOC_DDR_T_MAIN_SCHEDULER_ACTIVATE);

	/*
	 * Configuring timing values concerning device to device data bus
	 * ownership change [0xFFD1243C]
	 */
	writel(((caltim1.cfg_rd_to_rd_dc <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DEVTODEV_BUSRDTORD_LSB) |
		(caltim1.cfg_rd_to_wr_dc <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DEVTODEV_BUSRDTOWR_LSB) |
		(caltim3.cfg_wr_to_rd_dc <<
		 ARRIA10_NOC_MPU_DDR_T_SCHED_DEVTODEV_BUSWRTORD_LSB)),
	       ARRIA10_NOC_DDR_T_MAIN_SCHEDULER_DEVTODEV);

	/* Enable or disable the SDRAM ECC */
	if (ctrlcfg1.cfg_ctrl_enable_ecc) {
		setbits_le32(ARRIA10_ECC_HMC_OCP_MPR_ECCCTRL1,
			     (ARRIA10_ECC_HMC_OCP_ECCCTL_AWB_CNT_RST |
			      ARRIA10_ECC_HMC_OCP_ECCCTL_CNT_RST |
			      ARRIA10_ECC_HMC_OCP_ECCCTL_ECC_EN));
		clrbits_le32(ARRIA10_ECC_HMC_OCP_MPR_ECCCTRL1,
			     (ARRIA10_ECC_HMC_OCP_ECCCTL_AWB_CNT_RST |
			      ARRIA10_ECC_HMC_OCP_ECCCTL_CNT_RST));
		setbits_le32(ARRIA10_ECC_HMC_OCP_MPR_ECCCTRL2,
			     (ARRIA10_ECC_HMC_OCP_ECCCTL2_RMW_EN |
			      ARRIA10_ECC_HMC_OCP_ECCCTL2_AWB_EN));
	} else {
		clrbits_le32(ARRIA10_ECC_HMC_OCP_MPR_ECCCTRL1,
			     (ARRIA10_ECC_HMC_OCP_ECCCTL_AWB_CNT_RST |
			      ARRIA10_ECC_HMC_OCP_ECCCTL_CNT_RST |
			      ARRIA10_ECC_HMC_OCP_ECCCTL_ECC_EN));
		clrbits_le32(ARRIA10_ECC_HMC_OCP_MPR_ECCCTRL2,
			     (ARRIA10_ECC_HMC_OCP_ECCCTL2_RMW_EN |
			      ARRIA10_ECC_HMC_OCP_ECCCTL2_AWB_EN));
	}
}

static int arria10_sdram_firewall_setup(void)
{
	uint32_t mpu_en = 0;

	/* set to default state */
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_EN);
	writel(0x00000000, ARRIA10_NOC_FW_DDR_L3_DDR_SCR_ADDR + 0x00);

	writel(0xffff0000, ARRIA10_SDR_FW_MPU_FPGA_MPUREGION0ADDR);

	mpu_en |= ARRIA10_NOC_FW_DDR_MPU_MPUREG0EN;

	writel(mpu_en, ARRIA10_SDR_FW_MPU_FPGA_EN);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_MPUREGION1ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_MPUREGION2ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_MPUREGION3ADDR);
	writel(0xffff0000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM0REGION0ADDR);

	mpu_en |= ARRIA10_NOC_FW_DDR_MPU_MPUREG1EN;
	writel(mpu_en, ARRIA10_SDR_FW_MPU_FPGA_EN);

	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM0REGION1ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM0REGION2ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM0REGION3ADDR);
	writel(0xffff0000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM1REGION0ADDR);

	mpu_en |= ARRIA10_NOC_FW_DDR_MPU_MPUREG2EN;
	writel(mpu_en, ARRIA10_SDR_FW_MPU_FPGA_EN);

	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM1REGION1ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM1REGION2ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM1REGION3ADDR);
	writel(0xffff0000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM2REGION0ADDR);

	mpu_en |= ARRIA10_NOC_FW_DDR_MPU_MPUREG3EN;
	writel(mpu_en, ARRIA10_SDR_FW_MPU_FPGA_EN);

	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM2REGION1ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM2REGION2ADDR);
	writel(0x00000000, ARRIA10_SDR_FW_MPU_FPGA_FPGA2SDRAM2REGION3ADDR);

	writel(0xffff0000, ARRIA10_NOC_FW_DDR_L3_HPSREGION0ADDR);
	writel(ARRIA10_NOC_FW_DDR_L3_HPSREG0EN, ARRIA10_NOC_FW_DDR_L3_EN);

	return 0;
}

int arria10_ddr_calibration_sequence(void)
{
	/* Check to see if SDRAM cal was success */
	if (arria10_sdram_startup()) {
		puts_ll("DDRCAL: Failed\n");
		return -1;
	}

	puts_ll("DDRCAL: Success\n");

	/* initialize the MMR register */
	arria10_sdram_mmr_init();

	if (arria10_sdram_firewall_setup())
		puts_ll("FW: Error Configuring Firewall\n");

	puts_ll("SDRAM setup done\n");

	return 0;
}

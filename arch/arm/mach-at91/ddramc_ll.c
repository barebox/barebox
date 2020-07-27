// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2007, Stelian Pop <stelian.pop@leadtechdesign.com>
 * Copyright (c) 2007 Lead Tech Design <www.leadtechdesign.com>
 */

#include <linux/kconfig.h>
#include <asm/system.h>
#include <mach/at91_ddrsdrc.h>
#include <mach/ddramc.h>
#include <mach/early_udelay.h>

void at91_ddram_initialize(void __iomem *base_address,
			   void __iomem *ram_address,
			   struct at91_ddramc_register *ddramc_config)
{
	unsigned long ba_offset;
	unsigned long cr = 0;

	/* compute BA[] offset according to CR configuration */
	ba_offset = (ddramc_config->cr & AT91_DDRC2_NC) + 9;
	if ((ddramc_config->cr & AT91_DDRC2_DECOD) == AT91_DDRC2_DECOD_SEQUENTIAL)
		ba_offset += ((ddramc_config->cr & AT91_DDRC2_NR) >> 2) + 11;

	ba_offset += (ddramc_config->mdr & AT91_DDRC2_DBW) ? 1 : 2;

	/*
	 * Step 1: Program the memory device type into the Memory Device Register
	 */
	writel(ddramc_config->mdr, base_address + AT91_HDDRSDRC2_MDR);

	/*
	 * Step 2: Program the feature of DDR2-SDRAM device into
	 * the Timing Register, and into the Configuration Register
	 */
	writel(ddramc_config->cr, base_address + AT91_HDDRSDRC2_CR);

	writel(ddramc_config->t0pr, base_address + AT91_HDDRSDRC2_T0PR);
	writel(ddramc_config->t1pr, base_address + AT91_HDDRSDRC2_T1PR);
	writel(ddramc_config->t2pr, base_address + AT91_HDDRSDRC2_T2PR);

	/*
	 * Step 3: An NOP command is issued to the DDR2-SDRAM
	 */
	writel(AT91_DDRC2_MODE_NOP_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);
	/* Now, clocks which drive the DDR2-SDRAM device are enabled */

	/* A minimum pause wait 200 us is provided to precede any signal toggle.
	   (6 core cycles per iteration, core is at 396MHz: min 13340 loops) */
	early_udelay(200);

	/*
	 * Step 4:  An NOP command is issued to the DDR2-SDRAM
	 */
	writel(AT91_DDRC2_MODE_NOP_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);
	/* Now, CKE is driven high */
	/* wait 400 ns min */
	early_udelay(1);

	/*
	 * Step 5: An all banks precharge command is issued to the DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MODE_PRCGALL_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 6: An Extended Mode Register set(EMRS2) cycle is issued to chose between commercial or high
	 * temperature operations.
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1] is set to 1 and BA[0] is set to 0.
	 */
	writel(AT91_DDRC2_MODE_EXT_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x2 << ba_offset));

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 7: An Extended Mode Register set(EMRS3) cycle is issued
	 * to set the Extended Mode Register to "0".
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1] is set to 1 and BA[0] is set to 1.
	 */
	writel(AT91_DDRC2_MODE_EXT_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x3 << ba_offset));

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 8: An Extened Mode Register set(EMRS1) cycle is issued to enable DLL,
	 * and to program D.I.C(Output Driver Impedance Control)
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1] is set to 0 and BA[0] is set to 1.
	 */
	writel(AT91_DDRC2_MODE_EXT_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x1 << ba_offset));

	/* An additional 200 cycles of clock are required for locking DLL */
	early_udelay(1);

	/*
	 * Step 9: Program DLL field into the Configuration Register to high(Enable DLL reset)
	 */
	cr = readl(base_address + AT91_HDDRSDRC2_CR);
	writel(cr | AT91_DDRC2_ENABLE_RESET_DLL, base_address + AT91_HDDRSDRC2_CR);

	/*
	 * Step 10: A Mode Register set(MRS) cycle is issied to reset DLL.
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1:0] bits are set to 0.
	 */
	writel(AT91_DDRC2_MODE_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x0 << ba_offset));

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 11: An all banks precharge command is issued to the DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MODE_PRCGALL_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/* wait 400 ns min (not needed on certain DDR2 devices) */
	early_udelay(1);

	/*
	 * Step 12: Two auto-refresh (CBR) cycles are provided.
	 * Program the auto refresh command (CBR) into the Mode Register.
	 */
	writel(AT91_DDRC2_MODE_RFSH_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/* wait TRFC cycles min (135 ns min) extended to 400 ns */
	early_udelay(1);

	/* Set 2nd CBR */
	writel(AT91_DDRC2_MODE_RFSH_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/* wait TRFC cycles min (135 ns min) extended to 400 ns */
	early_udelay(1);

	/*
	 * Step 13: Program DLL field into the Configuration Register to low(Disable DLL reset).
	 */
	cr = readl(base_address + AT91_HDDRSDRC2_CR);
	writel(cr & ~AT91_DDRC2_ENABLE_RESET_DLL, base_address + AT91_HDDRSDRC2_CR);

	/*
	 * Step 14: A Mode Register set (MRS) cycle is issued to program
	 * the parameters of the DDR2-SDRAM devices, in particular CAS latency,
	 * burst length and to disable DDL reset.
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1:0] bits are set to 0.
	 */
	writel(AT91_DDRC2_MODE_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x0 << ba_offset));

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 15: Program OCD field into the Configuration Register
	 * to high (OCD calibration default).
	 */
	cr = readl(base_address + AT91_HDDRSDRC2_CR);
	writel(cr | AT91_DDRC2_OCD_DEFAULT, base_address + AT91_HDDRSDRC2_CR);

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 16: An Extended Mode Register set (EMRS1) cycle is issued to OCD default value.
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1] is set to 0 and BA[0] is set to 1.
	 */
	writel(AT91_DDRC2_MODE_EXT_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x1 << ba_offset));

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 17: Program OCD field into the Configuration Register
	 * to low (OCD calibration mode exit).
	 */
	cr = readl(base_address + AT91_HDDRSDRC2_CR);
	writel(cr & ~AT91_DDRC2_OCD_DEFAULT, base_address + AT91_HDDRSDRC2_CR);

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 18: An Extended Mode Register set (EMRS1) cycle is issued to enable OCD exit.
	 * Perform a write access to DDR2-SDRAM to acknowledge this command.
	 * The write address must be chosen so that BA[1] is set to 0 and BA[0] is set to 1.
	 */
	writel(AT91_DDRC2_MODE_EXT_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x1 << ba_offset));

	/* wait 2 cycles min (of tCK) = 15 ns min */
	early_udelay(1);

	/*
	 * Step 19: A Nornal mode command is provided.
	 */
	writel(AT91_DDRC2_MODE_NORMAL_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/*
	 * Step 20: Perform a write access to any DDR2-SDRAM address
	 */
	writel(0, ram_address);

	/*
	 * Step 21: Write the refresh rate into the count field in the Refresh Timer register.
	 */
	writel(ddramc_config->rtr, base_address + AT91_HDDRSDRC2_RTR);

	/*
	 * Now we are ready to work on the DDRSDR
	 *  wait for end of calibration
	 */
	early_udelay(10);
}

/* This initialization sequence is sama5d3 and sama5d4 LP-DDR2 specific */

void at91_lpddr2_sdram_initialize(void __iomem *base_address,
				  void __iomem *ram_address,
				  struct at91_ddramc_register *ddramc_config)
{
	unsigned long reg;

	writel(ddramc_config->lpddr2_lpr, base_address + AT91_MPDDRC_LPDDR2_LPR);

	writel(ddramc_config->tim_calr, base_address + AT91_MPDDRC_LPDDR2_TIM_CAL);

	/*
	 * Step 1: Program the memory device type.
	 */
	writel(ddramc_config->mdr, base_address + AT91_HDDRSDRC2_MDR);

	/*
	 * Step 2: Program the feature of the low-power DDR2-SDRAM device.
	 */
	writel(ddramc_config->cr, base_address + AT91_HDDRSDRC2_CR);

	writel(ddramc_config->t0pr, base_address + AT91_HDDRSDRC2_T0PR);
	writel(ddramc_config->t1pr, base_address + AT91_HDDRSDRC2_T1PR);
	writel(ddramc_config->t2pr, base_address + AT91_HDDRSDRC2_T2PR);

	/*
	 * Step 3: A NOP command is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MODE_NOP_CMD, base_address + AT91_HDDRSDRC2_MR);

	/*
	 * Step 3bis: Add memory barrier then Perform a write access to
	 * any low-power DDR2-SDRAM address to acknowledge the command.
	 */
	dmb();
	writel(0, ram_address);

	/*
	 * Step 4: A pause of at least 100 ns must be observed before
	 * a single toggle.
	 */
	early_udelay(1);

	/*
	 * Step 5: A NOP command is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MODE_NOP_CMD, base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 6: A pause of at least 200 us must be observed before a Reset
	 * Command.
	 */
	early_udelay(200);

	/*
	 * Step 7: A Reset command is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MRS(63) | AT91_DDRC2_MODE_LPDDR2_CMD,
		     base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 8: A pause of at least tINIT5 must be observed before issuing
	 * any commands.
	 */
	early_udelay(1);

	/*
	 * Step 9: A Calibration command is issued to the low-power DDR2-SDRAM.
	 */
	reg = readl(base_address + AT91_HDDRSDRC2_CR);
	reg &= ~AT91_DDRC2_ZQ;
	reg |= AT91_DDRC2_ZQ_RESET;
	writel(reg, base_address + AT91_HDDRSDRC2_CR);

	writel(AT91_DDRC2_MRS(10) | AT91_DDRC2_MODE_LPDDR2_CMD,
		     base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 9bis: The ZQ Calibration command is now issued.
	 * Program the type of calibration in the MPDDRC_CR: set the
	 * ZQ field to the SHORT value.
	 */
	reg = readl(base_address + AT91_HDDRSDRC2_CR);
	reg &= ~AT91_DDRC2_ZQ;
	reg |= AT91_DDRC2_ZQ_SHORT;
	writel(reg, base_address + AT91_HDDRSDRC2_CR);

	/*
	 * Step 10: A Mode Register Write command with 1 to the MRS field
	 * is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MRS(1) | AT91_DDRC2_MODE_LPDDR2_CMD,
		     base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 11: A Mode Register Write command with 2 to the MRS field
	 * is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MRS(2) | AT91_DDRC2_MODE_LPDDR2_CMD,
		     base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 12: A Mode Register Write command with 3 to the MRS field
	 * is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MRS(3) | AT91_DDRC2_MODE_LPDDR2_CMD,
		     base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 13: A Mode Register Write command with 16 to the MRS field
	 * is issued to the low-power DDR2-SDRAM.
	 */
	writel(AT91_DDRC2_MRS(16) | AT91_DDRC2_MODE_LPDDR2_CMD,
		     base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 14: A Normal Mode command is provided.
	 */
	writel(AT91_DDRC2_MODE_NORMAL_CMD, base_address + AT91_HDDRSDRC2_MR);
	dmb();
	writel(0, ram_address);

	/*
	 * Step 15: close the input buffers: error in documentation: no need.
	 */

	/*
	 * Step 16: Write the refresh rate into the COUNT field in the MPDDRC
	 * Refresh Timer Register.
	 */
	writel(ddramc_config->rtr, base_address + AT91_HDDRSDRC2_RTR);

	/*
	 * Now configure the CAL MR4 register.
	 */
	writel(ddramc_config->cal_mr4r, base_address + AT91_MPDDRC_LPDDR2_CAL_MR4);
}

void at91_lpddr1_sdram_initialize(void __iomem *base_address,
				  void __iomem *ram_address,
				  struct at91_ddramc_register *ddramc_config)
{
	unsigned long ba_offset;

	/* Compute BA[] offset according to CR configuration */
	ba_offset = (ddramc_config->cr & AT91_DDRC2_NC) + 8;
	if (!(ddramc_config->cr & AT91_DDRC2_DECOD_INTERLEAVED))
		ba_offset += ((ddramc_config->cr & AT91_DDRC2_NR) >> 2) + 11;

	ba_offset += (ddramc_config->mdr & AT91_DDRC2_DBW) ? 1 : 2;

	/*
	 * Step 1: Program the memory device type in the MPDDRC Memory Device Register
	 */
	writel(ddramc_config->mdr, base_address + AT91_HDDRSDRC2_MDR);

	/*
	 * Step 2: Program the features of the low-power DDR1-SDRAM device
	 * in the MPDDRC Configuration Register and in the MPDDRC Timing
	 * Parameter 0 Register/MPDDRC Timing Parameter 1 Register.
	 */
	writel(ddramc_config->cr, base_address + AT91_HDDRSDRC2_CR);

	writel(ddramc_config->t0pr, base_address + AT91_HDDRSDRC2_T0PR);
	writel(ddramc_config->t1pr, base_address + AT91_HDDRSDRC2_T1PR);
	writel(ddramc_config->t2pr, base_address + AT91_HDDRSDRC2_T2PR);

	/*
	 * Step 3: Program Temperature Compensated Self-refresh (TCR),
	 * Partial Array Self-refresh (PASR) and Drive Strength (DS) parameters
	 * in the MPDDRC Low-power Register.
	 */
	writel(ddramc_config->lpr, base_address + AT91_HDDRSDRC2_LPR);

	/*
	 * Step 4: A NOP command is issued to the low-power DDR1-SDRAM.
	 * Program the NOP command in the MPDDRC Mode Register (MPDDRC_MR).
	 * The clocks which drive the low-power DDR1-SDRAM device
	 * are now enabled.
	 */
	writel(AT91_DDRC2_MODE_NOP_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/*
	 * Step 5: A pause of at least 200 us must be observed before
	 * a signal toggle.
	 */
	early_udelay(200);

	/*
	 * Step 6: A NOP command is issued to the low-power DDR1-SDRAM.
	 * Program the NOP command in the MPDDRC_MR. calibration request is
	 * now made to the I/O pad.
	 */
	writel(AT91_DDRC2_MODE_NOP_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/*
	 * Step 7: An All Banks Precharge command is issued
	 * to the low-power DDR1-SDRAM.
	 * Program All Banks Precharge command in the MPDDRC_MR.
	 */
	writel(AT91_DDRC2_MODE_PRCGALL_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/*
	 * Step 8: Two auto-refresh (CBR) cycles are provided.
	 * Program the Auto Refresh command (CBR) in the MPDDRC_MR.
	 * The application must write a four to the MODE field
	 * in the MPDDRC_MR. Perform a write access to any low-power
	 * DDR1-SDRAM location twice to acknowledge these commands.
	 */
	writel(AT91_DDRC2_MODE_RFSH_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	writel(AT91_DDRC2_MODE_RFSH_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/*
	 * Step 9: An Extended Mode Register Set (EMRS) cycle is issued to
	 * program the low-power DDR1-SDRAM parameters (TCSR, PASR, DS).
	 * The application must write a five to the MODE field in the MPDDRC_MR
	 * and perform a write access to the SDRAM to acknowledge this command.
	 * The write address must be chosen so that signal BA[1] is set to 1
	 * and BA[0] is set to 0.
	 */
	writel(AT91_DDRC2_MODE_EXT_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x2 << ba_offset));

	/*
	 * Step 10: A Mode Register Set (MRS) cycle is issued to program
	 * parameters of the low-power DDR1-SDRAM devices, in particular
	 * CAS latency.
	 * The application must write a three to the MODE field in the MPDDRC_MR
	 * and perform a write access to the SDRAM to acknowledge this command.
	 * The write address must be chosen so that signals BA[1:0] are set to 0.
	 */
	writel(AT91_DDRC2_MODE_LMR_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address + (0x0 << ba_offset));

	/*
	 * Step 11: The application must enter Normal mode, write a zero
	 * to the MODE field in the MPDDRC_MR and perform a write access
	 * at any location in the SDRAM to acknowledge this command.
	 */
	writel(AT91_DDRC2_MODE_NORMAL_CMD, base_address + AT91_HDDRSDRC2_MR);
	writel(0, ram_address);

	/*
	 * Step 12: Perform a write access to any low-power DDR1-SDRAM address.
	 */
	writel(0, ram_address);

	/*
	 * Step 14: Write the refresh rate into the COUNT field in the MPDDRC
	 * Refresh Timer Register (MPDDRC_RTR):
	 */
	writel(ddramc_config->rtr, base_address + AT91_HDDRSDRC2_RTR);
}

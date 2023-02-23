/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2014 Marek Vasut <marex@denx.de> */

#ifndef NOVENA_DDR_REGS_H
#define NOVENA_DDR_REGS_H

/* MEMORY CONTROLLER CONFIGURATION */

static struct mx6dq_iomux_ddr_regs novena_ddr_regs = {
	/* SDCLK[0:1], CAS, RAS, Reset: Differential input, 40ohm */
	.dram_sdclk_0		= 0x00020038,
	.dram_sdclk_1		= 0x00020038,
	.dram_cas		= 0x00000038,
	.dram_ras		= 0x00000038,
	.dram_reset		= 0x00000038,
	/* SDCKE[0:1]: 100k pull-up */
	.dram_sdcke0		= 0x00003000,
	.dram_sdcke1		= 0x00003000,
	/* SDBA2: pull-up disabled */
	.dram_sdba2		= 0x00000000,
	/* SDODT[0:1]: 100k pull-up, 40 ohm */
	.dram_sdodt0		= 0x00000038,
	.dram_sdodt1		= 0x00000038,
	/* SDQS[0:7]: Differential input, 40 ohm */
	.dram_sdqs0		= 0x00000038,
	.dram_sdqs1		= 0x00000038,
	.dram_sdqs2		= 0x00000038,
	.dram_sdqs3		= 0x00000038,
	.dram_sdqs4		= 0x00000038,
	.dram_sdqs5		= 0x00000038,
	.dram_sdqs6		= 0x00000038,
	.dram_sdqs7		= 0x00000038,

	/* DQM[0:7]: Differential input, 40 ohm */
	.dram_dqm0		= 0x00000038,
	.dram_dqm1		= 0x00000038,
	.dram_dqm2		= 0x00000038,
	.dram_dqm3		= 0x00000038,
	.dram_dqm4		= 0x00000038,
	.dram_dqm5		= 0x00000038,
	.dram_dqm6		= 0x00000038,
	.dram_dqm7		= 0x00000038,
};

static struct mx6dq_iomux_grp_regs novena_grp_regs = {
	/* DDR3 */
	.grp_ddr_type		= 0x000c0000,
	.grp_ddrmode_ctl	= 0x00020000,
	/* Disable DDR pullups */
	.grp_ddrpke		= 0x00000000,
	/* ADDR[00:16], SDBA[0:1]: 40 ohm */
	.grp_addds		= 0x00000038,
	/* CS0/CS1/SDBA2/CKE0/CKE1/SDWE: 40 ohm */
	.grp_ctlds		= 0x00000038,
	/* DATA[00:63]: Differential input, 40 ohm */
	.grp_ddrmode		= 0x00020000,
	.grp_b0ds		= 0x00000038,
	.grp_b1ds		= 0x00000038,
	.grp_b2ds		= 0x00000038,
	.grp_b3ds		= 0x00000038,
	.grp_b4ds		= 0x00000038,
	.grp_b5ds		= 0x00000038,
	.grp_b6ds		= 0x00000038,
	.grp_b7ds		= 0x00000038,
};

/* MEMORY STICK CONFIGURATION */

static struct mx6_mmdc_calibration novena_mmdc_calib = {
	/* write leveling calibration determine */
	.p0_mpwldectrl0		= 0x00420048,
	.p0_mpwldectrl1		= 0x006f0059,
	.p1_mpwldectrl0		= 0x005a0104,
	.p1_mpwldectrl1		= 0x01070113,
	/* Read DQS Gating calibration */
	.p0_mpdgctrl0		= 0x437c040b,
	.p0_mpdgctrl1		= 0x0413040e,
	.p1_mpdgctrl0		= 0x444f0446,
	.p1_mpdgctrl1		= 0x044d0422,
	/* Read Calibration: DQS delay relative to DQ read access */
	.p0_mprddlctl		= 0x4c424249,
	.p1_mprddlctl		= 0x4e48414f,
	/* Write Calibration: DQ/DM delay relative to DQS write access */
	.p0_mpwrdlctl		= 0x42414641,
	.p1_mpwrdlctl		= 0x46374b43,
};

static struct mx6_ddr_sysinfo novena_ddr_info = {
	/* Width of data bus: 0=16, 1=32, 2=64 */
	.dsize		= 2,
	/* Config for full 4GB range so that get_mem_size() works */
	.cs_density	= 32,	/* 32Gb per CS */
	/* Single chip select */
	.ncs		= 1,
	.cs1_mirror	= 0,
	.rtt_wr		= 1,	/* RTT_Wr = RZQ/4 */
	.rtt_nom	= 2,	/* RTT_Nom = RZQ/2 */
	.walat		= 3,	/* Write additional latency */
	.ralat		= 7,	/* Read additional latency */
	.mif3_mode	= 3,	/* Command prediction working mode */
	.bi_on		= 0,	/* Bank interleaving disabled */
	.sde_to_rst	= 0x10,	/* 14 cycles, 200us (JEDEC default) */
	.rst_to_cke	= 0x23,	/* 33 cycles, 500us (JEDEC default) */
};

static struct mx6_ddr3_cfg novena_ddr_cfg = {
	.mem_speed	= 1600,
	.density	= 4,
	.width		= 64,
	.banks		= 8,
	.rowaddr	= 16,
	.coladdr	= 10,
	.pagesz		= 1,
	.trcd		= 1300,
	.trcmin		= 4900,
	.trasmin	= 3590,
};

#endif

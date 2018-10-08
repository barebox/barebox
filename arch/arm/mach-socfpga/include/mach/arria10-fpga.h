/*
 * FPGA Manager Driver for Altera Arria10 SoCFPGA
 *
 * Copyright (C) 2015-2016 Altera Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  __A10_FPGAMGR_H__
#define	__A10_FPGAMGR_H__

#include <linux/bitops.h>
#include <mach/arria10-regs.h>

#define A10_FPGAMGR_DCLKCNT_OFST				0x08
#define A10_FPGAMGR_DCLKSTAT_OFST				0x0c
#define A10_FPGAMGR_IMGCFG_CTL_00_OFST				0x70
#define A10_FPGAMGR_IMGCFG_CTL_01_OFST				0x74
#define A10_FPGAMGR_IMGCFG_CTL_02_OFST				0x78
#define A10_FPGAMGR_IMGCFG_STAT_OFST				0x80

#define A10_FPGAMGR_DCLKSTAT_DCLKDONE				BIT(0)

#define A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_NCONFIG		BIT(0)
#define A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_NSTATUS		BIT(1)
#define A10_FPGAMGR_IMGCFG_CTL_00_S2F_NENABLE_CONDONE		BIT(2)
#define A10_FPGAMGR_IMGCFG_CTL_00_S2F_NCONFIG			BIT(8)
#define A10_FPGAMGR_IMGCFG_CTL_00_S2F_NSTATUS_OE		BIT(16)
#define A10_FPGAMGR_IMGCFG_CTL_00_S2F_CONDONE_OE		BIT(24)

#define A10_FPGAMGR_IMGCFG_CTL_01_S2F_NENABLE_CONFIG		BIT(0)
#define A10_FPGAMGR_IMGCFG_CTL_01_S2F_PR_REQUEST		BIT(16)
#define A10_FPGAMGR_IMGCFG_CTL_01_S2F_NCE			BIT(24)

#define A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_CTRL			BIT(0)
#define A10_FPGAMGR_IMGCFG_CTL_02_EN_CFG_DATA			BIT(8)
#define A10_FPGAMGR_IMGCFG_CTL_02_CDRATIO_MASK			(BIT(16) | BIT(17))
#define A10_FPGAMGR_IMGCFG_CTL_02_CDRATIO_SHIFT			16
#define A10_FPGAMGR_IMGCFG_CTL_02_CFGWIDTH			BIT(24)
#define A10_FPGAMGR_IMGCFG_CTL_02_CFGWIDTH_SHIFT		24

#define A10_FPGAMGR_IMGCFG_STAT_F2S_CRC_ERROR			BIT(0)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_EARLY_USERMODE		BIT(1)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_USERMODE			BIT(2)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_NSTATUS_PIN			BIT(4)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_PIN			BIT(6)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_CONDONE_OE			BIT(7)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_PR_READY			BIT(9)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_PR_DONE			BIT(10)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_PR_ERROR			BIT(11)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_NCONFIG_PIN			BIT(12)
#define A10_FPGAMGR_IMGCFG_STAT_F2S_MSEL_MASK			(BIT(16) | BIT(17) | BIT(18))
#define A10_FPGAMGR_IMGCFG_STAT_F2S_MSEL_SHIFT		        16

/* FPGA CD Ratio Value */
#define CDRATIO_x1						0x0
#define CDRATIO_x2						0x1
#define CDRATIO_x4						0x2
#define CDRATIO_x8						0x3

/* Configuration width 16/32 bit */
#define CFGWDTH_32						1
#define CFGWDTH_16						0

static inline int a10_wait_for_usermode(int timeout) {
	while ((readl(ARRIA10_FPGAMGRREGS_ADDR +
		      A10_FPGAMGR_IMGCFG_STAT_OFST) &
		(A10_FPGAMGR_IMGCFG_STAT_F2S_EARLY_USERMODE |
		 A10_FPGAMGR_IMGCFG_STAT_F2S_USERMODE)) == 0)
		if (timeout-- <= 0)
			return -ETIMEDOUT;

	return 0;
}

#endif

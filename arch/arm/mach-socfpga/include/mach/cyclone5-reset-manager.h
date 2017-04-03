/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef	_RESET_MANAGER_H_
#define	_RESET_MANAGER_H_

#define RESET_MGR_STATUS_OFS		0x0
#define RESET_MGR_CTRL_OFS		0x4
#define RESET_MGR_COUNTS_OFS		0x8
#define RESET_MGR_MPU_MOD_RESET_OFS	0x10
#define RESET_MGR_PER_MOD_RESET_OFS	0x14
#define RESET_MGR_PER2_MOD_RESET_OFS	0x18
#define RESET_MGR_BRG_MOD_RESET_OFS	0x1c

#define RSTMGR_CTRL_SWWARMRSTREQ_LSB 1
#define RSTMGR_PERMODRST_OSC1TIMER0_LSB 8

#define RSTMGR_PERMODRST_EMAC0_LSB 0
#define RSTMGR_PERMODRST_EMAC1_LSB 1
#define RSTMGR_PERMODRST_L4WD0_LSB 6
#define RSTMGR_PERMODRST_SDR_LSB 29
#define RSTMGR_BRGMODRST_HPS2FPGA_MASK		0x00000001
#define RSTMGR_BRGMODRST_LWHPS2FPGA_MASK	0x00000002
#define RSTMGR_BRGMODRST_FPGA2HPS_MASK		0x00000004

/* Warm Reset mask */
#define RSTMGR_STAT_L4WD1RST_MASK		0x00008000
#define RSTMGR_STAT_L4WD0RST_MASK		0x00004000
#define RSTMGR_STAT_MPUWD1RST_MASK		0x00002000
#define RSTMGR_STAT_MPUWD0RST_MASK		0x00001000
#define RSTMGR_STAT_SWWARMRST_MASK		0x00000400
#define RSTMGR_STAT_FPGAWARMRST_MASK		0x00000200
#define RSTMGR_STAT_NRSTPINRST_MASK		0x00000100
#define RSTMGR_WARMRST_MASK			0x0000f700

#define RSTMGR_CTRL_SDRSELFREFEN_MASK		0x00000010
#define RSTMGR_CTRL_FPGAHSEN_MASK		0x00010000
#define RSTMGR_CTRL_ETRSTALLEN_MASK		0x00100000

#define RSTMGR_PERMODRST_EMAC0		(1 << 0)
#define RSTMGR_PERMODRST_EMAC1		(1 << 1)
#define RSTMGR_PERMODRST_USB0		(1 << 2)
#define RSTMGR_PERMODRST_USB1		(1 << 3)
#define RSTMGR_PERMODRST_NAND		(1 << 4)
#define RSTMGR_PERMODRST_QSPI		(1 << 5)
#define RSTMGR_PERMODRST_L4WD0		(1 << 6)
#define RSTMGR_PERMODRST_L4WD1		(1 << 7)
#define RSTMGR_PERMODRST_OSC1TIMER1	(1 << 9)
#define RSTMGR_PERMODRST_SPTIMER0	(1 << 10)
#define RSTMGR_PERMODRST_SPTIMER1	(1 << 11)
#define RSTMGR_PERMODRST_I2C0		(1 << 12)
#define RSTMGR_PERMODRST_I2C1		(1 << 13)
#define RSTMGR_PERMODRST_I2C2		(1 << 14)
#define RSTMGR_PERMODRST_I2C3		(1 << 15)
#define RSTMGR_PERMODRST_UART0		(1 << 16)
#define RSTMGR_PERMODRST_UART1		(1 << 17)
#define RSTMGR_PERMODRST_SPIM0		(1 << 18)
#define RSTMGR_PERMODRST_SPIM1		(1 << 19)
#define RSTMGR_PERMODRST_SPIS0		(1 << 20)
#define RSTMGR_PERMODRST_SPIS1		(1 << 21)
#define RSTMGR_PERMODRST_SDMMC		(1 << 22)
#define RSTMGR_PERMODRST_CAN0		(1 << 23)
#define RSTMGR_PERMODRST_CAN1		(1 << 24)
#define RSTMGR_PERMODRST_GPIO0		(1 << 25)
#define RSTMGR_PERMODRST_GPIO1		(1 << 26)
#define RSTMGR_PERMODRST_GPIO2		(1 << 27)
#define RSTMGR_PERMODRST_DMA		(1 << 28)
#define RSTMGR_PERMODRST_SDR		(1 << 29)

#define RSTMGR_PER2MODRST_DMAIF0	(1 << 0)
#define RSTMGR_PER2MODRST_DMAIF1	(1 << 1)
#define RSTMGR_PER2MODRST_DMAIF2	(1 << 2)
#define RSTMGR_PER2MODRST_DMAIF3	(1 << 3)
#define RSTMGR_PER2MODRST_DMAIF4	(1 << 4)
#define RSTMGR_PER2MODRST_DMAIF5	(1 << 5)
#define RSTMGR_PER2MODRST_DMAIF6	(1 << 6)
#define RSTMGR_PER2MODRST_DMAIF7	(1 << 7)

#endif /* _RESET_MANAGER_H_ */

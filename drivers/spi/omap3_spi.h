/*
 * Register definitions for the OMAP3 McSPI Controller
 *
 * Copyright (C) 2010 Dirk Behme <dirk.behme@googlemail.com>
 *
 * Parts taken from linux/drivers/spi/omap2_mcspi.c
 * Copyright (C) 2005, 2006 Nokia Corporation
 *
 * Modified by Ruslan Araslanov <ruslan.araslanov@vitecmm.com>
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

#ifndef _OMAP3_SPI_H_
#define _OMAP3_SPI_H_

#define OMAP3_MCSPI_MAX_FREQ	48000000

/* OMAP3 McSPI registers */
#define OMAP3_MCSPI REVISION		0x00
#define OMAP3_MCSPI_SYSCONFIG		0x10
#define OMAP3_MCSPI_SYSSTATUS		0x14
#define OMAP3_MCSPI_IRQSTATUS		0x18
#define OMAP3_MCSPI_IRQENABLE		0x1C
#define OMAP3_MCSPI_WAKEUPENABLE	0x20
#define OMAP3_MCSPI_SYST		0x24
#define OMAP3_MCSPI_MODULCTRL		0x28

#define OMAP3_MCSPI_CH_SIZE		0x14 /* bytes each */

#define OMAP3_MCSPI_CHCONF0		0x2C /* also 0x40, 0x54, 0x68 */
#define OMAP3_MCSPI_CHSTAT0		0x30 /* also 0x44, 0x58, 0x6C */
#define OMAP3_MCSPI_CHCTRL0		0x34 /* also 0x48, 0x5C, 0x70 */
#define OMAP3_MCSPI_TX0			0x38 /* also 0x4C, 0x60, 0x74 */
#define OMAP3_MCSPI_RX0			0x3C /* also 0x50, 0x64, 0x78 */

/* per-register bitmasks */
#define OMAP3_MCSPI_SYSCONFIG_SMARTIDLE (2 << 3)
#define OMAP3_MCSPI_SYSCONFIG_ENAWAKEUP (1 << 2)
#define OMAP3_MCSPI_SYSCONFIG_AUTOIDLE	(1 << 0)
#define OMAP3_MCSPI_SYSCONFIG_SOFTRESET (1 << 1)

#define OMAP3_MCSPI_SYSSTATUS_RESETDONE (1 << 0)

#define OMAP3_MCSPI_MODULCTRL_SINGLE	(1 << 0)
#define OMAP3_MCSPI_MODULCTRL_MS	(1 << 2)
#define OMAP3_MCSPI_MODULCTRL_STEST	(1 << 3)

#define OMAP3_MCSPI_CHCONF_PHA		(1 << 0)
#define OMAP3_MCSPI_CHCONF_POL		(1 << 1)
#define OMAP3_MCSPI_CHCONF_CLKD_MASK	(0x0f << 2)
#define OMAP3_MCSPI_CHCONF_EPOL		(1 << 6)
#define OMAP3_MCSPI_CHCONF_WL_MASK	(0x1f << 7)
#define OMAP3_MCSPI_CHCONF_TRM_RX_ONLY	(0x01 << 12)
#define OMAP3_MCSPI_CHCONF_TRM_TX_ONLY	(0x02 << 12)
#define OMAP3_MCSPI_CHCONF_TRM_MASK	(0x03 << 12)
#define OMAP3_MCSPI_CHCONF_DMAW		(1 << 14)
#define OMAP3_MCSPI_CHCONF_DMAR		(1 << 15)
#define OMAP3_MCSPI_CHCONF_DPE0		(1 << 16)
#define OMAP3_MCSPI_CHCONF_DPE1		(1 << 17)
#define OMAP3_MCSPI_CHCONF_IS		(1 << 18)
#define OMAP3_MCSPI_CHCONF_TURBO	(1 << 19)
#define OMAP3_MCSPI_CHCONF_FORCE	(1 << 20)

#define OMAP3_MCSPI_CHSTAT_RXS		(1 << 0)
#define OMAP3_MCSPI_CHSTAT_TXS		(1 << 1)
#define OMAP3_MCSPI_CHSTAT_EOT		(1 << 2)

#define OMAP3_MCSPI_CHCTRL_EN		(1 << 0)

#define OMAP3_MCSPI_WAKEUPENABLE_WKEN	(1 << 0)

struct omap3_spi_master {
	struct spi_master master;
	void __iomem *regs;
};

#endif /* _OMAP3_SPI_H_ */

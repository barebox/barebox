/*
 * Copyright 2013 GE Intelligent Platforms, Inc
 * (C) Copyright 2008 Wolfgang Grandegger <wg@denx.de>
 * (C) Copyright 2006
 * Thomas Waehner, TQ-System GmbH, thomas.waehner@tqs.de.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * This code only cares about setting up the UPM state machine for Linux
 * to use the NAND.
 */

#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <asm/fsl_lbc.h>
#include <mach/immap_85xx.h>

/* NAND UPM tables for a 25Mhz bus frequency.  */
static const u32 upm_patt_25[] = {
	/* Single read data */
	0xcff02c30, 0x0ff02c30, 0x0ff02c34, 0x0ff32c30,
	0xfff32c31, 0xfff32c30, 0xfffffc30, 0xfffffc30,
	/* UPM Read Burst RAM array entry -> NAND Write CMD */
	0xcfaf2c30, 0x0faf2c30, 0x0faf2c30, 0x0fff2c34,
	0xfffffc31, 0xfffffc30, 0xfffffc30, 0xfffffc30,
	/* UPM Read Burst RAM array entry -> NAND Write ADDR */
	0xcfa3ec30, 0x0fa3ec30, 0x0fa3ec30, 0x0ff3ec34,
	0xfff3ec31, 0xfffffc30, 0xfffffc30, 0xfffffc30,
	/* UPM Write Single RAM array entry -> NAND Write Data */
	0x0ff32c30, 0x0fa32c30, 0x0fa32c34, 0x0ff32c30,
	0xfff32c31, 0xfff0fc30, 0xfff0fc30, 0xfff0fc30,
	/* Default */
	0xfff3fc30, 0xfff3fc30, 0xfff6fc30, 0xfffcfc30,
	0xfffcfc30, 0xfffcfc30, 0xfffcfc30, 0xfffcfc30,
	0xfffcfc30, 0xfffcfc30, 0xfffcfc30, 0xfffcfc30,
	0xfffdfc30, 0xfffffc30, 0xfffffc30, 0xfffffc31,
	0xfffffc30, 0xfffffc00, 0xfffffc00, 0xfffffc00,
	0xfffffc00, 0xfffffc00, 0xfffffc00, 0xfffffc00,
	0xfffffc00, 0xfffffc00, 0xfffffc00, 0xfffffc01,
	0xfffffc00, 0xfffffc00, 0xfffffc00, 0xfffffc01,
};

static void upm_write(uint8_t addr, uint32_t val)
{
	void __iomem *lbc = LBC_BASE_ADDR;

	out_be32(lbc + FSL_LBC_MDR_OFFSET, val);
	clrsetbits_be32(lbc + FSL_LBC_MAMR_OFFSET, MxMR_MAD_MSK,
			MxMR_OP_WARR | (addr & MxMR_MAD_MSK));

	/* dummy access to perform write */
	out_8(IOMEM(0xfc000000), 0);
	clrbits_be32(lbc + FSL_LBC_MAMR_OFFSET, MxMR_OP_WARR);
}

static int board_nand_init(void)
{
	void __iomem *mxmr = IOMEM(LBC_BASE_ADDR + FSL_LBC_MAMR_OFFSET);
	int j;

	/* Base register CS2:
	 * - 0xfc000000
	 * - 8-bit data width
	 * - UPMA
	 */
	fsl_set_lbc_br(2, BR_PHYS_ADDR(0xfc000000) | BR_PS_8 | BR_MS_UPMA |
		       BR_V);

	/*
	 * Otions register:
	 * - 32KB window.
	 * - Buffer control disabled.
	 * - External address latch delay.
	 */
	fsl_set_lbc_or(2, 0xffffe001);

	for (j = 0; j < 64; j++)
		upm_write(j, upm_patt_25[j]);

	out_be32(mxmr, MxMR_OP_NORM | MxMR_GPL_x4DIS);

	return 0;
}

device_initcall(board_nand_init);

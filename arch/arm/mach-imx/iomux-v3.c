/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2008 by Sascha Hauer <kernel@pengutronix.de>
 * Copyright (C) 2009 by Jan Weitzel Phytec Messtechnik GmbH,
 *                       <armlinux@phytec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <common.h>
#include <asm/io.h>
#include <mach/iomux-v3.h>
#include <mach/imx-regs.h>

/*
 * setups a single pin:
 * 	- reserves the pin so that it is not claimed by another driver
 * 	- setups the iomux according to the configuration
 */
int mxc_iomux_v3_setup_pad(struct pad_desc *pad)
{
	if (pad->mux_ctrl_ofs)
		writel(pad->mux_mode, IMX_IOMUXC_BASE + pad->mux_ctrl_ofs);

	if (pad->select_input_ofs)
		writel(pad->select_input,
				IMX_IOMUXC_BASE + pad->select_input_ofs);

	if (!(pad->pad_ctrl & NO_PAD_CTRL))
		writel(pad->pad_ctrl, IMX_IOMUXC_BASE + pad->pad_ctrl_ofs);
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_v3_setup_pad);

int mxc_iomux_v3_setup_multiple_pads(struct pad_desc *pad_list, unsigned count)
{
	struct pad_desc *p = pad_list;
	int i;

	for (i = 0; i < count; i++) {
		mxc_iomux_v3_setup_pad(p);
		p++;
	}
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_v3_setup_multiple_pads);


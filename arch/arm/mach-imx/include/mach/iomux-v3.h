/*
 * Copyright (C) 2009 by Jan Weitzel Phytec Messtechnik GmbH,
 *			<armlinux@phytec.de>
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
 */

#ifndef __MACH_IOMUX_V3_H__
#define __MACH_IOMUX_V3_H__

#include <io.h>

/*
 *	build IOMUX_PAD structure
 *
 * This iomux scheme is based around pads, which are the physical balls
 * on the processor.
 *
 * - Each pad has a pad control register (IOMUXC_SW_PAD_CTRL_x) which controls
 *   things like driving strength and pullup/pulldown.
 * - Each pad can have but not necessarily does have an output routing register
 *   (IOMUXC_SW_MUX_CTL_PAD_x).
 * - Each pad can have but not necessarily does have an input routing register
 *   (IOMUXC_x_SELECT_INPUT)
 *
 * The three register sets do not have a fixed offset to each other,
 * hence we order this table by pad control registers (which all pads
 * have) and put the optional i/o routing registers into additional
 * fields.
 *
 * The naming convention for the pad modes is MX35_PAD_<padname>__<padmode>
 * If <padname> or <padmode> refers to a GPIO, it is named
 * GPIO_<unit>_<num>
 *
 * IOMUX/PAD Bit field definitions
 *
 * MUX_CTRL_OFS:	    0..11 (12)
 * PAD_CTRL_OFS:	   12..23 (12)
 * SEL_INPUT_OFS:	   24..35 (12)
 * MUX_MODE + SION:	   36..40  (5)
 * PAD_CTRL + NO_PAD_CTRL: 41..58 (18)
 * SEL_INP:		   59..62  (4)
 * reserved:		     63    (1)
*/

typedef u64 iomux_v3_cfg_t;

#define MUX_CTRL_OFS_SHIFT	0
#define MUX_CTRL_OFS_MASK	((iomux_v3_cfg_t)0xfff << MUX_CTRL_OFS_SHIFT)
#define MUX_PAD_CTRL_OFS_SHIFT	12
#define MUX_PAD_CTRL_OFS_MASK	((iomux_v3_cfg_t)0xfff << MUX_PAD_CTRL_OFS_SHIFT)
#define MUX_SEL_INPUT_OFS_SHIFT	24
#define MUX_SEL_INPUT_OFS_MASK	((iomux_v3_cfg_t)0xfff << MUX_SEL_INPUT_OFS_SHIFT)

#define MUX_MODE_SHIFT		36
#define MUX_MODE_MASK		((iomux_v3_cfg_t)0x1f << MUX_MODE_SHIFT)
#define MUX_PAD_CTRL_SHIFT	41
#define MUX_PAD_CTRL_MASK	((iomux_v3_cfg_t)0x3ffff << MUX_PAD_CTRL_SHIFT)
#define MUX_SEL_INPUT_SHIFT	59
#define MUX_SEL_INPUT_MASK	((iomux_v3_cfg_t)0xf << MUX_SEL_INPUT_SHIFT)

#define MUX_PAD_CTRL(x)		((iomux_v3_cfg_t)(x) << MUX_PAD_CTRL_SHIFT)

#define IOMUX_PAD(_pad_ctrl_ofs, _mux_ctrl_ofs, _mux_mode, _sel_input_ofs, \
		_sel_input, _pad_ctrl)					\
	(((iomux_v3_cfg_t)(_mux_ctrl_ofs) << MUX_CTRL_OFS_SHIFT) |	\
		((iomux_v3_cfg_t)(_mux_mode) << MUX_MODE_SHIFT) |	\
		((iomux_v3_cfg_t)(_pad_ctrl_ofs) << MUX_PAD_CTRL_OFS_SHIFT) | \
		((iomux_v3_cfg_t)(_pad_ctrl) << MUX_PAD_CTRL_SHIFT) |	\
		((iomux_v3_cfg_t)(_sel_input_ofs) << MUX_SEL_INPUT_OFS_SHIFT) | \
		((iomux_v3_cfg_t)(_sel_input) << MUX_SEL_INPUT_SHIFT))

#define IOMUX_PAD_FIELD(name, pad) 	(((pad) & name##_MASK) >> name##_SHIFT)
#define IOMUX_CTRL_OFS(pad)		IOMUX_PAD_FIELD(MUX_CTRL_OFS, pad)
#define IOMUX_MODE(pad)			IOMUX_PAD_FIELD(MUX_MODE, pad)
#define IOMUX_SEL_INPUT_OFS(pad)	IOMUX_PAD_FIELD(MUX_SEL_INPUT_OFS, pad)
#define IOMUX_SEL_INPUT(pad)		IOMUX_PAD_FIELD(MUX_SEL_INPUT, pad)
#define IOMUX_PAD_CTRL_OFS(pad)		IOMUX_PAD_FIELD(MUX_PAD_CTRL_OFS, pad)
#define IOMUX_PAD_CTRL(pad)		IOMUX_PAD_FIELD(MUX_PAD_CTRL, pad)

#define NEW_PAD_CTRL(cfg, pad)	(((cfg) & ~MUX_PAD_CTRL_MASK) | MUX_PAD_CTRL(pad))
/*
 * Use to set PAD control
 */

#define NO_PAD_CTRL			(1 << 17)
#define PAD_CTL_DVS			(1 << 13)
#define PAD_CTL_HYS			(1 << 8)

#define PAD_CTL_PKE			(1 << 7)
#define PAD_CTL_PUE			(1 << 6 | PAD_CTL_PKE)
#define PAD_CTL_PUS_100K_DOWN		(0 << 4 | PAD_CTL_PUE)
#define PAD_CTL_PUS_47K_UP		(1 << 4 | PAD_CTL_PUE)
#define PAD_CTL_PUS_100K_UP		(2 << 4 | PAD_CTL_PUE)
#define PAD_CTL_PUS_22K_UP		(3 << 4 | PAD_CTL_PUE)

#define PAD_CTL_ODE			(1 << 3)

#define PAD_CTL_DSE_LOW			(0 << 1)
#define PAD_CTL_DSE_MED			(1 << 1)
#define PAD_CTL_DSE_HIGH		(2 << 1)
#define PAD_CTL_DSE_MAX			(3 << 1)

#define PAD_CTL_SRE_FAST		(1 << 0)
#define PAD_CTL_SRE_SLOW		(0 << 0)

#define IOMUX_CONFIG_SION		(0x1 << 4)

#define SHARE_MUX_CONF_REG		0x1
#define ZERO_OFFSET_VALID		0x2
#define IMX7_PINMUX_LPSR		0x4

static inline void iomux_v3_setup_pad(void __iomem *iomux, unsigned int flags,
				      u32 mux_reg, u32 conf_reg, u32 input_reg,
				      u32 mux_val, u32 conf_val, u32 input_val)
{
	const bool mux_ok   = !!mux_reg || (flags & ZERO_OFFSET_VALID);
	const bool conf_ok  = !!conf_reg;
	const bool input_ok = !!input_reg;

	/*
	 * The sel_input registers for the LPSR controller pins are in the regular pinmux
	 * controller, so bend the register offset over to the other controller.
	 */
	if (flags & IMX7_PINMUX_LPSR)
		input_reg += 0x70000;

	if (flags & SHARE_MUX_CONF_REG) {
		mux_val |= conf_val;
	} else {
		if (conf_ok)
			writel(conf_val, iomux + conf_reg);
	}

	if (mux_ok)
		writel(mux_val, iomux + mux_reg);

	if (input_ok)
		writel(input_val, iomux + input_reg);
}

static inline void imx_setup_pad(void __iomem *iomux, iomux_v3_cfg_t pad)
{
	uint32_t pad_ctrl;

	pad_ctrl = IOMUX_PAD_CTRL(pad);
	pad_ctrl = (pad_ctrl & NO_PAD_CTRL) ? 0 : pad_ctrl,

	iomux_v3_setup_pad(iomux, 0,
			   IOMUX_CTRL_OFS(pad),
			   IOMUX_PAD_CTRL_OFS(pad),
			   IOMUX_SEL_INPUT_OFS(pad),
			   IOMUX_MODE(pad),
			   pad_ctrl,
			   IOMUX_SEL_INPUT(pad));
}



/*
 * setups a single pad in the iomuxer
 */
int mxc_iomux_v3_setup_pad(iomux_v3_cfg_t pad);

/*
 * setups mutliple pads
 * convenient way to call the above function with tables
 */
int mxc_iomux_v3_setup_multiple_pads(const iomux_v3_cfg_t *pad_list, unsigned count);

#endif /* __MACH_IOMUX_V3_H__*/

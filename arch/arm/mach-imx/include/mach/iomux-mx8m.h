/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IOMUX_IMX8M_H__
#define __MACH_IOMUX_IMX8M_H__

#include <mach/iomux-v3.h>

#define PAD_CTL_DSE_3P3V_45_OHM	0b110

static inline void imx8m_setup_pad(void __iomem *iomux, iomux_v3_cfg_t pad)
{
	unsigned int flags = 0;
	uint32_t mode = IOMUX_MODE(pad);

	if (mode & IOMUX_CONFIG_LPSR) {
		mode &= ~IOMUX_CONFIG_LPSR;
		flags = ZERO_OFFSET_VALID | IMX7_PINMUX_LPSR;
	}

	iomux_v3_setup_pad(iomux, flags,
			   IOMUX_CTRL_OFS(pad),
			   IOMUX_PAD_CTRL_OFS(pad),
			   IOMUX_SEL_INPUT_OFS(pad),
			   mode,
			   IOMUX_PAD_CTRL(pad),
			   IOMUX_SEL_INPUT(pad));
}

#endif /* __MACH_IOMUX_IMX8MQ_H__ */

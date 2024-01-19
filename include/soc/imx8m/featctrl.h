/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix */

#ifndef __IMX8M_FEATCTRL_H_
#define __IMX8M_FEATCTRL_H_

#include <linux/types.h>

struct imx8m_featctrl_data {
	struct {
		u32 vpu_bitmask;
		u32 cpu_bitmask;
	} tester3;
	struct {
		u32 vpu_bitmask;
		u32 gpu_bitmask;
		u32 mipi_dsi_bitmask;
		u32 isp_bitmask;
		u32 cpu_bitmask;
		u32 npu_bitmask;
		u32 lvds_bitmask;
		u32 dsp_bitmask;
	} tester4;
};

struct device;

#ifdef CONFIG_IMX8M_FEATCTRL
int imx8m_feat_ctrl_init(struct device *dev, u32 tester3, u32 tester4,
			 const struct imx8m_featctrl_data *data);
#else
static inline int imx8m_feat_ctrl_init(struct device *dev, u32 tester3, u32 tester4,
				       const struct imx8m_featctrl_data *data)
{
	return -ENODEV;
}
#endif

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix */

#ifndef __IMX8M_FEATCTRL_H_
#define __IMX8M_FEATCTRL_H_

#include <linux/types.h>

struct imx8m_featctrl_data {
	u32 vpu_bitmask;
	u32 gpu_bitmask;
	u32 mipi_dsi_bitmask;
	u32 isp_bitmask;
	bool check_cpus;
};

struct device;

#ifdef CONFIG_IMX8M_FEATCTRL
int imx8m_feat_ctrl_init(struct device *dev, u32 tester4,
			 const struct imx8m_featctrl_data *data);
#else
static inline int imx8m_feat_ctrl_init(struct device *dev, u32 tester4,
				       const struct imx8m_featctrl_data *data)
{
	return -ENODEV;
}
#endif

#endif

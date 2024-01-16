/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX_SCRATCH_H
#define __MACH_IMX_SCRATCH_H

struct imx_scratch_space {
	u32 bootrom_log[128];
};

struct imx_scratch_space *__imx8m_scratch_space(int ddr_buswidth);

#define imx8mq_scratch_space() __imx8m_scratch_space(32)
#define imx8mm_scratch_space() __imx8m_scratch_space(32)
#define imx8mn_scratch_space() __imx8m_scratch_space(16)
#define imx8mp_scratch_space() __imx8m_scratch_space(32)

#endif /* __MACH_IMX_SCRATCH_H */

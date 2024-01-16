/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX_SCRATCH_H
#define __MACH_IMX_SCRATCH_H

void *__imx8m_scratch_space(int ddr_buswidth);

const u32 *imx8m_scratch_get_bootrom_log(void);

#define imx8mq_scratch_space() __imx8m_scratch_space(32)
#define imx8mm_scratch_space() __imx8m_scratch_space(32)
#define imx8mn_scratch_space() __imx8m_scratch_space(16)
#define imx8mp_scratch_space() __imx8m_scratch_space(32)

#endif /* __MACH_IMX_SCRATCH_H */

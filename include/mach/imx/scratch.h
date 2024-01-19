/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX_SCRATCH_H
#define __MACH_IMX_SCRATCH_H

void imx8m_init_scratch_space(int ddr_buswidth, bool zero_init);
void imx93_init_scratch_space(bool zero_init);

const u32 *imx8m_scratch_get_bootrom_log(void);
void imx8m_scratch_save_bootrom_log(const u32 *rom_log);

struct optee_header;

const struct optee_header *imx_scratch_get_optee_hdr(void);
void imx_scratch_save_optee_hdr(const struct optee_header *hdr);

#define imx8mq_init_scratch_space() imx8m_init_scratch_space(32, true)
#define imx8mm_init_scratch_space() imx8m_init_scratch_space(32, true)
#define imx8mn_init_scratch_space() imx8m_init_scratch_space(16, true)
#define imx8mp_init_scratch_space() imx8m_init_scratch_space(32, true)

#endif /* __MACH_IMX_SCRATCH_H */

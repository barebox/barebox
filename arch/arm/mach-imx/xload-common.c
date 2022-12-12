// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#include <mach/xload.h>
#include <mach/esdctl.h>
#include <mach/imx8m-regs.h>
#include <asm/barebox-arm.h>

int imx_image_size(void)
{
	/* i.MX header is 4k */
	return barebox_image_size + SZ_4K;
}

int piggydata_size(void)
{
	return input_data_end - input_data;
}

struct imx_scratch_space *__imx8m_scratch_space(int ddr_buswidth)
{
	ulong endmem = MX8M_DDR_CSD1_BASE_ADDR +
		imx8m_barebox_earlymem_size(ddr_buswidth);

	return (void *)__arm_mem_scratch(endmem);
}

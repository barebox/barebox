// SPDX-License-Identifier: GPL-2.0-only

#include <asm/barebox-arm.h>
#include <init.h>
#include <linux/err.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/scratch.h>
#include <memory.h>

struct imx_scratch_space *__imx8m_scratch_space(int ddr_buswidth)
{
	ulong endmem = MX8M_DDR_CSD1_BASE_ADDR +
		imx8m_barebox_earlymem_size(ddr_buswidth);

	return (void *)arm_mem_scratch(endmem);
}

static int imx8m_reserve_scratch_area(void)
{
	return PTR_ERR_OR_ZERO(request_sdram_region("scratch area",
				    (ulong)arm_mem_scratch_get(),
				    sizeof(struct imx_scratch_space)));
}
device_initcall(imx8m_reserve_scratch_area);

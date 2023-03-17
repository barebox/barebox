// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <mach/imx/imx1-regs.h>
#include <mach/imx/iomux-v1.h>
#include <mach/imx/iomux-mx1.h>
#include <asm/barebox-arm.h>
#include <mach/imx/esdctl.h>

extern char __dtb_imx1_scb9328_start[];

/* Called from assembly */
void scb9328_start(void);

void scb9328_start(void)
{
	void *fdt;

	fdt = __dtb_imx1_scb9328_start + get_runtime_offset();

	imx1_gpio_mode(PA23_PF_CS5);

	imx1_barebox_entry(fdt);
}

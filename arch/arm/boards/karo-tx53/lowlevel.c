#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx5.h>
#include <mach/imx53-regs.h>
#include <mach/esdctl.h>
#include <mach/generic.h>

void __naked barebox_arm_reset_vector(void)
{
	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	/*
	 * For the TX53 rev 8030 the SDRAM setup is not stable without
	 * the proper PLL setup. It will crash once we enable the MMU,
	 * so do the PLL setup here.
	 */
	if (IS_ENABLED(CONFIG_TX53_REV_XX30))
		imx53_init_lowlevel_early(800);

	imx53_barebox_entry(NULL);
}

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx5.h>
#include <mach/esdctl.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();

	/*
	 * For the TX53 rev 8030 the SDRAM setup is not stable without
	 * the proper PLL setup. It will crash once we enable the MMU,
	 * so do the PLL setup here.
	 */
	if (IS_ENABLED(CONFIG_TX53_REV_XX30))
		imx53_init_lowlevel_early(800);

	imx53_barebox_entry(0);
}

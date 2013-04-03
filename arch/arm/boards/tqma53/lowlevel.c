#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx5.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	imx53_init_lowlevel_early(800);
	imx53_barebox_entry(0);
}

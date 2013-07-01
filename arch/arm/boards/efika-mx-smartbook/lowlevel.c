#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx5.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	arm_setup_stack(0x20000000 - 16);
	imx51_init_lowlevel(800);
	imx51_barebox_entry(0);
}

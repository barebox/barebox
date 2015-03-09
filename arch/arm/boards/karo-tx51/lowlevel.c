#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>

void __naked barebox_arm_reset_vector(void)
{
	imx5_cpu_lowlevel_init();
	arm_setup_stack(0x20000000 - 16);
	imx51_barebox_entry(NULL);
}

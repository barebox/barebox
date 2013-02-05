#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	imx51_barebox_entry(0);
}

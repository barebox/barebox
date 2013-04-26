#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx51-regs.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	barebox_arm_entry(MX51_CSD0_BASE_ADDR, SZ_128M, 0);
}

#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx23-regs.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	barebox_arm_entry(IMX_MEMORY_BASE, SZ_32M, 0);
}

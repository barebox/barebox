#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx23-regs.h>

void __naked barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	arm_cpu_lowlevel_init();
	barebox_arm_entry(IMX_MEMORY_BASE, SZ_32M, NULL);
}

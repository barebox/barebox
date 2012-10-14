#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx28-regs.h>

void __naked reset(void)
{
	common_reset();
	barebox_arm_entry(IMX_MEMORY_BASE, SZ_128M, 0);
}

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx28-regs.h>
#include <generated/mach-types.h>

ENTRY_FUNCTION(start_cfa10036, r0, r1, r2)
{
	arm_cpu_lowlevel_init();
	barebox_arm_entry(IMX_MEMORY_BASE, SZ_128M, (void *)MACH_TYPE_CFA10036);
}

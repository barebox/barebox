#include <common.h>
#include <mach/imx53-regs.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>

void __naked barebox_arm_reset_vector(void)
{
	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);
	imx53_barebox_entry(NULL);
}

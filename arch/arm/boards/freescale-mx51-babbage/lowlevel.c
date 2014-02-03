#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_imx51_babbage_start[];

ENTRY_FUNCTION(start_imx51_babbage, r0, r1, r2)
{
	uint32_t fdt;

	arm_cpu_lowlevel_init();

	fdt = (uint32_t)__dtb_imx51_babbage_start - get_runtime_offset();

	imx51_barebox_entry(fdt);
}

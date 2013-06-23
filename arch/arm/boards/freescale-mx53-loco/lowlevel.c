#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_imx53_qsb_start[];

ENTRY_FUNCTION(start_imx53_loco)(void)
{
	uint32_t fdt;

	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	fdt = (uint32_t)__dtb_imx53_qsb_start - get_runtime_offset();

	imx53_barebox_entry(fdt);
}

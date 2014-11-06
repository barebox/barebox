#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <image-metadata.h>

extern char __dtb_imx53_qsb_start[];

ENTRY_FUNCTION(start_imx53_loco, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();

	fdt = __dtb_imx53_qsb_start - get_runtime_offset();

	imx53_barebox_entry(fdt);
}

extern char __dtb_imx53_qsrb_start[];

ENTRY_FUNCTION(start_imx53_loco_r, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();

	fdt = __dtb_imx53_qsrb_start - get_runtime_offset();

	imx53_barebox_entry(fdt);
}

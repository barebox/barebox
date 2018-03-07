#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_imx53_voipac_bsb_start[];

ENTRY_FUNCTION(start_imx53_vmx53, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(0xf8020000 - 8);

	fdt = __dtb_imx53_voipac_bsb_start + get_runtime_offset();

	imx53_barebox_entry(fdt);
}

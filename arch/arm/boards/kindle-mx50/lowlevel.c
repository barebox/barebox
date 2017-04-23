#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx50-regs.h>
#include <mach/generic.h>

extern char __dtb_imx50_kindle_d01100_start[];
extern char __dtb_imx50_kindle_d01200_start[];
extern char __dtb_imx50_kindle_ey21_start[];

ENTRY_FUNCTION(start_imx50_kindle_d01100, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX50_IRAM_BASE_ADDR + MX50_IRAM_SIZE - 8);

	fdt = __dtb_imx50_kindle_d01100_start - get_runtime_offset();

	barebox_arm_entry(MX50_CSD0_BASE_ADDR, SZ_256M, fdt);
}

ENTRY_FUNCTION(start_imx50_kindle_d01200, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX50_IRAM_BASE_ADDR + MX50_IRAM_SIZE - 8);

	fdt = __dtb_imx50_kindle_d01200_start - get_runtime_offset();

	barebox_arm_entry(MX50_CSD0_BASE_ADDR, SZ_256M, fdt);
}

ENTRY_FUNCTION(start_imx50_kindle_ey21, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX50_IRAM_BASE_ADDR + MX50_IRAM_SIZE - 8);

	fdt = __dtb_imx50_kindle_ey21_start - get_runtime_offset();

	barebox_arm_entry(MX50_CSD0_BASE_ADDR, SZ_256M, fdt);
}

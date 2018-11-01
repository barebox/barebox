#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/platform.h>

extern char __dtb_bcm2835_rpi_start[];
ENTRY_FUNCTION(start_raspberry_pi1, r0, r1, r2)
{
	void *fdt = __dtb_bcm2835_rpi_start + get_runtime_offset();

	arm_cpu_lowlevel_init();

	barebox_arm_entry(BCM2835_SDRAM_BASE, SZ_128M, fdt);
}

extern char __dtb_bcm2836_rpi_2_start[];
ENTRY_FUNCTION(start_raspberry_pi2, r0, r1, r2)
{
	void *fdt = __dtb_bcm2836_rpi_2_start + get_runtime_offset();

	arm_cpu_lowlevel_init();

	barebox_arm_entry(BCM2835_SDRAM_BASE, SZ_512M, fdt);
}

extern char __dtb_bcm2837_rpi_3_start[];
ENTRY_FUNCTION(start_raspberry_pi3, r0, r1, r2)
{
	void *fdt = __dtb_bcm2837_rpi_3_start + get_runtime_offset();

	arm_cpu_lowlevel_init();

	barebox_arm_entry(BCM2835_SDRAM_BASE, SZ_512M, fdt);
}

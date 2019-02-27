#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/platform.h>
#include <of.h>

#include "lowlevel.h"

static void copy_vc_fdt(void *dest, void *src, unsigned long max_size)
{
	struct fdt_header *oftree_src = src;
	struct fdt_header *oftree_dest = dest;

	unsigned long size = be32_to_cpu(oftree_src->totalsize);
	if (size > max_size) {
		oftree_dest->magic = cpu_to_be32(VIDEOCORE_FDT_ERROR);
		/* Save an error code after the magic value for easier
		 * debugging. We can't print out anything this early */
		oftree_dest->totalsize = cpu_to_be32(ENOMEM);
		return;
	}

	memmove(dest, src, size);
}

/* Must be inline since stack isn't setup yet. */
static inline void start_raspberry_pi(unsigned long memsize, void *fdt,
								void *vc_fdt)
{
	void *saved_vc_fdt;
	unsigned long membase = BCM2835_SDRAM_BASE;

	/* A pointer to the FDT created by VideoCore was passed to us in r2. We
	 * reserve some memory just above the region used for Basebox and copy
	 * this FDT there. We fetch it from there later in rpi_devices_init().*/
	memsize -= VIDEOCORE_FDT_SZ;

	arm_cpu_lowlevel_init();

	/* Copied from barebox_arm_entry(). We need stack here early
	 * for normal function calls to work. */
	arm_setup_stack(arm_mem_stack_top(membase, membase + memsize) - 16);

	fdt += get_runtime_offset();

	saved_vc_fdt = (void *)(membase + memsize);
	copy_vc_fdt(saved_vc_fdt, vc_fdt, VIDEOCORE_FDT_SZ);

	barebox_arm_entry(membase, memsize, fdt);
}

extern char __dtb_bcm2835_rpi_start[];
ENTRY_FUNCTION(start_raspberry_pi1, r0, r1, r2)
{
	start_raspberry_pi(SZ_128M, __dtb_bcm2835_rpi_start, (void *)r2);
}

extern char __dtb_bcm2836_rpi_2_start[];
ENTRY_FUNCTION(start_raspberry_pi2, r0, r1, r2)
{
	start_raspberry_pi(SZ_512M, __dtb_bcm2836_rpi_2_start, (void *)r2);
}

extern char __dtb_bcm2837_rpi_3_start[];
ENTRY_FUNCTION(start_raspberry_pi3, r0, r1, r2)
{
	start_raspberry_pi(SZ_512M, __dtb_bcm2837_rpi_3_start, (void *)r2);
}

extern char __dtb_bcm2837_rpi_cm3_start[];
ENTRY_FUNCTION(start_raspberry_pi_cm3, r0, r1, r2)
{
	start_raspberry_pi(SZ_512M, __dtb_bcm2837_rpi_cm3_start, (void *)r2);
}

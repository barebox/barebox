// SPDX-License-Identifier: GPL-2.0-only

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
	unsigned long size;

	if (!src) {
		oftree_dest->magic = cpu_to_be32(VIDEOCORE_FDT_ERROR);
		oftree_dest->totalsize = cpu_to_be32(0);
		return;
	}

	size = be32_to_cpu(oftree_src->totalsize);
	if (size > max_size) {
		oftree_dest->magic = cpu_to_be32(VIDEOCORE_FDT_ERROR);
		/* Save an error code after the magic value for easier
		 * debugging. We can't print out anything this early */
		oftree_dest->totalsize = cpu_to_be32(ENOMEM);
		return;
	}

	memmove(dest, src, size);
}

/* A pointer to the FDT created by VideoCore was passed to us in r2. We
 * reserve some memory just above the region used for Barebox and copy
 * this FDT there. We fetch it from there later in rpi_devices_init().
 */
#define rpi_stack_top(memsize) \
	arm_mem_stack_top(BCM2835_SDRAM_BASE, BCM2835_SDRAM_BASE + memsize - VIDEOCORE_FDT_SZ)

/* Must be inline since stack isn't setup yet. */
static inline void start_raspberry_pi(unsigned long memsize, void *fdt,
								void *vc_fdt)
{
	unsigned long endmem = rpi_stack_top(memsize);

	arm_cpu_lowlevel_init();

	copy_vc_fdt((void *)endmem, vc_fdt, VIDEOCORE_FDT_SZ);

	fdt += get_runtime_offset();

	barebox_arm_entry(BCM2835_SDRAM_BASE, endmem - BCM2835_SDRAM_BASE, fdt);
}

#define RPI_ENTRY_FUNCTION(name, memsize, r2) \
	ENTRY_FUNCTION_WITHSTACK(name, rpi_stack_top(memsize), __r0, __r1, r2)

extern char __dtb_bcm2835_rpi_start[];
RPI_ENTRY_FUNCTION(start_raspberry_pi1, SZ_128M, r2)
{
	start_raspberry_pi(SZ_128M, __dtb_bcm2835_rpi_start, (void *)r2);
}

extern char __dtb_bcm2836_rpi_2_start[];
RPI_ENTRY_FUNCTION(start_raspberry_pi2, SZ_512M, r2)
{
	start_raspberry_pi(SZ_512M, __dtb_bcm2836_rpi_2_start, (void *)r2);
}

extern char __dtb_bcm2837_rpi_3_start[];
RPI_ENTRY_FUNCTION(start_raspberry_pi3, SZ_512M, r2)
{
	start_raspberry_pi(SZ_512M, __dtb_bcm2837_rpi_3_start, (void *)r2);
}

extern char __dtb_bcm2837_rpi_cm3_start[];
RPI_ENTRY_FUNCTION(start_raspberry_pi_cm3, SZ_512M, r2)
{
	start_raspberry_pi(SZ_512M, __dtb_bcm2837_rpi_cm3_start, (void *)r2);
}

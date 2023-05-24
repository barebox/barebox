// SPDX-License-Identifier: GPL-2.0-only

#include <asm/barebox-arm.h>
#include <asm/cache.h>
#include <common.h>
#include <linux/sizes.h>
#include <asm/unaligned.h>
#include <mach/bcm283x/platform.h>
#include <debug_ll.h>
#include <mach/bcm283x/debug_ll.h>
#include <mach/bcm283x/mbox.h>
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

static inline void start_raspberry_pi(unsigned long memsize, void *fdt,
								void *vc_fdt)
{
	unsigned long endmem;

	/*
	 * A pointer to the FDT created by VideoCore was passed to us in x0/r2. We
	 * reserve some memory at the end of SDRAM copy this FDT there. We fetch it
	 * from there later in rpi_devices_init().
	 */
	memsize -= VIDEOCORE_FDT_SZ;
	endmem = BCM2835_SDRAM_BASE + memsize;

	/* leave SZ_1K for the initial stack */
	copy_vc_fdt((void *)endmem, vc_fdt, VIDEOCORE_FDT_SZ - SZ_1K);

	fdt += get_runtime_offset();

	barebox_arm_entry(BCM2835_SDRAM_BASE, memsize, fdt);
}

#ifdef CONFIG_CPU_V8
#define RPI_ENTRY_FUNCTION(name, memsize, fdt) \
	ENTRY_FUNCTION_WITHSTACK(name, BCM2835_SDRAM_BASE + (memsize), fdt, __x1, __x2)
#else
#define RPI_ENTRY_FUNCTION(name, memsize, fdt) \
	ENTRY_FUNCTION_WITHSTACK(name, BCM2835_SDRAM_BASE + (memsize), __r0, __r1, fdt)
#endif

extern char __dtb_z_bcm2835_rpi_start[];
extern char __dtb_z_bcm2836_rpi_2_start[];
extern char __dtb_z_bcm2837_rpi_3_start[];
extern char __dtb_z_bcm2837_rpi_cm3_start[];
extern char __dtb_z_bcm2711_rpi_4_start[];

RPI_ENTRY_FUNCTION(start_raspberry_pi1, SZ_128M, fdt)
{
	arm_cpu_lowlevel_init();

	start_raspberry_pi(SZ_128M, __dtb_z_bcm2835_rpi_start, (void *)fdt);
}

RPI_ENTRY_FUNCTION(start_raspberry_pi2, SZ_512M, fdt)
{
	arm_cpu_lowlevel_init();

	start_raspberry_pi(SZ_512M, __dtb_z_bcm2836_rpi_2_start, (void *)fdt);
}

RPI_ENTRY_FUNCTION(start_raspberry_pi3, SZ_512M, fdt)
{
	arm_cpu_lowlevel_init();

	start_raspberry_pi(SZ_512M, __dtb_z_bcm2837_rpi_3_start, (void *)fdt);
}

RPI_ENTRY_FUNCTION(start_raspberry_pi_cm3, SZ_512M, fdt)
{
	arm_cpu_lowlevel_init();

	start_raspberry_pi(SZ_512M, __dtb_z_bcm2837_rpi_cm3_start, (void *)fdt);
}

#define DT_IF_ENABLED(dt, cfg) \
	(IS_ENABLED(cfg) ? (dt) : NULL)

static void *rpi_get_board_fdt(int rev)
{
	if (!(rev & 0x800000))
		return DT_IF_ENABLED(__dtb_z_bcm2835_rpi_start, CONFIG_MACH_RPI);

	switch (((rev >> 4) & 0xff)) {
	case BCM2835_BOARD_REV_A:
	case BCM2835_BOARD_REV_B:
	case BCM2835_BOARD_REV_A_PLUS:
	case BCM2835_BOARD_REV_B_PLUS:
	case BCM2835_BOARD_REV_CM1:
	case BCM2835_BOARD_REV_ZERO:
	case BCM2835_BOARD_REV_ZERO_W:
		return DT_IF_ENABLED(__dtb_z_bcm2835_rpi_start, CONFIG_MACH_RPI);

	case BCM2836_BOARD_REV_2_B:
		return DT_IF_ENABLED(__dtb_z_bcm2836_rpi_2_start, CONFIG_MACH_RPI2);

	case BCM2837_BOARD_REV_3_B:
	case BCM2837B0_BOARD_REV_3B_PLUS:
	case BCM2837B0_BOARD_REV_3A_PLUS:
	case BCM2837B0_BOARD_REV_ZERO_2:
		return DT_IF_ENABLED(__dtb_z_bcm2837_rpi_3_start, CONFIG_MACH_RPI3);

	case BCM2837_BOARD_REV_CM3:
	case BCM2837B0_BOARD_REV_CM3_PLUS:
		return DT_IF_ENABLED(__dtb_z_bcm2837_rpi_cm3_start, CONFIG_MACH_RPI_CM3);

	case BCM2711_BOARD_REV_4_B:
	case BCM2711_BOARD_REV_400:
	case BCM2711_BOARD_REV_CM4:
		return DT_IF_ENABLED(__dtb_z_bcm2711_rpi_4_start, CONFIG_MACH_RPI4);
	}

	return NULL;
}

RPI_ENTRY_FUNCTION(start_raspberry_pi_generic, SZ_128M, vc_fdt)
{
	void *fdt = NULL;
	ssize_t memsize;
	int rev;

	arm_cpu_lowlevel_init();

	debug_ll_init();

	putc_ll('>');

	relocate_to_current_adr();
	setup_c();

	memsize = rpi_get_arm_mem();
	if (memsize < 0) {
		pr_warn("mbox: failed to query ARM memory size. 128M assumed.\n");
		memsize = SZ_128M;
	}

	rev = rpi_get_board_rev();
	if (rev >= 0) {
		pr_debug("Detected revision %08x\n", rev);
		fdt = rpi_get_board_fdt(rev);
	}

	if (!fdt) {
		fdt = (void *)vc_fdt;

		pr_warn("Unknown Rpi board with rev %08x.\n", rev);

		if (get_unaligned_be32(fdt) != 0xd00dfeed)
			panic("No suitable built-in or videocore-supplied DT\n");
	}

	start_raspberry_pi(memsize, fdt, (void *)vc_fdt);
}

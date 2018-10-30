/*
 * Copyright (C) 2018 Ahmad Fatoum, Pengutronix
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>

#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

#include <mach/hardware.h>

extern char __dtb_at91_microchip_ksz9477_evb_start[];

ENTRY_FUNCTION(start_sama5d3_xplained_ung8071, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	arm_setup_stack(SAMA5D3_SRAM_BASE + SAMA5D3_SRAM_SIZE - 16);

	fdt = __dtb_at91_microchip_ksz9477_evb_start + get_runtime_offset();

	barebox_arm_entry(SAMA5_DDRCS, SZ_256M, fdt);
}

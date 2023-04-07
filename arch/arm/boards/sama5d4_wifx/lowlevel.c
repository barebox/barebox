// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix

#include <debug_ll.h>
#include <mach/at91/barebox-arm.h>
#include <mach/at91/ddramc.h>

SAMA5D4_ENTRY_FUNCTION(start_sama5d4_wifx_l1, r4)
{
	extern char __dtb_z_at91_sama5d4_wifx_l1_start[];
	void *fdt;

	putc_ll('>');

	fdt = __dtb_z_at91_sama5d4_wifx_l1_start + get_runtime_offset();

	sama5d4_barebox_entry(r4, fdt);
}

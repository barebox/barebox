// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 Oleksij Rempel <o.rempel@pengutronix.de>
 */

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>

extern char __dtb_z_novarq_tactical_1000_start[];

/*
 * Novarq Tactical 1000 memory map.
 *
 * DDR is mapped at 0x60000000. The 896 MiB used here is deliberately
 * conservative and is NOT a measured figure for this board: it is
 * u-boot's compile-time default LAN969X_DDR_SIZE_DEF from
 * include/configs/lan969x.h, whose comment attributes the missing
 * eighth of the gigabyte to ECC. LAN969X uses in-line ECC, which does
 * consume roughly an eighth of the array, so the ratio is plausible,
 * but nothing here has been checked against the fitted parts.
 *
 * u-boot itself only uses that constant as a fallback: dram_init()
 * asks TF-A via tfa_get_dram_size() first. Under-reporting is safe -
 * barebox simply uses less of the array - so this is fine for
 * bring-up, but querying TF-A for the real size is the correct fix and
 * is still to be done.
 *
 * The top 2 MiB is kept out of the region handed to barebox. There is
 * no derivation behind that number: it is what worked when starting
 * barebox as a second stage after u-boot, and without it the handover
 * did not come up.
 *
 * FIXME: find out whether this is still needed, and whether 2 MiB is
 * the right amount, on a system running TF-A plus barebox only.
 */
#define TACTICAL1000_DRAM_BASE		UL(0x60000000)
#define TACTICAL1000_DRAM_SIZE		(896 * SZ_1M)
#define TACTICAL1000_UBOOT_RESERVE	SZ_2M
#define TACTICAL1000_DRAM_USABLE	(TACTICAL1000_DRAM_SIZE - TACTICAL1000_UBOOT_RESERVE)
#define TACTICAL1000_DRAM_USABLE_END	(TACTICAL1000_DRAM_BASE + TACTICAL1000_DRAM_USABLE)

ENTRY_FUNCTION_WITHSTACK(start_novarq_tactical_1000,
			 TACTICAL1000_DRAM_USABLE_END, r0, r1, r2)
{
	arm_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();

	barebox_arm_entry(TACTICAL1000_DRAM_BASE, TACTICAL1000_DRAM_USABLE,
			  runtime_address(__dtb_z_novarq_tactical_1000_start));
}

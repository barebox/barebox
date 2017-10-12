/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/system_info.h>
#include <linux/amba/sp804.h>

static inline void start_vexpress_common(void *internal_dt)
{
	void *fdt = internal_dt - get_runtime_offset();

	arm_cpu_lowlevel_init();

	if (amba_is_arm_sp804(IOMEM(0x10011000)))
		barebox_arm_entry(0x60000000, SZ_512M, fdt);
	else
		barebox_arm_entry(0x80000000, SZ_512M, fdt);
}

extern char __dtb_vexpress_v2p_ca9_start[];
ENTRY_FUNCTION(start_vexpress_ca9, r0, r1, r2)
{
	start_vexpress_common(__dtb_vexpress_v2p_ca9_start);
}

extern char __dtb_vexpress_v2p_ca15_start[];
ENTRY_FUNCTION(start_vexpress_ca15, r0, r1, r2)
{
	start_vexpress_common(__dtb_vexpress_v2p_ca15_start);
}

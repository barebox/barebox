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
#include <asm/unaligned.h>
#include <linux/amba/sp804.h>

static inline void start_vexpress_common(void *internal_dt)
{
	void *fdt = internal_dt - get_runtime_offset();
	unsigned long membase, memsize = SZ_512M;

	arm_cpu_lowlevel_init();

	if (amba_is_arm_sp804(IOMEM(0x10011000)))
		membase = 0x60000000;
	else
		membase = 0x80000000;

	/* QEMU may put a DTB at the start of RAM */
	if (IS_ENABLED(CONFIG_OFDEVICE) &&
	    get_unaligned_be32((void*)membase) == FDT_MAGIC) {
		fdt = (void*)membase;
		/*
		 * Need to move membase a bit as the PBL wants to relocate
		 * to the start of RAM, which would overwrite the DTB.
		 */
		membase += SZ_4M;
		memsize -= SZ_4M;
	}

	barebox_arm_entry(membase, memsize, fdt);
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

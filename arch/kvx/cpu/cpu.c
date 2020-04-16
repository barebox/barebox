// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <common.h>
#include <asm/sfr.h>
#include <asm/sys_arch.h>

extern char __dtb_start[];

/* Default to builtin dtb */
void *boot_dtb = __dtb_start;

void kvx_lowlevel_setup(unsigned long r0, void *dtb_ptr)
{
	uint64_t ev_val = (uint64_t) &_exception_start | EXCEPTION_STRIDE;

	if (r0 == FSBL_PARAM_MAGIC) {
		boot_dtb = dtb_ptr;
		pr_info("Using DTB provided by FSBL\n");
	}

	/* Install exception handlers */
	kvx_sfr_set(EV, ev_val);

	/* Clear exception taken bit now that we setup our handlers */
	kvx_sfr_set_field(PS, ET, 0);

	/* Finally, make sure nobody disabled hardware trap before us */
	kvx_sfr_set_field(PS, HTD, 0);
}

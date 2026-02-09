// SPDX-License-Identifier: GPL-2.0-only
/*
 * TLB configuration for QEMU ppce500
 *
 * QEMU sets up TLB entries before jumping to firmware, so we
 * provide a minimal table here just for the API.
 */

#include <linux/types.h>
#include <linux/array_size.h>
#include <mach/mmu.h>

struct fsl_e_tlb_entry tlb_table[] = {
	/* QEMU already configured TLB - empty table */
};

int num_tlb_entries = ARRAY_SIZE(tlb_table);

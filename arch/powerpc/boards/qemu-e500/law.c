// SPDX-License-Identifier: GPL-2.0-only
/*
 * LAW configuration for QEMU ppce500
 *
 * QEMU virtual hardware doesn't require LAW (Local Access Window)
 * configuration, so we provide an empty table.
 */

#include <linux/types.h>
#include <linux/array_size.h>
#include <asm/fsl_law.h>

struct law_entry law_table[] = {
	/* No LAW entries needed for QEMU virtual hardware */
};

int num_law_entries = ARRAY_SIZE(law_table);

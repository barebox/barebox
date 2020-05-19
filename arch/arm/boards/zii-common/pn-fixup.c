// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2019 Zodiac Inflight Innovation

#include <common.h>
#include <init.h>
#include <linux/nvmem-consumer.h>

#include "pn-fixup.h"

char *zii_read_part_number(const char *cell_name, size_t cell_size)
{
	struct device_node *np;

	np = of_find_node_by_name(NULL, "device-info");
	if (!np) {
		pr_warn("No device information found\n");
		return ERR_PTR(-ENOENT);
	}

	return nvmem_cell_get_and_read(np, cell_name, cell_size);
}

/*
 * Copyright (C) 2019 Zodiac Inflight Innovation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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

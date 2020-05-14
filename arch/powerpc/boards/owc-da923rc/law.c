/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
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
#include <asm/fsl_law.h>

struct law_entry law_table[] = {
	FSL_SET_LAW(0xf8000000, LAW_SIZE_128M, LAW_TRGT_IF_LBC),
	FSL_SET_LAW(0xc0000000, LAW_SIZE_256M, LAW_TRGT_IF_PCI),
	FSL_SET_LAW(0xe1000000, LAW_SIZE_64K, LAW_TRGT_IF_PCI),
};

int num_law_entries = ARRAY_SIZE(law_table);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Michael Tretter <m.tretter@pengutronix.de>
 */

#include <common.h>
#include <mach/zynqmp-bbu.h>

int zynqmp_bbu_register_handler(const char *name, char *devicefile,
				unsigned long flags)
{
	return bbu_register_std_file_update(name, flags, devicefile,
					    filetype_zynq_image);
}

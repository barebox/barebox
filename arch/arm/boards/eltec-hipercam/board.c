// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2015 Sascha Hauer <s.hauer@pengutronix.de>

#include <common.h>
#include <init.h>
#include <bbu.h>
#include <mach/bbu.h>

static int hipercam_init(void)
{
	if (!of_machine_is_compatible("eltec,hipercam-rev01"))
		return 0;

	imx6_bbu_internal_spi_i2c_register_handler("nor", "/dev/m25p0.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_hostname("hipercam");

	return 0;
}
device_initcall(hipercam_init);

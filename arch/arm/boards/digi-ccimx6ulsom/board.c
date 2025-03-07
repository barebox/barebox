// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2019 Rouven Czerwinski, Pengutronix

#include <common.h>
#include <init.h>
#include <mach/imx/generic.h>
#include <mach/imx/bbu.h>

static int digi_ccimx6ulsbcpro_device_init(void)
{
	if (!of_machine_is_compatible("digi,ccimx6ulsbcpro"))
		return 0;

	imx6_bbu_nand_register_handler("nand", "/dev/nand0.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_hostname("ccimx6ulsbcpro");

	return 0;
}
device_initcall(digi_ccimx6ulsbcpro_device_init);

// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Lucas Stach <l.stach@pengutronix.de>

#include <common.h>
#include <init.h>

static int toradex_colibri_t20_device_init(void)
{
	if (!of_machine_is_compatible("toradex,colibri_t20-512"))
		return 0;

	barebox_set_hostname("colibri-t20");

	return 0;
}
device_initcall(toradex_colibri_t20_device_init);

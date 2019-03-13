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
#include <globalvar.h>
#include <init.h>

static int rdu_networkconfig(void)
{
	static char *rdu_netconfig;
	struct device_d *sp_dev;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx51-rdu1"))
		return 0;

	sp_dev = get_device_by_name("sp");
	if (!sp_dev) {
		pr_warn("no sp device found, network config not available!\n");
		return -ENODEV;
	}

	rdu_netconfig = basprintf("ip=%s:::%s::eth0:",
				  dev_get_param(sp_dev, "ipaddr"),
				  dev_get_param(sp_dev, "netmask"));

	globalvar_add_simple_string("linux.bootargs.rdu_network",
				    &rdu_netconfig);

	return 0;
}
late_initcall(rdu_networkconfig);

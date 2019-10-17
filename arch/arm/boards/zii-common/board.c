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
#include <fs.h>
#include <net.h>
#include <linux/nvmem-consumer.h>

static int rdu_eth_register_ethaddr(struct device_node *np)
{
	u8 mac[ETH_ALEN];
	u8 *data;
	int i;

	data = nvmem_cell_get_and_read(np, "mac-address", ETH_ALEN);
	if (IS_ERR(data))
		return PTR_ERR(data);
	/*
	 * EEPROM stores MAC address in reverse (to what we expect it
	 * to be) byte order.
	 */
	for (i = 0; i < ETH_ALEN; i++)
		mac[i] = data[ETH_ALEN - i - 1];

	free(data);

	of_eth_register_ethaddr(np, mac);

	return 0;
}

static int rdu_ethernet_init(void)
{
	static const char * const aliases[] = { "ethernet0", "ethernet1" };
	struct device_node *np, *root;
	int i, ret;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx8mq-ultra"))
		return 0;

	root = of_get_root_node();

	for (i = 0; i < ARRAY_SIZE(aliases); i++) {
		const char *alias = aliases[i];

		np = of_find_node_by_alias(root, alias);
		if (!np) {
			pr_warn("Failed to find %s\n", alias);
			continue;
		}

		ret = rdu_eth_register_ethaddr(np);
		if (ret) {
			pr_warn("Failed to register MAC for %s\n", alias);
			continue;
		}
	}

	return 0;
}
late_initcall(rdu_ethernet_init);

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

#define I210_CFGWORD_PCIID_157B		0x157b1a11
static int rdu_i210_invm(void)
{
	int fd;
	u32 val;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx8mq-ultra"))
		return 0;

	fd = open("/dev/e1000-invm0", O_RDWR);
	if (fd < 0) {
		pr_err("could not open e1000 iNVM device!\n");
		return fd;
	}

	pread(fd, &val, sizeof(val), 0);
	if (val == I210_CFGWORD_PCIID_157B) {
		pr_debug("i210 already programmed correctly\n");
		return 0;
	}

	val = I210_CFGWORD_PCIID_157B;
	pwrite(fd, &val, sizeof(val), 0);

	return 0;
}
late_initcall(rdu_i210_invm);

/*
 * ocotp.c - i.MX6 ocotp fusebox driver
 *
 * Provide an interface for programming and sensing the information that are
 * stored in on-chip fuse elements. This functionality is part of the IC
 * Identification Module (IIM), which is present on some i.MX CPUs.
 *
 * Copyright (c) 2010 Baruch Siach <baruch@tkos.co.il>,
 * 	Orex Computed Radiography
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <net.h>
#include <io.h>

/*
 * a single MAC address reference has the form
 * <&phandle regoffset>
 */
#define MAC_ADDRESS_PROPLEN	(2 * sizeof(__be32))

static void imx_ocotp_init_dt(struct device_d *dev, void __iomem *base)
{
	char mac[6];
	const __be32 *prop;
	struct device_node *node = dev->device_node;
	int len;

	if (!node)
		return;

	prop = of_get_property(node, "barebox,provide-mac-address", &len);
	if (!prop)
		return;

	while (len >= MAC_ADDRESS_PROPLEN) {
		struct device_node *rnode;
		uint32_t phandle, offset, value;

		phandle = be32_to_cpup(prop++);

		rnode = of_find_node_by_phandle(phandle);
		offset = be32_to_cpup(prop++);

		value = readl(base + offset + 0x10);
		mac[0] = (value >> 8);
		mac[1] = value;
		value = readl(base + offset);
		mac[2] = value >> 24;
		mac[3] = value >> 16;
		mac[4] = value >> 8;
		mac[5] = value;

		of_eth_register_ethaddr(rnode, mac);

		len -= MAC_ADDRESS_PROPLEN;
	}
}

static int imx_ocotp_probe(struct device_d *dev)
{
	void __iomem *base;

	base = dev_request_mem_region(dev, 0);
	if (!base)
		return -EBUSY;

	imx_ocotp_init_dt(dev, base);

	return 0;
}

static __maybe_unused struct of_device_id imx_ocotp_dt_ids[] = {
	{
		.compatible = "fsl,imx6q-ocotp",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_ocotp_driver = {
	.name	= "imx_ocotp",
	.probe	= imx_ocotp_probe,
	.of_compatible = DRV_OF_COMPAT(imx_ocotp_dt_ids),
};

static int imx_ocotp_init(void)
{
	platform_driver_register(&imx_ocotp_driver);

	return 0;
}
coredevice_initcall(imx_ocotp_init);

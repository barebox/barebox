// SPDX-License-Identifier: GPL-2.0-only
/*
 * GPIO based MDIO bitbang driver.
 * Supports OpenFirmware.
 *
 * (C) Copyright 2015
 *  CogentEmbedded, Andrey Gusakov <andrey.gusakov@cogentembedded.com>
 *
 * based on mvmdio driver from Linux
 *  Copyright (c) 2008 CSE Semaphore Belgium.
 *   by Laurent Pinchart <laurentp@cse-semaphore.com>
 *
 * Copyright (C) 2008, Paulius Zaleckas <paulius.zaleckas@teltonika.lt>
 *
 * Based on earlier work by
 *
 * Copyright (c) 2003 Intracom S.A.
 *  by Pantelis Antoniou <panto@intracom.gr>
 *
 * 2005 (c) MontaVista Software, Inc.
 * Vitaly Bordug <vbordug@ru.mvista.com>
 */

#define DEBUG

#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <of_gpio.h>
#include <linux/phy.h>
#include <gpio.h>
#include <malloc.h>
#include <xfuncs.h>

#include <linux/mdio-bitbang.h>

struct mdio_gpio_info {
	struct mdiobb_ctrl ctrl;
	int mdc, mdio, mdo;
	int mdc_active_low, mdio_active_low, mdo_active_low;
};

static struct mdio_gpio_info *mdio_gpio_of_get_info(struct device_d *dev)
{
	int ret;
	enum of_gpio_flags flags;
	struct mdio_gpio_info *info;

	info = xzalloc(sizeof(*info));

	ret = of_get_gpio_flags(dev->device_node, 0, &flags);
	if (ret < 0) {
		dev_dbg(dev, "failed to get MDC inforamtion from DT\n");
		goto free_info;
	}

	info->mdc = ret;
	info->mdc_active_low = flags & OF_GPIO_ACTIVE_LOW;

	ret = of_get_gpio_flags(dev->device_node, 1, &flags);
	if (ret < 0) {
		dev_dbg(dev, "failed to get MDIO inforamtion from DT\n");
		goto free_info;
	}

	info->mdio = ret;
	info->mdio_active_low = flags & OF_GPIO_ACTIVE_LOW;

	ret = of_get_gpio_flags(dev->device_node, 2, &flags);
	if (ret > 0) {
		dev_dbg(dev, "found MDO information in DT\n");
		info->mdo = ret;
		info->mdo_active_low = flags & OF_GPIO_ACTIVE_LOW;
	}

	return info;

free_info:
	free(info);
	return ERR_PTR(ret);
}

static void mdio_dir(struct mdiobb_ctrl *ctrl, int dir)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	if (bitbang->mdo) {
		/* Separate output pin. Always set its value to high
		 * when changing direction. If direction is input,
		 * assume the pin serves as pull-up. If direction is
		 * output, the default value is high.
		 */
		gpio_set_value(bitbang->mdo,
				1 ^ bitbang->mdo_active_low);
		return;
	}

	if (dir)
		gpio_direction_output(bitbang->mdio,
				      1 ^ bitbang->mdio_active_low);
	else
		gpio_direction_input(bitbang->mdio);
}

static int mdio_get(struct mdiobb_ctrl *ctrl)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	return gpio_get_value(bitbang->mdio) ^
		bitbang->mdio_active_low;
}

static void mdio_set(struct mdiobb_ctrl *ctrl, int what)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	if (bitbang->mdo)
		gpio_set_value(bitbang->mdo,
				what ^ bitbang->mdo_active_low);
	else
		gpio_set_value(bitbang->mdio,
				what ^ bitbang->mdio_active_low);
}

static void mdc_set(struct mdiobb_ctrl *ctrl, int what)
{
	struct mdio_gpio_info *bitbang =
		container_of(ctrl, struct mdio_gpio_info, ctrl);

	gpio_set_value(bitbang->mdc, what ^ bitbang->mdc_active_low);
}

static struct mdiobb_ops mdio_gpio_ops = {
	.set_mdc = mdc_set,
	.set_mdio_dir = mdio_dir,
	.set_mdio_data = mdio_set,
	.get_mdio_data = mdio_get,
};

static int mdio_gpio_probe(struct device_d *dev)
{
	int ret;
	struct device_node *np = dev->device_node;
	struct mdio_gpio_info *info;
	struct mii_bus *bus;

	info = mdio_gpio_of_get_info(dev);
	if (IS_ERR(info))
		return PTR_ERR(info);

	info->ctrl.ops = &mdio_gpio_ops;

	ret = gpio_request(info->mdc, "mdc");
	if (ret < 0) {
		dev_dbg(dev, "failed to request MDC\n");
		goto free_info;
	}

	ret = gpio_request(info->mdio, "mdio");
	if (ret < 0) {
		dev_dbg(dev, "failed to request MDIO\n");
		goto free_mdc;
	}

	if (info->mdo) {
		ret = gpio_request(info->mdo, "mdo");
		if (ret < 0) {
			dev_dbg(dev, "failed to request MDO\n");
			goto free_mdio;
		}

		ret = gpio_direction_output(info->mdo, 1);
		if (ret < 0) {
			dev_dbg(dev, "failed to set MDO as output\n");
			goto free_mdo;
		}

		ret = gpio_direction_input(info->mdio);
		if (ret < 0) {
			dev_dbg(dev, "failed to set MDIO as input\n");
			goto free_mdo;
		}
	}

	ret = gpio_direction_output(info->mdc, 0);
	if (ret < 0) {
		dev_dbg(dev, "failed to set MDC as output\n");
		goto free_mdo;
	}

	bus = alloc_mdio_bitbang(&info->ctrl);
	bus->parent = dev;
	bus->dev.device_node = np;

	dev->priv = bus;

	ret = mdiobus_register(bus);
	if (!ret)
		return 0;

	free(bus);
free_mdo:
	gpio_free(info->mdo);
free_mdc:
	gpio_free(info->mdc);
free_mdio:
	gpio_free(info->mdio);
free_info:
	free(info);
	return ret;
}

static const struct of_device_id gpio_mdio_dt_ids[] = {
	{ .compatible = "virtual,mdio-gpio", },
	{ /* sentinel */ }
};

static struct driver_d mdio_gpio_driver = {
	.name = "mdio-gpio",
	.probe = mdio_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(gpio_mdio_dt_ids),
};
device_platform_driver(mdio_gpio_driver);

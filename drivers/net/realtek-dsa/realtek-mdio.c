// SPDX-License-Identifier: GPL-2.0+
/* Realtek MDIO interface driver
 *
 * ASICs we intend to support with this driver:
 *
 * RTL8366   - The original version, apparently
 * RTL8369   - Similar enough to have the same datsheet as RTL8366
 * RTL8366RB - Probably reads out "RTL8366 revision B", has a quite
 *             different register layout from the other two
 * RTL8366S  - Is this "RTL8366 super"?
 * RTL8367   - Has an OpenWRT driver as well
 * RTL8368S  - Seems to be an alternative name for RTL8366RB
 * RTL8370   - Also uses SMI
 *
 * Copyright (C) 2017 Linus Walleij <linus.walleij@linaro.org>
 * Copyright (C) 2010 Antti Seppälä <a.seppala@gmail.com>
 * Copyright (C) 2010 Roman Yeryomin <roman@advem.lv>
 * Copyright (C) 2011 Colin Leitner <colin.leitner@googlemail.com>
 * Copyright (C) 2009-2010 Gabor Juhos <juhosg@openwrt.org>
 */

#include <of_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <clock.h>
#include <linux/gpio/consumer.h>
#include <linux/printk.h>
#include <linux/mdio.h>

#include "realtek.h"

/* Read/write via mdiobus */
#define REALTEK_MDIO_CTRL0_REG		31
#define REALTEK_MDIO_START_REG		29
#define REALTEK_MDIO_CTRL1_REG		21
#define REALTEK_MDIO_ADDRESS_REG	23
#define REALTEK_MDIO_DATA_WRITE_REG	24
#define REALTEK_MDIO_DATA_READ_REG	25

#define REALTEK_MDIO_START_OP		0xFFFF
#define REALTEK_MDIO_ADDR_OP		0x000E
#define REALTEK_MDIO_READ_OP		0x0001
#define REALTEK_MDIO_WRITE_OP		0x0003

static int realtek_mdio_write(void *ctx, u32 reg, u32 val)
{
	struct realtek_priv *priv = ctx;
	struct mii_bus *bus = priv->bus;
	int ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_CTRL0_REG,
			    REALTEK_MDIO_ADDR_OP);
	if (ret)
		return ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_ADDRESS_REG,
			    reg);
	if (ret)
		return ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_DATA_WRITE_REG,
			    val);
	if (ret)
		return ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_CTRL1_REG,
			    REALTEK_MDIO_WRITE_OP);
	if (ret)
		return ret;

	return 0;
}

static int realtek_mdio_read(void *ctx, u32 reg, u32 *val)
{
	struct realtek_priv *priv = ctx;
	struct mii_bus *bus = priv->bus;
	int ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_CTRL0_REG,
			    REALTEK_MDIO_ADDR_OP);
	if (ret)
		return ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_ADDRESS_REG,
			    reg);
	if (ret)
		return ret;

	ret = mdiobus_write(bus, priv->mdio_addr, REALTEK_MDIO_CTRL1_REG,
			    REALTEK_MDIO_READ_OP);
	if (ret)
		return ret;

	ret = mdiobus_read(bus, priv->mdio_addr, REALTEK_MDIO_DATA_READ_REG);
	if (ret < 0)
		return ret;

	*val = ret;

	return 0;
}

static int realtek_mdio_mdio_write(struct mii_bus *bus, int addr, int regnum,
				   u16 val)
{
	struct realtek_priv *priv = bus->priv;

	return priv->ops->phy_write(priv, addr, regnum, val);
}

static int realtek_mdio_mdio_read(struct mii_bus *bus, int addr, int regnum)
{
	struct realtek_priv *priv = bus->priv;

	return priv->ops->phy_read(priv, addr, regnum);
}

static const struct regmap_config realtek_mdio_regmap_config = {
	.reg_bits = 10, /* A4..A0 R4..R0 */
	.val_bits = 16,
	.reg_stride = 1,
	/* PHY regs are at 0x8000 */
	.max_register = 0xffff,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
};

static const struct regmap_bus realtek_mdio_regmap_bus = {
	.reg_write = realtek_mdio_write,
	.reg_read = realtek_mdio_read,
};

static int realtek_mdio_setup_mdio(struct dsa_switch *ds)
{
	struct realtek_priv *priv =  ds->priv;
	struct device_node *np;
	int ret;

	np = of_get_child_by_name(priv->dev->of_node, "mdio");
	if (!np) {
		dev_err(priv->dev, "missing 'mdio' child node\n");
		return -ENODEV;
	}

	priv->slave_mii_bus->priv = priv;
	priv->slave_mii_bus->read = realtek_mdio_mdio_read;
	priv->slave_mii_bus->write = realtek_mdio_mdio_write;
	priv->slave_mii_bus->dev.of_node = np;
	priv->slave_mii_bus->parent = priv->dev;

	ret = mdiobus_register(priv->slave_mii_bus);
	if (ret) {
		dev_err(priv->dev, "unable to register MDIO bus %pOF\n", np);
		goto err_put_node;
	}

	/* Avoid interleaved MDIO access during PHY status polling */
	slice_depends_on(mdiobus_slice(priv->slave_mii_bus),
			 mdiobus_slice(priv->bus));

	return 0;

err_put_node:
	of_node_put(np);

	return ret;
}

static int realtek_mdio_probe(struct phy_device *mdiodev)
{
	struct realtek_priv *priv;
	struct device *dev = &mdiodev->dev;
	const struct realtek_variant *var;
	struct regmap_config rc;
	struct device_node *np;
	int ret;

	var = of_device_get_match_data(dev);
	if (!var)
		return -EINVAL;

	priv = kzalloc(sizeof(*priv) + var->chip_data_sz, GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	rc = realtek_mdio_regmap_config;
	priv->map = regmap_init(dev, &realtek_mdio_regmap_bus, priv, &rc);
	if (IS_ERR(priv->map)) {
		ret = PTR_ERR(priv->map);
		dev_err(dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	priv->mdio_addr = mdiodev->addr;
	priv->bus = mdiodev->bus;
	priv->dev = &mdiodev->dev;
	priv->chip_data = (void *)priv + sizeof(*priv);

	priv->clk_delay = var->clk_delay;
	priv->cmd_read = var->cmd_read;
	priv->cmd_write = var->cmd_write;
	priv->ops = var->ops;

	priv->setup_interface = realtek_mdio_setup_mdio;
	priv->write_reg_noack = realtek_mdio_write;

	np = dev->of_node;

	dev->priv = priv;

	/* TODO: if power is software controlled, set up any regulators here */
	priv->leds_disabled = of_property_read_bool(np, "realtek,disable-leds");

	priv->reset = gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(priv->reset))
		return dev_errp_probe(dev, priv->reset, "failed to get RESET GPIO\n");

	if (priv->reset) {
		gpiod_set_value(priv->reset, 1);
		dev_dbg(dev, "asserted RESET\n");
		mdelay(REALTEK_HW_STOP_DELAY);
		gpiod_set_value(priv->reset, 0);
		mdelay(REALTEK_HW_START_DELAY);
		dev_dbg(dev, "deasserted RESET\n");
	}

	ret = priv->ops->detect(priv);
	if (ret) {
		dev_err(dev, "unable to detect switch\n");
		return ret;
	}

	priv->ds = kzalloc(sizeof(*priv->ds), GFP_KERNEL);
	if (!priv->ds)
		return -ENOMEM;

	priv->ds->dev = dev;
	priv->ds->num_ports = priv->num_ports;
	priv->ds->priv = priv;
	priv->ds->ops = var->ds_ops;

	ret = realtek_dsa_init_tagger(priv);
	if (ret)
		return ret;

	ret = dsa_register_switch(priv->ds);
	if (ret) {
		dev_err(priv->dev, "unable to register switch ret = %d\n", ret);
		return ret;
	}

	return priv->ops->setup ? priv->ops->setup(priv) : 0;
}

static void realtek_mdio_remove(struct phy_device *mdiodev)
{
	struct realtek_priv *priv = mdiodev->dev.priv;

	/* leave the device reset asserted */
	gpiod_set_value(priv->reset, 1);
}

static const struct of_device_id realtek_mdio_of_match[] = {
#if IS_ENABLED(CONFIG_NET_DSA_REALTEK_RTL8366RB)
	{ .compatible = "realtek,rtl8366rb", .data = &rtl8366rb_variant, },
#endif
#if IS_ENABLED(CONFIG_NET_DSA_REALTEK_RTL8365MB)
	{ .compatible = "realtek,rtl8365mb", .data = &rtl8365mb_variant, },
#endif
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, realtek_mdio_of_match);
MODULE_DEVICE_TABLE(of, realtek_mdio_of_match);

static struct phy_driver realtek_mdio_driver = {
	.drv = {
		.name = "realtek-mdio",
		.of_match_table = of_match_ptr(realtek_mdio_of_match),
	},
	.probe  = realtek_mdio_probe,
	.remove = realtek_mdio_remove,
};

device_mdio_driver(realtek_mdio_driver);

MODULE_AUTHOR("Luiz Angelo Daros de Luca <luizluca@gmail.com>");
MODULE_DESCRIPTION("Driver for Realtek ethernet switch connected via MDIO interface");
MODULE_LICENSE("GPL");

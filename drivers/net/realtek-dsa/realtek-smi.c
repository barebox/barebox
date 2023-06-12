// SPDX-License-Identifier: GPL-2.0+
/* Realtek Simple Management Interface (SMI) driver
 * It can be discussed how "simple" this interface is.
 *
 * The SMI protocol piggy-backs the MDIO MDC and MDIO signals levels
 * but the protocol is not MDIO at all. Instead it is a Realtek
 * pecularity that need to bit-bang the lines in a special way to
 * communicate with the switch.
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

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <driver.h>
#include <of.h>
#include <of_device.h>
#include <linux/mdio.h>
#include <linux/printk.h>
#include <clock.h>
#include <gpiod.h>
#include <driver.h>
#include <regmap.h>
#include <linux/bitops.h>
#include <linux/if_bridge.h>


#include "realtek.h"

#define REALTEK_SMI_ACK_RETRY_COUNT		5

static inline void realtek_smi_clk_delay(struct realtek_priv *priv)
{
	ndelay(priv->clk_delay);
}

static void realtek_smi_start(struct realtek_priv *priv)
{
	/* Set GPIO pins to output mode, with initial state:
	 * SCK = 0, SDA = 1
	 */
	gpiod_direction_output(priv->mdc, 0);
	gpiod_direction_output(priv->mdio, 1);
	realtek_smi_clk_delay(priv);

	/* CLK 1: 0 -> 1, 1 -> 0 */
	gpiod_set_value(priv->mdc, 1);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 0);
	realtek_smi_clk_delay(priv);

	/* CLK 2: */
	gpiod_set_value(priv->mdc, 1);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdio, 0);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 0);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdio, 1);
}

static void realtek_smi_stop(struct realtek_priv *priv)
{
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdio, 0);
	gpiod_set_value(priv->mdc, 1);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdio, 1);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 1);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 0);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 1);

	/* Add a click */
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 0);
	realtek_smi_clk_delay(priv);
	gpiod_set_value(priv->mdc, 1);

	/* Set GPIO pins to input mode */
	gpiod_direction_input(priv->mdio);
	gpiod_direction_input(priv->mdc);
}

static void realtek_smi_write_bits(struct realtek_priv *priv, u32 data, u32 len)
{
	for (; len > 0; len--) {
		realtek_smi_clk_delay(priv);

		/* Prepare data */
		gpiod_set_value(priv->mdio, !!(data & (1 << (len - 1))));
		realtek_smi_clk_delay(priv);

		/* Clocking */
		gpiod_set_value(priv->mdc, 1);
		realtek_smi_clk_delay(priv);
		gpiod_set_value(priv->mdc, 0);
	}
}

static void realtek_smi_read_bits(struct realtek_priv *priv, u32 len, u32 *data)
{
	gpiod_direction_input(priv->mdio);

	for (*data = 0; len > 0; len--) {
		u32 u;

		realtek_smi_clk_delay(priv);

		/* Clocking */
		gpiod_set_value(priv->mdc, 1);
		realtek_smi_clk_delay(priv);
		u = !!gpiod_get_value(priv->mdio);
		gpiod_set_value(priv->mdc, 0);

		*data |= (u << (len - 1));
	}

	gpiod_direction_output(priv->mdio, 0);
}

static int realtek_smi_wait_for_ack(struct realtek_priv *priv)
{
	int retry_cnt;

	retry_cnt = 0;
	do {
		u32 ack;

		realtek_smi_read_bits(priv, 1, &ack);
		if (ack == 0)
			break;

		if (++retry_cnt > REALTEK_SMI_ACK_RETRY_COUNT) {
			dev_err(priv->dev, "ACK timeout\n");
			return -ETIMEDOUT;
		}
	} while (1);

	return 0;
}

static int realtek_smi_write_byte(struct realtek_priv *priv, u8 data)
{
	realtek_smi_write_bits(priv, data, 8);
	return realtek_smi_wait_for_ack(priv);
}

static int realtek_smi_write_byte_noack(struct realtek_priv *priv, u8 data)
{
	realtek_smi_write_bits(priv, data, 8);
	return 0;
}

static int realtek_smi_read_byte0(struct realtek_priv *priv, u8 *data)
{
	u32 t;

	/* Read data */
	realtek_smi_read_bits(priv, 8, &t);
	*data = (t & 0xff);

	/* Send an ACK */
	realtek_smi_write_bits(priv, 0x00, 1);

	return 0;
}

static int realtek_smi_read_byte1(struct realtek_priv *priv, u8 *data)
{
	u32 t;

	/* Read data */
	realtek_smi_read_bits(priv, 8, &t);
	*data = (t & 0xff);

	/* Send an ACK */
	realtek_smi_write_bits(priv, 0x01, 1);

	return 0;
}

static int realtek_smi_read_reg(struct realtek_priv *priv, u32 addr, u32 *data)
{
	unsigned long flags;
	u8 lo = 0;
	u8 hi = 0;
	int ret;

	spin_lock_irqsave(&priv->lock, flags);

	realtek_smi_start(priv);

	/* Send READ command */
	ret = realtek_smi_write_byte(priv, priv->cmd_read);
	if (ret)
		goto out;

	/* Set ADDR[7:0] */
	ret = realtek_smi_write_byte(priv, addr & 0xff);
	if (ret)
		goto out;

	/* Set ADDR[15:8] */
	ret = realtek_smi_write_byte(priv, addr >> 8);
	if (ret)
		goto out;

	/* Read DATA[7:0] */
	realtek_smi_read_byte0(priv, &lo);
	/* Read DATA[15:8] */
	realtek_smi_read_byte1(priv, &hi);

	*data = ((u32)lo) | (((u32)hi) << 8);

	ret = 0;

 out:
	realtek_smi_stop(priv);
	spin_unlock_irqrestore(&priv->lock, flags);

	return ret;
}

static int realtek_smi_write_reg(struct realtek_priv *priv,
				 u32 addr, u32 data, bool ack)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&priv->lock, flags);

	realtek_smi_start(priv);

	/* Send WRITE command */
	ret = realtek_smi_write_byte(priv, priv->cmd_write);
	if (ret)
		goto out;

	/* Set ADDR[7:0] */
	ret = realtek_smi_write_byte(priv, addr & 0xff);
	if (ret)
		goto out;

	/* Set ADDR[15:8] */
	ret = realtek_smi_write_byte(priv, addr >> 8);
	if (ret)
		goto out;

	/* Write DATA[7:0] */
	ret = realtek_smi_write_byte(priv, data & 0xff);
	if (ret)
		goto out;

	/* Write DATA[15:8] */
	if (ack)
		ret = realtek_smi_write_byte(priv, data >> 8);
	else
		ret = realtek_smi_write_byte_noack(priv, data >> 8);
	if (ret)
		goto out;

	ret = 0;

 out:
	realtek_smi_stop(priv);
	spin_unlock_irqrestore(&priv->lock, flags);

	return ret;
}

/* There is one single case when we need to use this accessor and that
 * is when issueing soft reset. Since the device reset as soon as we write
 * that bit, no ACK will come back for natural reasons.
 */
static int realtek_smi_write_reg_noack(void *ctx, u32 reg, u32 val)
{
	return realtek_smi_write_reg(ctx, reg, val, false);
}

/* Regmap accessors */

static int realtek_smi_write(void *ctx, u32 reg, u32 val)
{
	struct realtek_priv *priv = ctx;

	return realtek_smi_write_reg(priv, reg, val, true);
}

static int realtek_smi_read(void *ctx, u32 reg, u32 *val)
{
	struct realtek_priv *priv = ctx;

	return realtek_smi_read_reg(priv, reg, val);
}

static const struct regmap_config realtek_smi_regmap_config = {
	.reg_bits = 10, /* A4..A0 R4..R0 */
	.val_bits = 16,
	.reg_stride = 1,
	/* PHY regs are at 0x8000 */
	.max_register = 0xffff,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
};

static const struct regmap_bus realtek_smi_regmap_bus = {
	.reg_read = realtek_smi_read,
	.reg_write = realtek_smi_write,
};

static int realtek_smi_mdio_read(struct mii_bus *bus, int addr, int regnum)
{
	struct realtek_priv *priv = bus->priv;

	return priv->ops->phy_read(priv, addr, regnum);
}

static int realtek_smi_mdio_write(struct mii_bus *bus, int addr, int regnum,
				  u16 val)
{
	struct realtek_priv *priv = bus->priv;

	return priv->ops->phy_write(priv, addr, regnum, val);
}

static int realtek_smi_setup_mdio(struct dsa_switch *ds)
{
	struct realtek_priv *priv =  ds->priv;
	struct device_node *mdio_np;
	int ret;

	mdio_np = of_get_compatible_child(priv->dev->of_node, "realtek,smi-mdio");
	if (!mdio_np) {
		dev_err(priv->dev, "no MDIO bus node\n");
		return -ENODEV;
	}

	priv->slave_mii_bus->priv = priv;
	priv->slave_mii_bus->read = realtek_smi_mdio_read;
	priv->slave_mii_bus->write = realtek_smi_mdio_write;
	priv->slave_mii_bus->dev.of_node = mdio_np;
	priv->slave_mii_bus->parent = priv->dev;
	ds->slave_mii_bus = priv->slave_mii_bus;

	ret = mdiobus_register(priv->slave_mii_bus);
	if (ret) {
		dev_err(priv->dev, "unable to register MDIO bus %pOF\n",
			mdio_np);
		goto err_put_node;
	}

	return 0;

err_put_node:
	of_node_put(mdio_np);

	return ret;
}

static int realtek_smi_probe(struct device *dev)
{
	const struct realtek_variant *var;
	struct realtek_priv *priv;
	struct regmap_config rc;
	struct device_node *np;
	int ret;

	var = of_device_get_match_data(dev);
	np = dev->of_node;

	priv = kzalloc(sizeof(*priv) + var->chip_data_sz, GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->chip_data = (void *)priv + sizeof(*priv);

	rc = realtek_smi_regmap_config;
	priv->map = regmap_init(dev, &realtek_smi_regmap_bus, priv, &rc);
	if (IS_ERR(priv->map)) {
		ret = PTR_ERR(priv->map);
		dev_err(dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	/* Link forward and backward */
	priv->dev = dev;
	priv->clk_delay = var->clk_delay;
	priv->cmd_read = var->cmd_read;
	priv->cmd_write = var->cmd_write;
	priv->ops = var->ops;

	priv->setup_interface = realtek_smi_setup_mdio;
	priv->write_reg_noack = realtek_smi_write_reg_noack;

	dev->priv = priv;
	spin_lock_init(&priv->lock);

	/* TODO: if power is software controlled, set up any regulators here */

	priv->reset = gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (priv->reset < 0 && priv->reset != -ENOENT)
		return dev_err_probe(dev, priv->reset, "failed to get RESET GPIO\n");

	if (gpio_is_valid(priv->reset)) {
		gpiod_set_value(priv->reset, 1);
		dev_dbg(dev, "asserted RESET\n");
		mdelay(REALTEK_HW_STOP_DELAY);
		gpiod_set_value(priv->reset, 0);
		mdelay(REALTEK_HW_START_DELAY);
		dev_dbg(dev, "deasserted RESET\n");
	}

	/* Fetch MDIO pins */
	priv->mdc = gpiod_get(dev, "mdc", GPIOD_OUT_LOW);
	if (!gpio_is_valid(priv->mdc))
		return dev_err_probe(dev, priv->mdc, "failed to get MDC GPIO\n");

	priv->mdio = gpiod_get(dev, "mdio", GPIOD_OUT_LOW);
	if (!gpio_is_valid(priv->mdio))
		return dev_err_probe(dev, priv->mdio, "failed to get MDIO GPIO\n");

	priv->leds_disabled = of_property_read_bool(np, "realtek,disable-leds");

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
	priv->ds->ops = var->ds_ops_smi;

	ret = realtek_dsa_init_tagger(priv);
	if (ret)
		return ret;

	ret = dsa_register_switch(priv->ds);
	if (ret) {
		dev_err_probe(dev, ret, "unable to register switch\n");
		return ret;
	}

	return priv->ops->setup ? priv->ops->setup(priv) : 0;
}

static void realtek_smi_remove(struct device *dev)
{
	struct realtek_priv *priv = dev->priv;

	/* leave the device reset asserted */
	gpiod_set_value(priv->reset, 1);
}

static const struct of_device_id realtek_smi_of_match[] = {
#if IS_ENABLED(CONFIG_NET_DSA_REALTEK_RTL8366RB)
	{
		.compatible = "realtek,rtl8366rb",
		.data = &rtl8366rb_variant,
	},
#endif
#if IS_ENABLED(CONFIG_NET_DSA_REALTEK_RTL8365MB)
	{
		.compatible = "realtek,rtl8365mb",
		.data = &rtl8365mb_variant,
	},
#endif
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, realtek_smi_of_match);
MODULE_DEVICE_TABLE(of, realtek_smi_of_match);

static struct driver realtek_smi_driver = {
	.name = "realtek-smi",
	.of_match_table = of_match_ptr(realtek_smi_of_match),
	.probe  = realtek_smi_probe,
	.remove = realtek_smi_remove,
};
device_platform_driver(realtek_smi_driver);

MODULE_AUTHOR("Linus Walleij <linus.walleij@linaro.org>");
MODULE_DESCRIPTION("Driver for Realtek ethernet switch connected via SMI interface");
MODULE_LICENSE("GPL");

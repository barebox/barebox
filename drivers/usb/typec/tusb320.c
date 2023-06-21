// SPDX-License-Identifier: GPL-2.0
/*
 * Based on the Linux driver:
 *   drivers/typec/typec-tusb320.c - TUSB320 typec driver
 *
 * Copyright (C) 2020 National Instruments Corporation
 * Author: Michael Auchter <michael.auchter@ni.com>
 */

#include <linux/bitfield.h>
#include <i2c/i2c.h>
#include <init.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/bitops.h>
#include <module.h>
#include <regmap.h>
#include <linux/usb/typec.h>
#include <linux/usb/typec_altmode.h>

#define TUSB320_REG8				0x8
#define TUSB320_REG8_CURRENT_MODE_ADVERTISE	GENMASK(7, 6)
#define TUSB320_REG8_CURRENT_MODE_ADVERTISE_USB	0x0
#define TUSB320_REG8_CURRENT_MODE_ADVERTISE_15A	0x1
#define TUSB320_REG8_CURRENT_MODE_ADVERTISE_30A	0x2
#define TUSB320_REG8_CURRENT_MODE_DETECT	GENMASK(5, 4)
#define TUSB320_REG8_CURRENT_MODE_DETECT_DEF	0x0
#define TUSB320_REG8_CURRENT_MODE_DETECT_MED	0x1
#define TUSB320_REG8_CURRENT_MODE_DETECT_ACC	0x2
#define TUSB320_REG8_CURRENT_MODE_DETECT_HI	0x3
#define TUSB320_REG8_ACCESSORY_CONNECTED	GENMASK(3, 1)
#define TUSB320_REG8_ACCESSORY_CONNECTED_NONE	0x0
#define TUSB320_REG8_ACCESSORY_CONNECTED_AUDIO	0x4
#define TUSB320_REG8_ACCESSORY_CONNECTED_ACHRG	0x5
#define TUSB320_REG8_ACCESSORY_CONNECTED_DBGDFP	0x6
#define TUSB320_REG8_ACCESSORY_CONNECTED_DBGUFP	0x7
#define TUSB320_REG8_ACTIVE_CABLE_DETECTION	BIT(0)

#define TUSB320_REG9				0x9
#define TUSB320_REG9_ATTACHED_STATE		GENMASK(7, 6)
#define TUSB320_REG9_CABLE_DIRECTION		BIT(5)
#define TUSB320_REG9_INTERRUPT_STATUS		BIT(4)

enum tusb320_attached_state {
	TUSB320_ATTACHED_STATE_NONE,
	TUSB320_ATTACHED_STATE_DFP,
	TUSB320_ATTACHED_STATE_UFP,
	TUSB320_ATTACHED_STATE_ACC,
};

struct tusb320_priv {
	struct device *dev;
	struct regmap *regmap;
	struct typec_port *port;
	struct typec_capability	cap;
};

static int tusb320_typec_irq_handler(struct tusb320_priv *priv, u8 reg9)
{
	struct typec_port *port = priv->port;
	int typec_mode;
	enum typec_role pwr_role;
	enum usb_role usb_role;
	u8 state, accessory;
	int ret, reg8;

	ret = regmap_read(priv->regmap, TUSB320_REG8, &reg8);
	if (ret)
		return ret;

	state = FIELD_GET(TUSB320_REG9_ATTACHED_STATE, reg9);
	accessory = FIELD_GET(TUSB320_REG8_ACCESSORY_CONNECTED, reg8);

	switch (state) {
	case TUSB320_ATTACHED_STATE_DFP:
		typec_mode = TYPEC_MODE_USB2;
		usb_role = USB_ROLE_HOST;
		pwr_role = TYPEC_SOURCE;
		break;
	case TUSB320_ATTACHED_STATE_UFP:
		typec_mode = TYPEC_MODE_USB2;
		usb_role = USB_ROLE_DEVICE;
		pwr_role = TYPEC_SINK;
		break;
	case TUSB320_ATTACHED_STATE_ACC:
		/*
		 * Accessory detected. For debug accessories, just make some
		 * qualified guesses as to the role for lack of a better option.
		 */
		if (accessory == TUSB320_REG8_ACCESSORY_CONNECTED_AUDIO ||
		    accessory == TUSB320_REG8_ACCESSORY_CONNECTED_ACHRG) {
			typec_mode = TYPEC_MODE_AUDIO;
			usb_role = USB_ROLE_NONE;
			pwr_role = TYPEC_SINK;
			break;
		} else if (accessory ==
			   TUSB320_REG8_ACCESSORY_CONNECTED_DBGDFP) {
			typec_mode = TYPEC_MODE_DEBUG;
			pwr_role = TYPEC_SOURCE;
			usb_role = USB_ROLE_HOST;
			break;
		} else if (accessory ==
			   TUSB320_REG8_ACCESSORY_CONNECTED_DBGUFP) {
			typec_mode = TYPEC_MODE_DEBUG;
			pwr_role = TYPEC_SINK;
			usb_role = USB_ROLE_DEVICE;
			break;
		}

		dev_warn(priv->dev, "unexpected ACCESSORY_CONNECTED state %d\n",
			 accessory);

		fallthrough;
	default:
		typec_mode = TYPEC_MODE_USB2;
		usb_role = USB_ROLE_NONE;
		pwr_role = TYPEC_SINK;
		break;
	}

	typec_set_pwr_role(port, pwr_role);
	typec_set_mode(port, typec_mode);
	typec_set_role(port, usb_role);

	return 0;
}

static int tusb320_state_update_handler(struct tusb320_priv *priv,
					bool force_update)
{
	unsigned int reg;
	int ret;

	ret = regmap_read(priv->regmap, TUSB320_REG9, &reg);
	if (ret)
		return ret;

	if (!force_update && !(reg & TUSB320_REG9_INTERRUPT_STATUS))
		return 0;

	ret = tusb320_typec_irq_handler(priv, reg);

	regmap_write(priv->regmap, TUSB320_REG9, reg);

	return ret;
}

static irqreturn_t tusb320_irq_handler(struct typec_port *port)
{
	struct tusb320_priv *priv = typec_get_drvdata(port);

	return tusb320_state_update_handler(priv, false);
}

static const struct typec_operations tusb320_typec_ops = {
	.poll	= tusb320_irq_handler,
};

static const struct regmap_config tusb320_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int tusb320_typec_probe(struct i2c_client *client,
			       struct tusb320_priv *priv)
{
	struct device_node *connector;

	connector = of_get_child_by_name(client->dev.of_node, "connector");

	priv->cap.driver_data		= priv;
	priv->cap.ops			= &tusb320_typec_ops;
	priv->cap.of_node		= connector;

	priv->port = typec_register_port(&client->dev, &priv->cap);

	return PTR_ERR_OR_ZERO(priv->port);
}

static int tusb320_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tusb320_priv *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->dev = &client->dev;
	i2c_set_clientdata(client, priv);

	priv->regmap = regmap_init_i2c(client, &tusb320_regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	ret = tusb320_typec_probe(client, priv);
	if (ret)
		return ret;

	/* update initial state */
	tusb320_state_update_handler(priv, true);

	return ret;
}

static const struct of_device_id tusb320_typec_dt_match[] = {
	{ .compatible = "ti,tusb320" },
	{ .compatible = "ti,tusb320l" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, tusb320_typec_dt_match);

static struct driver tusb320_typec_driver = {
	.name		= "typec-tusb320",
	.of_match_table = tusb320_typec_dt_match,
	.probe		= tusb320_probe,
};
device_i2c_driver(tusb320_typec_driver);

MODULE_AUTHOR("Michael Auchter <michael.auchter@ni.com>");
MODULE_DESCRIPTION("TI TUSB320 Type-C driver");
MODULE_LICENSE("GPL v2");

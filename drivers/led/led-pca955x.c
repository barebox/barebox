/*
 * Copyright 2007-2008 Extreme Engineering Solutions, Inc.
 * Author: Nate Case <ncase@xes-inc.com>
 *
 * Copyright (C) 2018 WAGO Kontakttechnik GmbH & Co. KG <http://global.wago.com>
 * Author: Oleg Karfich <oleg.karfich@wago.com>
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
 *
 * This code was ported from linux-4.18 kernel driver.
 * Orginal code with it's copyright info can be found in
 * drivers/leds/leds-pca955x.c
 *
 * LED driver for various PCA955x I2C LED drivers
 *
 * Supported devices:
 *
 *	Device		Description		7-bit slave address
 *	------		-----------		-------------------
 *	PCA9550		2-bit driver		0x60 .. 0x61
 *	PCA9551		8-bit driver		0x60 .. 0x67
 *	PCA9552		16-bit driver		0x60 .. 0x67
 *	PCA9553/01	4-bit driver		0x62
 *	PCA9553/02	4-bit driver		0x63
 *
 * Philips PCA955x LED driver chips follow a register map as shown below:
 *
 *	Control Register		Description
 *	----------------		-----------
 *	0x0				Input register 0
 *					..
 *	NUM_INPUT_REGS - 1		Last Input register X
 *
 *	NUM_INPUT_REGS			Frequency prescaler 0
 *	NUM_INPUT_REGS + 1		PWM register 0
 *	NUM_INPUT_REGS + 2		Frequency prescaler 1
 *	NUM_INPUT_REGS + 3		PWM register 1
 *
 *	NUM_INPUT_REGS + 4		LED selector 0
 *	NUM_INPUT_REGS + 4
 *	    + NUM_LED_REGS - 1		Last LED selector
 *
 *  where NUM_INPUT_REGS and NUM_LED_REGS vary depending on how many
 *  bits the chip supports.
 *
  */

#include <common.h>
#include <init.h>
#include <led.h>
#include <malloc.h>
#include <i2c/i2c.h>
#include <of_device.h>

/* LED select registers determine the source that drives LED outputs */
#define PCA955X_LS_LED_ON	0x0	/* Output LOW */
#define PCA955X_LS_LED_OFF	0x1	/* Output HI-Z */
#define PCA955X_LS_BLINK0	0x2	/* Blink at PWM0 rate */
#define PCA955X_LS_BLINK1	0x3	/* Blink at PWM1 rate */

enum led_brightness {
	LED_OFF		= 0,
	LED_HALF	= 127,
	LED_FULL	= 255,
};

enum pca955x_type {
	pca9550,
	pca9551,
	pca9552,
	pca9553,
};

struct pca955x_chipdef {
	int	bits;
	u8	slv_addr;	/* 7-bit slave address mask */
	int	slv_addr_shift;	/* Number of bits to ignore */
};

static struct pca955x_chipdef pca955x_chipdefs[] = {
	[pca9550] = {
		.bits		= 2,
		.slv_addr	= /* 110000x */ 0x60,
		.slv_addr_shift	= 1,
	},
	[pca9551] = {
		.bits		= 8,
		.slv_addr	= /* 1100xxx */ 0x60,
		.slv_addr_shift	= 3,
	},
	[pca9552] = {
		.bits		= 16,
		.slv_addr	= /* 1100xxx */ 0x60,
		.slv_addr_shift	= 3,
	},
	[pca9553] = {
		.bits		= 4,
		.slv_addr	= /* 110001x */ 0x62,
		.slv_addr_shift	= 1,
	},
};

static const struct platform_device_id led_pca955x_id[] = {
	{ "pca9550", pca9550 },
	{ "pca9551", pca9551 },
	{ "pca9552", pca9552 },
	{ "pca9553", pca9553 },
	{ }
};

struct pca955x {
	struct pca955x_led *leds;
	struct pca955x_chipdef	*chipdef;
	struct i2c_client	*client;
};

struct pca955x_led {
	struct pca955x	*pca955x;
	struct led	led_cdev;
	int		led_num;	/* 0 .. 15 potentially */
	char		name[32];
};

struct pca955x_platform_data {
	struct pca955x_led	*leds;
	int			num_leds;
};

/* 8 bits per input register */
static inline int pca95xx_num_input_regs(int bits)
{
	return (bits + 7) / 8;
}

/*
 * Return an LED selector register value based on an existing one, with
 * the appropriate 2-bit state value set for the given LED number (0-3).
 */
static inline u8 pca955x_ledsel(u8 oldval, int led_num, int state)
{
	return (oldval & (~(0x3 << (led_num << 1)))) |
		((state & 0x3) << (led_num << 1));
}

/*
 * Write to frequency prescaler register, used to program the
 * period of the PWM output.  period = (PSCx + 1) / 38
 */
static int pca955x_write_psc(struct i2c_client *client, int n, u8 val)
{
	struct pca955x *pca955x = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client,
		pca95xx_num_input_regs(pca955x->chipdef->bits) + 2*n,
		val);
	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, val 0x%x, err %d\n",
			__func__, n, val, ret);
	return ret;
}

/*
 * Write to PWM register, which determines the duty cycle of the
 * output.  LED is OFF when the count is less than the value of this
 * register, and ON when it is greater.  If PWMx == 0, LED is always OFF.
 *
 * Duty cycle is (256 - PWMx) / 256
 */
static int pca955x_write_pwm(struct i2c_client *client, int n, u8 val)
{
	struct pca955x *pca955x = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client,
		pca95xx_num_input_regs(pca955x->chipdef->bits) + 1 + 2*n,
		val);
	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, val 0x%x, err %d\n",
			__func__, n, val, ret);
	return ret;
}

/*
 * Write to LED selector register, which determines the source that
 * drives the LED output.
 */
static int pca955x_write_ls(struct i2c_client *client, int n, u8 val)
{
	struct pca955x *pca955x = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client,
		pca95xx_num_input_regs(pca955x->chipdef->bits) + 4 + n,
		val);
	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, val 0x%x, err %d\n",
			__func__, n, val, ret);
	return ret;
}

/*
 * Read the LED selector register, which determines the source that
 * drives the LED output.
 */
static int pca955x_read_ls(struct i2c_client *client, int n, u8 *val)
{
	struct pca955x *pca955x = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_read_byte_data(client,
		pca95xx_num_input_regs(pca955x->chipdef->bits) + 4 + n);
	if (ret < 0) {
		dev_err(&client->dev, "%s: reg 0x%x, err %d\n",
			__func__, n, ret);
		return ret;
	}
	*val = (u8)ret;
	return 0;
}

static void pca955x_led_set(struct led *led_cdev, unsigned int value)
{
	struct pca955x_led *pca955x_led;
	struct pca955x *pca955x;
	u8 ls;
	int chip_ls;	/* which LSx to use (0-3 potentially) */
	int ls_led;	/* which set of bits within LSx to use (0-3) */
	int ret;

	pca955x_led = container_of(led_cdev, struct pca955x_led, led_cdev);
	pca955x = pca955x_led->pca955x;

	chip_ls = pca955x_led->led_num / 4;
	ls_led = pca955x_led->led_num % 4;

	ret = pca955x_read_ls(pca955x->client, chip_ls, &ls);
	if (ret)
		return;

	switch (value) {
	case LED_FULL:
		ls = pca955x_ledsel(ls, ls_led, PCA955X_LS_LED_ON);
		break;
	case LED_OFF:
		ls = pca955x_ledsel(ls, ls_led, PCA955X_LS_LED_OFF);
		break;
	case LED_HALF:
		ls = pca955x_ledsel(ls, ls_led, PCA955X_LS_BLINK0);
		break;
	default:
		/*
		 * Use PWM1 for all other values.  This has the unwanted
		 * side effect of making all LEDs on the chip share the
		 * same brightness level if set to a value other than
		 * OFF, HALF, or FULL.  But, this is probably better than
		 * just turning off for all other values.
		 */
		ret = pca955x_write_pwm(pca955x->client, 1, 255 - value);
		if (ret)
			return;
		ls = pca955x_ledsel(ls, ls_led, PCA955X_LS_BLINK1);
		break;
	}

	pca955x_write_ls(pca955x->client, chip_ls, ls);
}

static struct pca955x_platform_data *
led_pca955x_pdata_of_init(struct device_node *np, struct pca955x *pca955x)
{
	struct device_node *child;
	struct pca955x_chipdef *chip = pca955x->chipdef;
	struct pca955x_platform_data *pdata;
	int count, err;

	count = of_get_child_count(np);
	if (!count || count > chip->bits)
		return ERR_PTR(-ENODEV);

	pdata = xzalloc(sizeof(*pdata));
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->leds = xzalloc(chip->bits * sizeof(struct pca955x_led));
	if (!pdata->leds)
		return ERR_PTR(-ENOMEM);

	for_each_child_of_node(np, child) {
		struct pca955x_led *pca955x_led;
		const char *name;
		u32 reg;
		int res;

		res = of_property_read_u32(child, "reg", &reg);
		if ((res != 0) || (reg >= chip->bits))
			continue;

		pca955x_led = &pdata->leds[reg];
		pca955x_led->led_num = reg;
		pca955x_led->pca955x = pca955x;

		if (of_property_read_string(child, "label", &name))
			name = child->name;

		snprintf(pca955x_led->name, sizeof(pca955x_led->name),
								"%s", name);

		pca955x_led->led_cdev.name = pca955x_led->name;
		pca955x_led->led_cdev.set = pca955x_led_set;
		pca955x_led->led_cdev.num = pca955x_led->led_num;
		pca955x_led->led_cdev.max_value = 255;

		err = led_register(&pca955x_led->led_cdev);
		if (err)
			return ERR_PTR(err);

		/* Turn off LED */
		pca955x_led_set(&pca955x_led->led_cdev, LED_OFF);

		led_of_parse_trigger(&pca955x_led->led_cdev, child);
	}

	pdata->num_leds = count;

	return pdata;
}

static const struct of_device_id of_pca955x_match[] = {
	{ .compatible = "nxp,pca9550", .data = (void *)pca9550 },
	{ .compatible = "nxp,pca9551", .data = (void *)pca9551 },
	{ .compatible = "nxp,pca9552", .data = (void *)pca9552 },
	{ .compatible = "nxp,pca9553", .data = (void *)pca9553 },
	{},
};

static int led_pca955x_probe(struct device_d *dev)
{
	struct pca955x *pca955x;
	struct pca955x_led *pca955x_led;
	struct pca955x_chipdef *chip;
	struct i2c_client *client;
	int err;
	struct pca955x_platform_data *pdata;

	chip = &pca955x_chipdefs[dev->id_entry->driver_data];
	client = to_i2c_client(dev);

	/* Make sure the slave address / chip type combo given is possible */
	if ((client->addr & ~((1 << chip->slv_addr_shift) - 1)) !=
	    chip->slv_addr) {
		dev_err(dev, "invalid slave address %02x\n", client->addr);
		return -ENODEV;
	}

	dev_info(dev, "leds-pca955x: Using %s %d-bit LED driver at "
			"slave address 0x%02x\n",
			client->dev.name, chip->bits, client->addr);

	pca955x = xzalloc(sizeof(*pca955x));
	if (!pca955x)
		return -ENOMEM;

	pca955x->leds = xzalloc(chip->bits * sizeof(*pca955x_led));
	if (!pca955x->leds)
		return -ENOMEM;

	i2c_set_clientdata(client, pca955x);

	pca955x->client = client;
	pca955x->chipdef = chip;

	pdata =	led_pca955x_pdata_of_init(dev->device_node, pca955x);
	if (IS_ERR(pdata))
		return PTR_ERR(pdata);

	if (pdata->num_leds != chip->bits)
		dev_warn(dev, "board info claims %d LEDs on a %d-bit chip\n",
			pdata->num_leds, chip->bits);

	/* PWM0 is used for half brightness or 50% duty cycle */
	err = pca955x_write_pwm(client, 0, 255 - LED_HALF);
	if (err)
		return err;

	/* PWM1 is used for variable brightness, default to OFF */
	err = pca955x_write_pwm(client, 1, 0);
	if (err)
		return err;

	/* Set to fast frequency so we do not see flashing */
	err = pca955x_write_psc(client, 0, 0);
	if (err)
		return err;
	err = pca955x_write_psc(client, 1, 0);
	if (err)
		return err;

	return 0;
}

static struct driver_d led_pca955x_driver = {
	.name	= "led-pca955x",
	.probe	= led_pca955x_probe,
	.id_table = led_pca955x_id,
	.of_compatible = DRV_OF_COMPAT(of_pca955x_match),
};

static int __init led_pca955x_init(void)
{
	return i2c_driver_register(&led_pca955x_driver);
}
device_initcall(led_pca955x_init);

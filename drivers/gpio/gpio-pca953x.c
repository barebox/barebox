/*
 *  PCA953x 4/8/16/24/40 bit I/O ports
 *
 *  This code was ported from linux-3.15 kernel by Antony Pavlov.
 *
 *  Copyright (C) 2005 Ben Gardner <bgardner@wabtec.com>
 *  Copyright (C) 2007 Marvell International Ltd.
 *
 *  Derived from drivers/i2c/chips/pca9539.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <i2c/i2c.h>

#include <gpio.h>
#include <platform_data/pca953x.h>

#define PCA953X_INPUT		0
#define PCA953X_OUTPUT		1
#define PCA953X_INVERT		2
#define PCA953X_DIRECTION	3

#define REG_ADDR_AI		0x80

#define PCA957X_IN		0
#define PCA957X_INVRT		1
#define PCA957X_BKEN		2
#define PCA957X_PUPD		3
#define PCA957X_CFG		4
#define PCA957X_OUT		5
#define PCA957X_MSK		6
#define PCA957X_INTS		7

#define PCA_GPIO_MASK		0x00FF
#define PCA_INT			0x0100
#define PCA953X_TYPE		0x1000
#define PCA957X_TYPE		0x2000

static struct platform_device_id pca953x_id[] = {
	{ "pca9505", 40 | PCA953X_TYPE | PCA_INT, },
	{ "pca9534", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca9535", 16 | PCA953X_TYPE | PCA_INT, },
	{ "pca9536", 4  | PCA953X_TYPE, },
	{ "pca9537", 4  | PCA953X_TYPE | PCA_INT, },
	{ "pca9538", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca9539", 16 | PCA953X_TYPE | PCA_INT, },
	{ "pca9554", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca9555", 16 | PCA953X_TYPE | PCA_INT, },
	{ "pca9556", 8  | PCA953X_TYPE, },
	{ "pca9557", 8  | PCA953X_TYPE, },
	{ "pca9574", 8  | PCA957X_TYPE | PCA_INT, },
	{ "pca9575", 16 | PCA957X_TYPE | PCA_INT, },
	{ "pca9698", 40 | PCA953X_TYPE, },

	{ "max7310", 8  | PCA953X_TYPE, },
	{ "max7312", 16 | PCA953X_TYPE | PCA_INT, },
	{ "max7313", 16 | PCA953X_TYPE | PCA_INT, },
	{ "max7315", 8  | PCA953X_TYPE | PCA_INT, },
	{ "pca6107", 8  | PCA953X_TYPE | PCA_INT, },
	{ "tca6408", 8  | PCA953X_TYPE | PCA_INT, },
	{ "tca6416", 16 | PCA953X_TYPE | PCA_INT, },
	{ "tca6424", 24 | PCA953X_TYPE | PCA_INT, },
	{ "xra1202", 8  | PCA953X_TYPE },
	{ }
};

#define MAX_BANK 5
#define BANK_SZ 8

#define NBANK(chip) (chip->gpio_chip.ngpio / BANK_SZ)

struct pca953x_chip {
	unsigned gpio_start;
	u8 reg_output[MAX_BANK];
	u8 reg_direction[MAX_BANK];
	struct i2c_client *client;
	struct gpio_chip gpio_chip;
	const char *const *names;
	int	chip_type;
};

static inline struct pca953x_chip *to_pca(struct gpio_chip *gc)
{
	return container_of(gc, struct pca953x_chip, gpio_chip);
}

static int pca953x_read_single(struct pca953x_chip *chip, int reg, u32 *val,
				int off)
{
	int ret;
	int bank_shift = fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);
	int offset = off / BANK_SZ;

	ret = i2c_smbus_read_byte_data(chip->client,
				(reg << bank_shift) + offset);
	*val = ret;

	if (ret < 0) {
		dev_err(&chip->client->dev, "failed reading register\n");
		return ret;
	}

	return 0;
}

static int pca953x_write_single(struct pca953x_chip *chip, int reg, u32 val,
				int off)
{
	int ret = 0;
	int bank_shift = fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);
	int offset = off / BANK_SZ;

	ret = i2c_smbus_write_byte_data(chip->client,
					(reg << bank_shift) + offset, val);

	if (ret < 0) {
		dev_err(&chip->client->dev, "failed writing register\n");
		return ret;
	}

	return 0;
}

static int pca953x_write_regs(struct pca953x_chip *chip, int reg, u8 *val)
{
	int ret = 0;

	if (chip->gpio_chip.ngpio <= 8)
		ret = i2c_smbus_write_byte_data(chip->client, reg, *val);
	else if (chip->gpio_chip.ngpio >= 24) {
		int bank_shift = fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);
		ret = i2c_smbus_write_i2c_block_data(chip->client,
					(reg << bank_shift) | REG_ADDR_AI,
					NBANK(chip), val);
	} else {
		switch (chip->chip_type) {
		case PCA953X_TYPE:
			ret = i2c_smbus_write_word_data(chip->client,
							reg << 1, (u16) *val);
			break;
		case PCA957X_TYPE:
			ret = i2c_smbus_write_byte_data(chip->client, reg << 1,
							val[0]);
			if (ret < 0)
				break;
			ret = i2c_smbus_write_byte_data(chip->client,
							(reg << 1) + 1,
							val[1]);
			break;
		}
	}

	if (ret < 0) {
		dev_err(&chip->client->dev, "failed writing register\n");
		return ret;
	}

	return 0;
}

static int pca953x_read_regs(struct pca953x_chip *chip, int reg, u8 *val)
{
	int ret;

	if (chip->gpio_chip.ngpio <= 8) {
		ret = i2c_smbus_read_byte_data(chip->client, reg);
		*val = ret;
	} else if (chip->gpio_chip.ngpio >= 24) {
		int bank_shift = fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);

		ret = i2c_smbus_read_i2c_block_data(chip->client,
					(reg << bank_shift) | REG_ADDR_AI,
					NBANK(chip), val);
	} else {
		ret = i2c_smbus_read_word_data(chip->client, reg << 1);
		val[0] = (u16)ret & 0xFF;
		val[1] = (u16)ret >> 8;
	}
	if (ret < 0) {
		dev_err(&chip->client->dev, "failed reading register\n");
		return ret;
	}

	return 0;
}

static int pca953x_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct pca953x_chip *chip = to_pca(gc);
	u8 reg_val;
	int ret, offset = 0;

	reg_val = chip->reg_direction[off / BANK_SZ] | (1u << (off % BANK_SZ));

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_DIRECTION;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_CFG;
		break;
	}
	ret = pca953x_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_direction[off / BANK_SZ] = reg_val;
	ret = 0;
exit:
	return ret;
}

static int pca953x_gpio_direction_output(struct gpio_chip *gc,
		unsigned off, int val)
{
	struct pca953x_chip *chip = to_pca(gc);
	u8 reg_val;
	int ret, offset = 0;

	/* set output level */
	if (val)
		reg_val = chip->reg_output[off / BANK_SZ]
			| (1u << (off % BANK_SZ));
	else
		reg_val = chip->reg_output[off / BANK_SZ]
			& ~(1u << (off % BANK_SZ));

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_OUTPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_OUT;
		break;
	}
	ret = pca953x_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_output[off / BANK_SZ] = reg_val;

	/* then direction */
	reg_val = chip->reg_direction[off / BANK_SZ] & ~(1u << (off % BANK_SZ));
	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_DIRECTION;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_CFG;
		break;
	}
	ret = pca953x_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_direction[off / BANK_SZ] = reg_val;
	ret = 0;
exit:
	return ret;
}

static int pca953x_gpio_get_direction(struct gpio_chip *gc, unsigned off)
{
	struct pca953x_chip *chip = to_pca(gc);
	u8 reg_val;

	reg_val = chip->reg_direction[off / BANK_SZ] & (1u << (off % BANK_SZ));

	if (reg_val)
		return GPIOF_DIR_IN;

	return GPIOF_DIR_OUT;
}

static int pca953x_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct pca953x_chip *chip = to_pca(gc);
	u32 reg_val;
	int ret, offset = 0;

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_INPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_IN;
		break;
	}
	ret = pca953x_read_single(chip, offset, &reg_val, off);
	if (ret < 0) {
		/* NOTE:  diagnostic already emitted; that's all we should
		 * do unless gpio_*_value_cansleep() calls become different
		 * from their nonsleeping siblings (and report faults).
		 */
		return 0;
	}

	return (reg_val & (1u << (off % BANK_SZ))) ? 1 : 0;
}

static void pca953x_gpio_set_value(struct gpio_chip *gc, unsigned off, int val)
{
	struct pca953x_chip *chip = to_pca(gc);
	u8 reg_val;
	int ret, offset = 0;

	if (val)
		reg_val = chip->reg_output[off / BANK_SZ]
			| (1u << (off % BANK_SZ));
	else
		reg_val = chip->reg_output[off / BANK_SZ]
			& ~(1u << (off % BANK_SZ));

	switch (chip->chip_type) {
	case PCA953X_TYPE:
		offset = PCA953X_OUTPUT;
		break;
	case PCA957X_TYPE:
		offset = PCA957X_OUT;
		break;
	}
	ret = pca953x_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_output[off / BANK_SZ] = reg_val;
exit:
	return;
}

static struct gpio_ops pca953x_gpio_ops = {
	.direction_input  = pca953x_gpio_direction_input,
	.direction_output = pca953x_gpio_direction_output,
	.get_direction = pca953x_gpio_get_direction,
	.get = pca953x_gpio_get_value,
	.set = pca953x_gpio_set_value,
};

static void pca953x_setup_gpio(struct pca953x_chip *chip, int gpios)
{
	struct gpio_chip *gc;

	gc = &chip->gpio_chip;

	gc->ops = &pca953x_gpio_ops;

	gc->base = chip->gpio_start;
	gc->ngpio = gpios;
	gc->dev = &chip->client->dev;
}

static int device_pca953x_init(struct pca953x_chip *chip, u32 invert)
{
	int ret;
	u8 val[MAX_BANK];

	ret = pca953x_read_regs(chip, PCA953X_OUTPUT, chip->reg_output);
	if (ret)
		goto out;

	ret = pca953x_read_regs(chip, PCA953X_DIRECTION,
			       chip->reg_direction);
	if (ret)
		goto out;

	/* set platform specific polarity inversion */
	if (invert)
		memset(val, 0xFF, NBANK(chip));
	else
		memset(val, 0, NBANK(chip));

	ret = pca953x_write_regs(chip, PCA953X_INVERT, val);
out:
	return ret;
}

static int device_pca957x_init(struct pca953x_chip *chip, u32 invert)
{
	int ret;
	u8 val[MAX_BANK];

	ret = pca953x_read_regs(chip, PCA957X_OUT, chip->reg_output);
	if (ret)
		goto out;
	ret = pca953x_read_regs(chip, PCA957X_CFG, chip->reg_direction);
	if (ret)
		goto out;

	/* set platform specific polarity inversion */
	if (invert)
		memset(val, 0xFF, NBANK(chip));
	else
		memset(val, 0, NBANK(chip));
	pca953x_write_regs(chip, PCA957X_INVRT, val);

	/* To enable register 6, 7 to controll pull up and pull down */
	memset(val, 0x02, NBANK(chip));
	pca953x_write_regs(chip, PCA957X_BKEN, val);

	return 0;
out:
	return ret;
}

static int pca953x_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned long driver_data;
	struct pca953x_platform_data *pdata;
	struct pca953x_chip *chip;
	int ret;
	u32 invert = 0;

	chip = xzalloc(sizeof(struct pca953x_chip));

	driver_data = 0;
	pdata = dev->platform_data;
	if (pdata) {
		chip->gpio_start = pdata->gpio_base;
		invert = pdata->invert;
		chip->names = pdata->names;
	} else {
		int err;

		err = dev_get_drvdata(dev, (const void **)&driver_data);
		if (err)
			return err;

		chip->gpio_start = -1;
	}

	chip->client = client;

	chip->chip_type = driver_data & (PCA953X_TYPE | PCA957X_TYPE);

	/* initialize cached registers from their original values.
	 * we can't share this chip with another i2c master.
	 */
	pca953x_setup_gpio(chip, driver_data & PCA_GPIO_MASK);

	if (chip->chip_type == PCA953X_TYPE)
		ret = device_pca953x_init(chip, invert);
	else
		ret = device_pca957x_init(chip, invert);
	if (ret)
		return ret;

	ret = gpiochip_add(&chip->gpio_chip);
	if (ret)
		return ret;

	if (pdata && pdata->setup) {
		ret = pdata->setup(client, chip->gpio_chip.base,
				chip->gpio_chip.ngpio, pdata->context);
		if (ret < 0)
			dev_warn(&client->dev, "setup failed, %d\n", ret);
	}

	return 0;
}

/* convenience to stop overlong match-table lines */
#define OF_953X(__nrgpio, __int) (void *)(__nrgpio | PCA953X_TYPE | __int)
#define OF_957X(__nrgpio, __int) (void *)(__nrgpio | PCA957X_TYPE | __int)

static const struct of_device_id pca953x_dt_ids[] = {
	{ .compatible = "nxp,pca9505", .data = OF_953X(40, PCA_INT), },
	{ .compatible = "nxp,pca9534", .data = OF_953X( 8, PCA_INT), },
	{ .compatible = "nxp,pca9535", .data = OF_953X(16, PCA_INT), },
	{ .compatible = "nxp,pca9536", .data = OF_953X( 4, 0), },
	{ .compatible = "nxp,pca9537", .data = OF_953X( 4, PCA_INT), },
	{ .compatible = "nxp,pca9538", .data = OF_953X( 8, PCA_INT), },
	{ .compatible = "nxp,pca9539", .data = OF_953X(16, PCA_INT), },
	{ .compatible = "nxp,pca9554", .data = OF_953X( 8, PCA_INT), },
	{ .compatible = "nxp,pca9555", .data = OF_953X(16, PCA_INT), },
	{ .compatible = "nxp,pca9556", .data = OF_953X( 8, 0), },
	{ .compatible = "nxp,pca9557", .data = OF_953X( 8, 0), },
	{ .compatible = "nxp,pca9574", .data = OF_957X( 8, PCA_INT), },
	{ .compatible = "nxp,pca9575", .data = OF_957X(16, PCA_INT), },
	{ .compatible = "nxp,pca9698", .data = OF_953X(40, 0), },

	{ .compatible = "maxim,max7310", .data = OF_953X( 8, 0), },
	{ .compatible = "maxim,max7312", .data = OF_953X(16, PCA_INT), },
	{ .compatible = "maxim,max7313", .data = OF_953X(16, PCA_INT), },
	{ .compatible = "maxim,max7315", .data = OF_953X( 8, PCA_INT), },

	{ .compatible = "ti,pca6107", .data = OF_953X( 8, PCA_INT), },
	{ .compatible = "ti,tca6408", .data = OF_953X( 8, PCA_INT), },
	{ .compatible = "ti,tca6416", .data = OF_953X(16, PCA_INT), },
	{ .compatible = "ti,tca6424", .data = OF_953X(24, PCA_INT), },

	{ .compatible = "exar,xra1202", .data = OF_953X( 8, 0), },
	{ }
};

static struct driver_d pca953x_driver = {
	.name	= "pca953x",
	.probe		= pca953x_probe,
	.of_compatible	= DRV_OF_COMPAT(pca953x_dt_ids),
	.id_table	= pca953x_id,
};

static int __init pca953x_init(void)
{
	return i2c_driver_register(&pca953x_driver);
}
device_initcall(pca953x_init);

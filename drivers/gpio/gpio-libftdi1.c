// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2016,2017 Antony Pavlov <antonynpavlov@gmail.com>

/*
 * libftdi1 sandbox barebox GPIO driver
 */

#include <common.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <malloc.h>
#include <mach/linux.h>

struct libftdi1_gpio_chip {
	struct gpio_chip chip;
	struct ft2232_bitbang *ftbb;
};

static int libftdi1_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	struct libftdi1_gpio_chip *gpio =
		container_of(chip, struct libftdi1_gpio_chip, chip);

	barebox_libftdi1_gpio_direction(gpio->ftbb, off, GPIOF_DIR_IN);
	barebox_libftdi1_update(gpio->ftbb);

	return 0;
}

static int libftdi1_gpio_direction_output(
	struct gpio_chip *chip, unsigned off, int value)
{
	struct libftdi1_gpio_chip *gpio =
		container_of(chip, struct libftdi1_gpio_chip, chip);

	barebox_libftdi1_gpio_set_value(gpio->ftbb, off, value);
	barebox_libftdi1_gpio_direction(gpio->ftbb, off, GPIOF_DIR_OUT);
	barebox_libftdi1_update(gpio->ftbb);

	return 0;
}

static int libftdi1_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	struct libftdi1_gpio_chip *gpio =
		container_of(chip, struct libftdi1_gpio_chip, chip);

	return barebox_libftdi1_gpio_get_value(gpio->ftbb, off);
}

static void libftdi1_gpio_set_value(
	struct gpio_chip *chip, unsigned off, int value)
{
	struct libftdi1_gpio_chip *gpio =
		container_of(chip, struct libftdi1_gpio_chip, chip);

	barebox_libftdi1_gpio_set_value(gpio->ftbb, off, value);
	barebox_libftdi1_update(gpio->ftbb);
}

static struct gpio_ops libftdi1_gpio_ops = {
	.direction_input = libftdi1_gpio_direction_input,
	.direction_output = libftdi1_gpio_direction_output,
	.get = libftdi1_gpio_get_value,
	.set = libftdi1_gpio_set_value,
};

static int libftdi1_gpio_probe(struct device *dev)
{
	struct libftdi1_gpio_chip *gpio;
	struct ft2232_bitbang *ftbb;
	int ret;
	uint32_t id_vendor, id_product;
	const char *i_serial_number = NULL;

	of_property_read_u32(dev->of_node, "usb,id_vendor",
				&id_vendor);

	of_property_read_u32(dev->of_node, "usb,id_product",
				&id_product);

	of_property_read_string(dev->of_node, "usb,i_serial_number",
				&i_serial_number);

	ftbb = barebox_libftdi1_open(id_vendor, id_product,
				i_serial_number);
	if (!ftbb)
		return -EIO;

	gpio = xzalloc(sizeof(*gpio));

	gpio->ftbb = ftbb;

	gpio->chip.dev = dev;
	gpio->chip.ops = &libftdi1_gpio_ops;
	gpio->chip.base = 0;
	gpio->chip.ngpio = 8;

	ret = gpiochip_add(&gpio->chip);

	dev_dbg(dev, "%d: probed gpio%d with base %d\n",
			ret, dev->id, gpio->chip.base);

	return 0;
}

static __maybe_unused const struct of_device_id libftdi1_gpio_dt_ids[] = {
	{
		.compatible = "barebox,libftdi1-gpio",
	}, {
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, libftdi1_gpio_dt_ids);

static void libftdi1_gpio_remove(struct device *dev)
{
	barebox_libftdi1_close();
}

static struct driver libftdi1_gpio_driver = {
	.name = "libftdi1-gpio",
	.probe = libftdi1_gpio_probe,
	.remove = libftdi1_gpio_remove,
	.of_compatible = DRV_OF_COMPAT(libftdi1_gpio_dt_ids),
};
device_platform_driver(libftdi1_gpio_driver);

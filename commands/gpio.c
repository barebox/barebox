// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <command.h>
#include <errno.h>
#include <gpio.h>
#include <getopt.h>

static int get_gpio_and_value(int argc, char *argv[],
			      int *gpio, int *value)
{
	struct gpio_chip *chip = NULL;
	struct device_d *dev;
	int count = 2;
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "d:")) > 0) {
		switch (opt) {
		case 'd':
			dev = find_device(optarg);
			if (!dev)
				return -ENODEV;

			chip = gpio_get_chip_by_dev(dev);
			if (!chip)
				return -EINVAL;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (value)
		count++;

	if (optind < count)
		return COMMAND_ERROR_USAGE;

	*gpio = gpio_find_by_name(argv[optind]);
	if (*gpio < 0)
		*gpio = gpio_find_by_label(argv[optind]);
	if (*gpio < 0) {
		ret = kstrtoint(argv[optind], 0, gpio);
		if (ret < 0)
			return ret;

		if (chip)
			*gpio += chip->base;
	} else if (chip) {
		if (gpio_get_chip(*gpio) != chip) {
			printf("%s: not exporting pin %u\n", dev_name(chip->dev), *gpio);
			return -EINVAL;
		}
	}

	if (value)
		ret = kstrtoint(argv[optind + 1], 0, value);

	return ret;
}

static int do_gpio_get_value(int argc, char *argv[])
{
	int gpio, value, ret;

	ret = get_gpio_and_value(argc, argv, &gpio, NULL);
	if (ret)
		return ret;

	value = gpio_get_value(gpio);
	if (value < 0)
		return 1;

	return value;
}

BAREBOX_CMD_START(gpio_get_value)
	.cmd		= do_gpio_get_value,
	BAREBOX_CMD_DESC("return value of a GPIO pin")
	BAREBOX_CMD_OPTS("[-d CONTROLLER] GPIO")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

static int do_gpio_set_value(int argc, char *argv[])
{
	int gpio, value, ret;

	ret = get_gpio_and_value(argc, argv, &gpio, &value);
	if (ret)
		return ret;

	gpio_set_value(gpio, value);

	return 0;
}

BAREBOX_CMD_START(gpio_set_value)
	.cmd		= do_gpio_set_value,
	BAREBOX_CMD_DESC("set a GPIO's output value")
	BAREBOX_CMD_OPTS("[-d CONTROLLER] GPIO VALUE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

static int do_gpio_direction_input(int argc, char *argv[])
{
	int gpio, ret;

	ret = get_gpio_and_value(argc, argv, &gpio, NULL);
	if (ret)
		return ret;

	ret = gpio_direction_input(gpio);
	if (ret)
		return 1;

	return 0;
}

BAREBOX_CMD_START(gpio_direction_input)
	.cmd		= do_gpio_direction_input,
	BAREBOX_CMD_DESC("set direction of a GPIO pin to input")
	BAREBOX_CMD_OPTS("[-d CONTROLLER] GPIO")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

static int do_gpio_direction_output(int argc, char *argv[])
{
	int gpio, value, ret;

	ret = get_gpio_and_value(argc, argv, &gpio, &value);
	if (ret)
		return ret;

	ret = gpio_direction_output(gpio, value);
	if (ret)
		return 1;

	return 0;
}

BAREBOX_CMD_START(gpio_direction_output)
	.cmd		= do_gpio_direction_output,
	BAREBOX_CMD_DESC("set direction of a GPIO pin to output")
	BAREBOX_CMD_OPTS("[-d CONTROLLER] GPIO VALUE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

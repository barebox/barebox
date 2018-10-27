/*
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
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <gpio.h>

static int get_gpio_and_value(int argc, char *argv[],
			      int *gpio, int *value)
{
	const int count = value ? 3 : 2;
	int ret = 0;

	if (argc < count)
		return COMMAND_ERROR_USAGE;

	*gpio = gpio_find_by_label(argv[1]);
	if (*gpio < 0) {
		ret = kstrtoint(argv[1], 0, gpio);
		if (ret < 0)
			return ret;
	}

	if (value)
		ret = kstrtoint(argv[2], 0, value);

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
	BAREBOX_CMD_OPTS("GPIO")
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
	BAREBOX_CMD_OPTS("GPIO VALUE")
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
	BAREBOX_CMD_OPTS("GPIO")
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
	BAREBOX_CMD_OPTS("GPIO VALUE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

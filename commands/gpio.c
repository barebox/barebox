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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <gpio.h>

static int do_gpio_get_value(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int gpio, value;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	gpio = simple_strtoul(argv[1], NULL, 0);

	value = gpio_get_value(gpio);
	if (value < 0)
		return 1;

	return value;
}

static const __maybe_unused char cmd_gpio_get_value_help[] =
"Usage: gpio_set_value <gpio>\n";

BAREBOX_CMD_START(gpio_get_value)
	.cmd		= do_gpio_get_value,
	.usage		= "return a gpio's value",
	BAREBOX_CMD_HELP(cmd_gpio_get_value_help)
BAREBOX_CMD_END

static int do_gpio_set_value(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int gpio, value;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	gpio = simple_strtoul(argv[1], NULL, 0);
	value = simple_strtoul(argv[2], NULL, 0);

	gpio_set_value(gpio, value);

	return 0;
}

static const __maybe_unused char cmd_gpio_set_value_help[] =
"Usage: gpio_set_value <gpio> <value>\n";

BAREBOX_CMD_START(gpio_set_value)
	.cmd		= do_gpio_set_value,
	.usage		= "set a gpio's output value",
	BAREBOX_CMD_HELP(cmd_gpio_set_value_help)
BAREBOX_CMD_END

static int do_gpio_direction_input(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int gpio, ret;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	gpio = simple_strtoul(argv[1], NULL, 0);

	ret = gpio_direction_input(gpio);
	if (ret)
		return 1;

	return 0;
}

static const __maybe_unused char cmd_do_gpio_direction_input_help[] =
"Usage: gpio_direction_input <gpio>\n";

BAREBOX_CMD_START(gpio_direction_input)
	.cmd		= do_gpio_direction_input,
	.usage		= "set a gpio as output",
	BAREBOX_CMD_HELP(cmd_do_gpio_direction_input_help)
BAREBOX_CMD_END

static int do_gpio_direction_output(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int gpio, value, ret;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	gpio = simple_strtoul(argv[1], NULL, 0);
	value = simple_strtoul(argv[2], NULL, 0);

	ret = gpio_direction_output(gpio, value);
	if (ret)
		return 1;

	return 0;
}

static const __maybe_unused char cmd_gpio_direction_output_help[] =
"Usage: gpio_direction_output <gpio> <value>\n";

BAREBOX_CMD_START(gpio_direction_output)
	.cmd		= do_gpio_direction_output,
	.usage		= "set a gpio as output",
	BAREBOX_CMD_HELP(cmd_gpio_direction_output_help)
BAREBOX_CMD_END


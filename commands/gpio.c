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

static int do_gpio_get_value(struct command *cmdtp, int argc, char *argv[])
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
"Usage: gpio_get_value <gpio>\n";

BAREBOX_CMD_START(gpio_get_value)
	.cmd		= do_gpio_get_value,
	.usage		= "return a gpio's value",
	BAREBOX_CMD_HELP(cmd_gpio_get_value_help)
BAREBOX_CMD_END

static int do_gpio_set_value(struct command *cmdtp, int argc, char *argv[])
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

static int do_gpio_direction_input(struct command *cmdtp, int argc, char *argv[])
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

static int do_gpio_direction_output(struct command *cmdtp, int argc, char *argv[])
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

/**
@page gpio_for_users Runtime GPIO handling

@section regular_gpio General usage information

These commands are available if the symbol @b CONFIG_GENERIC_GPIO and
@b CONFIG_CMD_GPIO are enabled in the Kconfig.

@note All gpio related commands take a number to identify the pad. This
number is architecture dependent. There may be no intuitional correlation
between available pads and the GPIO numbers to be used in the commands. Due
to this it's also possible the numbers change between @b barebox releases.

@section gpio_dir_out Switch a pad into an output GPIO
@verbatim
gpio_direction_output <gpio_no> <initial_value>
@endverbatim
- @b gpio_no Architecture dependent GPIO number
- @b initial_value Output value the pad should emit

@note To avoid glitches on the pad's line, the routines will first setting up
the pad's value and after that switching the pad itself to output (if the
silicon is able to do so)

@note If the pad is already configured into a non GPIO mode (if available)
this command may fail (silently)

@section gpio_dir_in Switch a pad into an input GPIO
@verbatim
gpio_direction_input <gpio_no>
@endverbatim
- @b gpio_no Architecture dependent GPIO number

@note If the pad is already configured into a non GPIO mode (if available)
this command may fail (silently)

@section gpio_get_value Read in the value of an GPIO input pad
@verbatim
gpio_get_value <gpio_no>
@endverbatim

Reads in the current pad's line value from the given GPIO number. It returns
the value as a shell return code. There is no visible output at stdout. You
can check the return value by using "echo $?"

@note If the return code is not '0' or '1' it's meant as an error code.

@note If the pad is not configured for GPIO mode this command may fail
(silently) and returns garbage

@section gpio_set_value Set a new value to a GPIO output pad
@verbatim
gpio_set_value <gpio_no> <value>
@endverbatim
- @b gpio_no Architecture dependent GPIO number
- @b value Output value the pad should emit

Sets a new output @b value to the pad with GPIO number @b gpio_no

@note If the pad is not configured for GPIO mode this command may fail (silently)
*/

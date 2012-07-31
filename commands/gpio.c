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

static int do_gpio_get_value(int argc, char *argv[])
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

BAREBOX_CMD_HELP_START(gpio_get_value)
BAREBOX_CMD_HELP_USAGE("gpio_get_value <gpio>\n")
BAREBOX_CMD_HELP_SHORT("get the value of an gpio input pin\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(gpio_get_value)
	.cmd		= do_gpio_get_value,
	.usage		= "return value of a gpio pin",
	BAREBOX_CMD_HELP(cmd_gpio_get_value_help)
BAREBOX_CMD_END

static int do_gpio_set_value(int argc, char *argv[])
{
	int gpio, value;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	gpio = simple_strtoul(argv[1], NULL, 0);
	value = simple_strtoul(argv[2], NULL, 0);

	gpio_set_value(gpio, value);

	return 0;
}

BAREBOX_CMD_HELP_START(gpio_set_value)
BAREBOX_CMD_HELP_USAGE("gpio_set_value <gpio> <value>\n")
BAREBOX_CMD_HELP_SHORT("set the value of an gpio output pin\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(gpio_set_value)
	.cmd		= do_gpio_set_value,
	.usage		= "set a gpio's output value",
	BAREBOX_CMD_HELP(cmd_gpio_set_value_help)
BAREBOX_CMD_END

static int do_gpio_direction_input(int argc, char *argv[])
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

BAREBOX_CMD_HELP_START(gpio_direction_input)
BAREBOX_CMD_HELP_USAGE("gpio_direction_input <gpio>\n")
BAREBOX_CMD_HELP_SHORT("set direction of a gpio pin to input\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(gpio_direction_input)
	.cmd		= do_gpio_direction_input,
	.usage		= "set direction of a gpio pin to input",
	BAREBOX_CMD_HELP(cmd_gpio_direction_input_help)
BAREBOX_CMD_END

static int do_gpio_direction_output(int argc, char *argv[])
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

BAREBOX_CMD_HELP_START(gpio_direction_output)
BAREBOX_CMD_HELP_USAGE("gpio_direction_output <gpio> <value>\n")
BAREBOX_CMD_HELP_SHORT("set direction of a gpio pin to output\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(gpio_direction_output)
	.cmd		= do_gpio_direction_output,
	.usage		= "set direction of a gpio pin to output",
	BAREBOX_CMD_HELP(cmd_gpio_direction_output_help)
BAREBOX_CMD_END

/**
 * @page gpio_for_users GPIO Handling

@section regular_gpio General usage information

These commands are available if the symbol @b CONFIG_GENERIC_GPIO and @b
CONFIG_CMD_GPIO are enabled in Kconfig.

@note All gpio related commands take a number to identify the pad. This
number is architecture dependent and may not directly correlate with the
pad numbers. Due to this, it is also possible that the numbers changes
between @b barebox releases.

@section gpio_dir_out Use Pad as GPIO Output
@verbatim
# gpio_direction_output <gpio_no> <initial_value>
@endverbatim
- gpio_no: Architecture dependend GPIO number
- initial_value: Output value

<p> To avoid glitches on the pad the routines will first set up the
pad's value and afterwards switch the pad to output (if the silicon is
able to do so). If the pad is already configured in non-GPIO mode (if
available), this command may silently fail. </p>

@section gpio_dir_in Use Pad as GPIO Input
@verbatim
# gpio_direction_input <gpio_no>
@endverbatim
- gpio_no: Architecture dependent GPIO number

<p> If the pad is already configured in non-GPIO mode (if available),
this command may silently fail. </p>

@section gpio_get_value Read Input Value from GPIO Pin
@verbatim
# gpio_get_value <gpio_no>
@endverbatim

<p> Reads the current value of a GPIO pin and return the value as a
shell return code. There is no visible output on stdout. You can check
the return value by using "echo $?". </p>

<p> A return code other than '0' or '1' specifies an error code. </p>

<p> If the pad is not configured in GPIO mode, this command may silently
fail and return garbage. </p>

@section gpio_set_value Set Output Value on GPIO Pin
@verbatim
# gpio_set_value <gpio_no> <value>
@endverbatim
- gpio_no: Architecture dependent GPIO number
- value: Output value

<p> Set a new output value on pad with GPIO number \<gpio_no>. </p>

<p> If the pad is not configured in GPIO-mode, this command may silently
fail. </p>

*/

/*
 * based on code:
 *
 * (C) Copyright 2000 Paolo Scaffardi, AIRVENT SAM s.p.a -
 * RIMINI(ITALY), arsenio@tin.it
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */
#include <common.h>
#include <fs.h>
#include <errno.h>
#include <console.h>
#include <init.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <password.h>

#ifndef CONFIG_CONSOLE_NONE

static int console_input_allow;

static int console_global_init(void)
{
	if (IS_ENABLED(CONFIG_CMD_LOGIN) && is_passwd_enable())
		console_input_allow = 0;
	else
		console_input_allow = 1;

	globalvar_add_simple_bool("console.input_allow", &console_input_allow);

	return 0;
}
late_initcall(console_global_init);

BAREBOX_MAGICVAR_NAMED(global_console_input_allow, global.console.input_allow, "console input allowed");

bool console_is_input_allow(void)
{
	return console_input_allow;
}

void console_allow_input(bool val)
{
	console_input_allow = val;
}

int barebox_loglevel = CONFIG_DEFAULT_LOGLEVEL;

int pr_print(int level, const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CFG_PBSIZE];

	if (level > barebox_loglevel)
		return 0;

	va_start(args, fmt);
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	puts(printbuffer);

	return i;
}

static int loglevel_init(void)
{
	return globalvar_add_simple_int("loglevel", &barebox_loglevel, "%d");
}
device_initcall(loglevel_init);

int printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CFG_PBSIZE];

	va_start(args, fmt);

	/*
	 * For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	puts(printbuffer);

	return i;
}
EXPORT_SYMBOL(printf);

int vprintf(const char *fmt, va_list args)
{
	uint i;
	char printbuffer[CFG_PBSIZE];

	/*
	 * For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);

	/* Print the string */
	puts(printbuffer);

	return i;
}
EXPORT_SYMBOL(vprintf);

#endif /* !CONFIG_CONSOLE_NONE */

int fprintf(int file, const char *fmt, ...)
{
	va_list args;
	char printbuffer[CFG_PBSIZE];

	va_start(args, fmt);

	/*
	 * For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	return fputs(file, printbuffer);
}
EXPORT_SYMBOL(fprintf);

int fputs(int fd, const char *s)
{
	if (fd == 1)
		return puts(s);
	else if (fd == 2)
		return eputs(s);
	else
		return write(fd, s, strlen(s));
}
EXPORT_SYMBOL(fputs);

int fputc(int fd, char c)
{
	if (fd == 1)
		putchar(c);
	else if (fd == 2)
		eputc(c);
	else
		return write(fd, &c, 1);

	return 0;
}
EXPORT_SYMBOL(fputc);

struct console_device *console_get_by_dev(struct device_d *dev)
{
	struct console_device *cdev;

	for_each_console(cdev) {
		if (cdev->dev == dev)
			return cdev;
	}

	return NULL;
}
EXPORT_SYMBOL(console_get_by_dev);

/*
 * @brief returns current used console device
 *
 * @return console device which is registered with CONSOLE_STDIN and
 * CONSOLE_STDOUT
 */
struct console_device *console_get_first_active(void)
{
	struct console_device *cdev;
	/*
	 * Assumption to have BOTH CONSOLE_STDIN AND STDOUT in the
	 * same output console
	 */
	for_each_console(cdev) {
		if ((cdev->f_active & (CONSOLE_STDIN | CONSOLE_STDOUT)))
			return cdev;
	}

	return NULL;
}
EXPORT_SYMBOL(console_get_first_active);

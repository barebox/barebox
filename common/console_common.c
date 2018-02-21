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
#include <clock.h>
#include <malloc.h>
#include <asm-generic/div64.h>

#ifndef CONFIG_CONSOLE_NONE

static const char *colored_log_level[] = {
	[MSG_EMERG] = "\033[31mEMERG:\033[0m ",		/* red */
	[MSG_ALERT] = "\033[31mALERT:\033[0m ",		/* red */
	[MSG_CRIT] = "\033[31mCRITICAL:\033[0m ",	/* red */
	[MSG_ERR] = "\033[31mERROR:\033[0m ",		/* red */
	[MSG_WARNING] = "\033[33mWARNING:\033[0m ",	/* yellow */
	[MSG_NOTICE] = "\033[34mNOTICE:\033[0m ",	/* blue */
};

int barebox_loglevel = CONFIG_DEFAULT_LOGLEVEL;

LIST_HEAD(barebox_logbuf);
static int barebox_logbuf_num_messages;
static int barebox_log_max_messages = 1000;

static void log_del(struct log_entry *log)
{
	free(log->msg);
	list_del(&log->list);
	free(log);
	barebox_logbuf_num_messages--;
}

/**
 * log_clean - delete log messages from buffer
 *
 * @limit:	The maximum messages left in the buffer after
 *		calling this function.
 *
 * This function deletes all messages in the logbuf exeeding
 * the limit.
 */
void log_clean(unsigned int limit)
{
	struct log_entry *log, *tmp;

	if (list_empty(&barebox_logbuf))
		return;

	list_for_each_entry_safe(log, tmp, &barebox_logbuf, list) {
		if (barebox_logbuf_num_messages <= limit)
			break;
		log_del(log);
	}
}

static void pr_puts(int level, const char *str)
{
	struct log_entry *log;

	if (IS_ENABLED(CONFIG_LOGBUF) && mem_malloc_is_initialized()) {
		if (barebox_log_max_messages > 0)
			log_clean(barebox_log_max_messages - 1);

		if (barebox_log_max_messages >= 0) {
			log = malloc(sizeof(*log));
			if (!log)
				goto nolog;

			log->msg = strdup(str);
			if (!log->msg) {
				free(log);
				goto nolog;
			}

			log->timestamp = get_time_ns();
			log->level = level;
			list_add_tail(&log->list, &barebox_logbuf);
			barebox_logbuf_num_messages++;
		}
	}
nolog:
	if (level > barebox_loglevel)
		return;

	puts(str);
}

static void print_colored_log_level(const int level)
{
	if (!console_allow_color())
		return;
	if (level >= ARRAY_SIZE(colored_log_level))
		return;
	if (!colored_log_level[level])
		return;

	pr_puts(level, colored_log_level[level]);
}

int pr_print(int level, const char *fmt, ...)
{
	va_list args;
	int i;
	char printbuffer[CFG_PBSIZE];

	if (!IS_ENABLED(CONFIG_LOGBUF) && level > barebox_loglevel)
		return 0;

	print_colored_log_level(level);

	va_start(args, fmt);
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	pr_puts(level, printbuffer);

	return i;
}

int dev_printf(int level, const struct device_d *dev, const char *format, ...)
{
	va_list args;
	int ret = 0;
	char printbuffer[CFG_PBSIZE];

	if (!IS_ENABLED(CONFIG_LOGBUF) && level > barebox_loglevel)
		return 0;

	print_colored_log_level(level);

	if (dev->driver && dev->driver->name)
		ret += sprintf(printbuffer, "%s ", dev->driver->name);

	ret += sprintf(printbuffer + ret, "%s: ", dev_name(dev));

	va_start(args, format);

	ret += vsprintf(printbuffer + ret, format, args);

	va_end(args);

	pr_puts(level, printbuffer);

	return ret;
}

#ifdef CONFIG_CONSOLE_ALLOW_COLOR
static unsigned int __console_allow_color = 1;
#else
static unsigned int __console_allow_color = 0;
#endif

bool console_allow_color(void)
{
	return __console_allow_color;
}

static int console_common_init(void)
{
	if (IS_ENABLED(CONFIG_LOGBUF))
		globalvar_add_simple_int("log_max_messages",
				&barebox_log_max_messages, "%d");

	globalvar_add_simple_bool("allow_color", &__console_allow_color);

	return globalvar_add_simple_int("loglevel", &barebox_loglevel, "%d");
}
device_initcall(console_common_init);

void log_print(unsigned flags)
{
	struct log_entry *log;
	unsigned long last = 0;

	list_for_each_entry(log, &barebox_logbuf, list) {
		uint64_t diff = log->timestamp - time_beginning;
		unsigned long difful;

		do_div(diff, 1000);
		difful = diff;

		if (!log->timestamp)
			difful = 0;

		if (flags & (BAREBOX_LOG_PRINT_TIME | BAREBOX_LOG_DIFF_TIME))
			printf("[");

		if (flags & BAREBOX_LOG_PRINT_TIME)
			printf("%10luus", difful);

		if (flags & BAREBOX_LOG_DIFF_TIME) {
			printf(" < %10luus", difful - last);
			last = difful;
		}

		if (flags & (BAREBOX_LOG_PRINT_TIME | BAREBOX_LOG_DIFF_TIME))
			printf("] ");

		printf("%s", log->msg);
	}
}

int printf(const char *fmt, ...)
{
	va_list args;
	int i;
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
	int i;
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

struct console_device *console_get_by_name(const char *name)
{
	struct console_device *cdev;

	for_each_console(cdev) {
		if (cdev->devname && !strcmp(cdev->devname, name))
			return cdev;
	}

	return NULL;
}
EXPORT_SYMBOL(console_get_by_name);

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

#endif /* !CONFIG_CONSOLE_NONE */

int dprintf(int file, const char *fmt, ...)
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
	return dputs(file, printbuffer);
}
EXPORT_SYMBOL(dprintf);

int dputs(int fd, const char *s)
{
	if (fd == 1)
		return puts(s);
	else if (fd == 2)
		return console_puts(CONSOLE_STDERR, s);
	else
		return write(fd, s, strlen(s));
}
EXPORT_SYMBOL(dputs);

int dputc(int fd, char c)
{
	if (fd == 1)
		putchar(c);
	else if (fd == 2)
		console_putc(CONSOLE_STDERR, c);
	else
		return write(fd, &c, 1);

	return 0;
}
EXPORT_SYMBOL(dputc);

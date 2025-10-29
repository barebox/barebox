// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * based on code:
 *
 * (C) Copyright 2000 Paolo Scaffardi, AIRVENT SAM s.p.a -
 * RIMINI(ITALY), arsenio@tin.it
 */
#include "stdio.h"
#include <common.h>
#include <fs.h>
#include <errno.h>
#include <console.h>
#include <init.h>
#include <string.h>
#include <environment.h>
#include <globalvar.h>
#include <magicvar.h>
#include <memory.h>
#include <of.h>
#include <password.h>
#include <clock.h>
#include <malloc.h>
#include <linux/pstore.h>
#include <linux/math64.h>
#include <linux/sizes.h>
#include <linux/overflow.h>
#include <security/config.h>

#ifndef CONFIG_CONSOLE_NONE

static const char *colored_log_level[] = {
	[MSG_EMERG] = "\033[1;31mEMERG:\033[0m ",	/* red */
	[MSG_ALERT] = "\033[1;31mALERT:\033[0m ",	/* red */
	[MSG_CRIT] = "\033[1;31mCRITICAL:\033[0m ",	/* red */
	[MSG_ERR] = "\033[1;31mERROR:\033[0m ",		/* red */
	[MSG_WARNING] = "\033[1;33mWARNING:\033[0m ",	/* yellow */
	[MSG_NOTICE] = "\033[1;34mNOTICE:\033[0m ",	/* blue */
};

int barebox_loglevel = CONFIG_DEFAULT_LOGLEVEL;

LIST_HEAD(barebox_logbuf);
static int barebox_logbuf_num_messages;
static int barebox_log_max_messages;

static void log_del(struct log_entry *log)
{
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
 * This function deletes all messages in the logbuf exceeding
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

static void print_colored_log_level(unsigned int ch, const int level)
{
	if (!console_allow_color())
		return;
	if (level >= ARRAY_SIZE(colored_log_level))
		return;
	if (!colored_log_level[level])
		return;

	console_puts(ch, colored_log_level[level]);
}

static void pr_puts(int level, const char *str)
{
	struct log_entry *log;

	if (IS_ENABLED(CONFIG_LOGBUF) && mem_malloc_is_initialized()) {
		if (barebox_log_max_messages > 0)
			log_clean(barebox_log_max_messages - 1);

		if (barebox_log_max_messages >= 0) {
			int msglen;

			msglen = strlen(str);
			log = malloc(struct_size(log, msg, msglen + 1));
			if (!log)
				goto nolog;

			memcpy(log->msg, str, msglen + 1);

			log->timestamp = get_time_ns();
			log->level = level;
			list_add_tail(&log->list, &barebox_logbuf);
			barebox_logbuf_num_messages++;
		}
	}

	pstore_log(str);
nolog:

	if (level > barebox_loglevel)
		return;

	print_colored_log_level(CONSOLE_STDERR, level);
	console_puts(CONSOLE_STDERR, str);
}

int pr_print(int level, const char *fmt, ...)
{
	va_list args;
	int i;
	char printbuffer[CFG_PBSIZE];

	if (!IS_ENABLED(CONFIG_LOGBUF) && level > barebox_loglevel)
		return 0;

	va_start(args, fmt);
	i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
	va_end(args);

	pr_puts(level, printbuffer);

	return i;
}
EXPORT_SYMBOL(pr_print);

int dev_printf(int level, const struct device *dev, const char *format, ...)
{
	va_list args;
	int ret = 0;
	char printbuffer[CFG_PBSIZE];
	size_t size = sizeof(printbuffer);

	if (!IS_ENABLED(CONFIG_LOGBUF) && level > barebox_loglevel)
		return 0;

	if (dev && dev->driver && dev->driver->name)
		ret += snprintf(printbuffer, size, "%s ", dev->driver->name);

	ret += snprintf(printbuffer + ret, size - ret, "%s: ", dev_name(dev));

	va_start(args, format);

	ret += vsnprintf(printbuffer + ret, size - ret, format, args);

	va_end(args);

	pr_puts(level, printbuffer);

	return ret;
}
EXPORT_SYMBOL(dev_printf);

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
	if (IS_ENABLED(CONFIG_LOGBUF)) {
		barebox_log_max_messages
			= clamp(mem_malloc_size() / SZ_32K, 1000UL, 100000UL);
		globalvar_add_simple_int("log_max_messages",
				&barebox_log_max_messages, "%d");
	}

	globalvar_add_simple_bool("allow_color", &__console_allow_color);

	return globalvar_add_simple_int("loglevel", &barebox_loglevel, "%d");
}
core_initcall(console_common_init);

int log_writefile(const char *filepath)
{
	int ret = 0, nbytes = 0, fd = -1;
	struct log_entry *log;

	fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
		return -errno;

	list_for_each_entry(log, &barebox_logbuf, list) {
		ret = dputs(fd, log->msg);
		if (ret < 0)
			break;
		nbytes += ret;
	}

	close(fd);
	return ret < 0 ? ret : nbytes;
}

/**
 * log_print - print the log buffer
 *
 * @flags:	Flags selecting output formatting
 * @levels:	bitmask of loglevels to print, 0 for all
 *
 * This function prints the messages of the selected levels; optionally with
 * additional information and formatting.
 */
int log_print(unsigned flags, unsigned levels)
{
	struct log_entry *log;
	unsigned long last = 0;

	list_for_each_entry(log, &barebox_logbuf, list) {
		uint64_t time_ns = log->timestamp;
		unsigned long time;

		if (levels && !(levels & (1 << log->level)))
			continue;
		if (ctrlc_non_interruptible())
			return -EINTR;

		if (!(flags & (BAREBOX_LOG_PRINT_RAW | BAREBOX_LOG_PRINT_TIME
			       | BAREBOX_LOG_DIFF_TIME)))
			print_colored_log_level(CONSOLE_STDOUT, log->level);

		if (flags & BAREBOX_LOG_PRINT_RAW)
			printf("<%i>", log->level);

		/* convert ns to us */
		do_div(time_ns, 1000);
		time = time_ns;

		if (flags & (BAREBOX_LOG_PRINT_TIME | BAREBOX_LOG_DIFF_TIME))
			printf("[");

		if (flags & BAREBOX_LOG_PRINT_TIME)
			printf("%10luus", time);

		if (flags & BAREBOX_LOG_DIFF_TIME) {
			printf(" < %10luus", time - last);
			last = time;
		}

		if (flags & (BAREBOX_LOG_PRINT_TIME | BAREBOX_LOG_DIFF_TIME))
			printf("] ");

		printf("%s", log->msg);
	}

	return 0;
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
	i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
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
	i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);

	/* Print the string */
	puts(printbuffer);

	return i;
}
EXPORT_SYMBOL(vprintf);

struct console_device *console_get_by_dev(struct device *dev)
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
struct console_device *console_get_first_interactive(void)
{
	struct console_device *cdev;
	const unsigned char active = CONSOLE_STDIN | CONSOLE_STDOUT;

	/* if no console input is allows, then we can't have STDIN on any. */
	if (!IS_ALLOWED(SCONFIG_CONSOLE_INPUT))
		return NULL;

	/*
	 * Assumption to have BOTH CONSOLE_STDIN AND STDOUT in the
	 * same output console
	 */
	for_each_console(cdev) {
		if ((cdev->f_active & active) == active)
			return cdev;
	}

	return NULL;
}
EXPORT_SYMBOL(console_get_first_interactive);

struct console_device *of_console_get_by_alias(const char *alias)
{
	struct device_node *node;
	struct device *dev;

	node = of_find_node_by_alias(NULL, alias);
	if (!node)
		return NULL;

	dev = of_find_device_by_node(node);
	if (!dev)
		return NULL;

	return console_get_by_dev(dev);
}
EXPORT_SYMBOL(of_console_get_by_alias);

#endif /* !CONFIG_CONSOLE_NONE */

int vdprintf(int file, const char *fmt, va_list args)
{
	char printbuffer[CFG_PBSIZE];

	/*
	 * For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);

	/* Print the string */
	return dputs(file, printbuffer);
}
EXPORT_SYMBOL(vdprintf);

int dprintf(int file, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vdprintf(file, fmt, args);
	va_end(args);

	return i;
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

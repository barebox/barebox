/*
 * (C) Copyright 2000
 * Paolo Scaffardi, AIRVENT SAM s.p.a - RIMINI(ITALY), arsenio@tin.it
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <param.h>
#include <console.h>
#include <driver.h>
#include <fs.h>
#include <reloc.h>
#include <init.h>
#include <clock.h>
#include <kfifo.h>
#include <module.h>

static struct console_device *first_console = NULL;

#define CONSOLE_UNINITIALIZED	0
#define CONSOLE_INIT_EARLY	1
#define CONSOLE_INIT_FULL	2

static int __initdata initialized = 0;

static int console_std_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct console_device *cdev = dev->type_data;
	unsigned int flag = 0, i = 0;

	if (strchr(val, 'i') && cdev->f_caps & CONSOLE_STDIN) {
		cdev->active[i++] = 'i';
		flag |= CONSOLE_STDIN;
	}

	if (strchr(val, 'o') && cdev->f_caps & CONSOLE_STDOUT) {
		cdev->active[i++] = 'o';
		flag |= CONSOLE_STDOUT;
	}

	if (strchr(val, 'e') && cdev->f_caps & CONSOLE_STDERR) {
		cdev->active[i++] = 'e';
		flag |= CONSOLE_STDERR;
	}

	cdev->active[i] = 0;
	cdev->f_active = flag;

	return 0;
}

static int console_baudrate_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct console_device *cdev = dev->type_data;
	int baudrate;

	baudrate = simple_strtoul(val, NULL, 10);

	if (cdev->f_active) {
		printf("## Switch baudrate to %d bps and press ENTER ...\n",
			baudrate);
		mdelay(50);
		cdev->setbrg(cdev, baudrate);
		mdelay(50);
		while (getc() != '\r');
	} else
		cdev->setbrg(cdev, baudrate);

	sprintf(cdev->baudrate_string, "%d", baudrate);

	return 0;
}

int console_register(struct console_device *newcdev)
{
	struct console_device *cdev = first_console;
	struct device_d *dev = newcdev->dev;

	if (newcdev->setbrg) {
		newcdev->baudrate_param.set = console_baudrate_set;
		newcdev->baudrate_param.name = "baudrate";
		sprintf(newcdev->baudrate_string, "%d",
			CONFIG_BAUDRATE);
		console_baudrate_set(dev, &newcdev->baudrate_param,
			newcdev->baudrate_string);
		cdev->baudrate_param.value = newcdev->baudrate_string;
		dev_add_param(dev, &newcdev->baudrate_param);
	}

	newcdev->active_param.set = console_std_set;
	newcdev->active_param.name  = "active";
	newcdev->active_param.value = newcdev->active;
	dev_add_param(dev, &newcdev->active_param);

	initialized = CONSOLE_INIT_FULL;
#ifdef CONFIG_CONSOLE_ACTIVATE_ALL
	console_std_set(dev, &newcdev->active_param, "ioe");
#endif
	if (!first_console) {
#ifdef CONFIG_CONSOLE_ACTIVATE_FIRST
		console_std_set(dev, &newcdev->active_param, "ioe");
#endif
		first_console = newcdev;
		return 0;
	}

	while (1) {
		if (!cdev->next) {
			cdev->next = newcdev;
			return 0;
		}
		cdev = cdev->next;
	}
}
EXPORT_SYMBOL(console_register);

int getc_raw(void)
{
	struct console_device *cdev = NULL;
	while (1) {
		if (!cdev)
			cdev = first_console;
		if (cdev->f_active & CONSOLE_STDIN && cdev->tstc(cdev))
			return cdev->getc(cdev);
		cdev = cdev->next;
	}
}

static struct kfifo *console_buffer;

int getc_buffer_flush(void)
{
	console_buffer = kfifo_alloc(1024);
	return 0;
}

postcore_initcall(getc_buffer_flush);

int getc(void)
{
	unsigned char ch;
	uint64_t start;

	start = get_time_ns();
	while (1) {
		if (tstc()) {
			kfifo_putc(console_buffer, getc_raw());

			start = get_time_ns();
		}
		if (is_timeout(start, 100 * USECOND) && kfifo_len(console_buffer))
			break;
	}

	kfifo_getc(console_buffer, &ch);
	return ch;
}
EXPORT_SYMBOL(getc);

int fgetc(int fd)
{
	char c;

	if (!fd)
		return getc();
	return read(fd, &c, 1);
}
EXPORT_SYMBOL(fgetc);

int tstc(void)
{
	struct console_device *cdev = first_console;

	while (cdev) {
		if (cdev->f_active & CONSOLE_STDIN && cdev->tstc(cdev))
			return 1;
		cdev = cdev->next;
	}

	return 0;
}
EXPORT_SYMBOL(tstc);

void __initdata *early_console_base;

void console_putc(unsigned int ch, char c)
{
	struct console_device *cdev = first_console;
	int init = INITDATA(initialized);

	switch (init) {
	case CONSOLE_UNINITIALIZED:
		return;

#ifdef CONFIG_HAS_EARLY_INIT
	case CONSOLE_INIT_EARLY:
		early_console_putc(INITDATA(early_console_base), c);
		return;
#endif

	case CONSOLE_INIT_FULL:
		while (cdev) {
			if (cdev->f_active & ch) {
				cdev->putc(cdev, c);
				if (c == '\n')
					cdev->putc(cdev, '\r');
			}
			cdev = cdev->next;
		}
		return;
	default:
		/* If we have problems inititalizing our data
		 * get them early
		 */
		hang();
	}
}
EXPORT_SYMBOL(console_putc);

int fputc(int fd, char c)
{
	if(!first_console) {
		if(!fd)
			console_putc(0, c);
		return 0;
	}

	if (fd == 1)
		putchar(c);
	else if (fd == 2)
		eputc(c);
	else
		return write(fd, &c, 1);
	return 0;
}
EXPORT_SYMBOL(fputc);

void console_puts(unsigned int ch, const char *str)
{
	const char *s = str;
	while (*s) {
		console_putc(ch, *s);
		if (*s == '\n')
			console_putc(ch, '\r');
		s++;
	}
}
EXPORT_SYMBOL(console_puts);

int fputs(int fd, const char *s)
{
	if (fd == 1)
		puts(s);
	else if (fd == 2)
		eputs(s);
	else
		return write(fd, s, strlen(s));
	return 0;
}
EXPORT_SYMBOL(fputs);

void fprintf (int file, const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CFG_PBSIZE];

	va_start (args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);
	va_end (args);

	/* Print the string */
	fputs (file, printbuffer);
}
EXPORT_SYMBOL(fprintf);

int printf (const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CFG_PBSIZE];

	va_start (args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);
	va_end (args);

	/* Print the string */
	puts (printbuffer);

	return i;
}
EXPORT_SYMBOL(printf);

int vprintf (const char *fmt, va_list args)
{
	uint i;
	char printbuffer[CFG_PBSIZE];

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);

	/* Print the string */
	puts (printbuffer);

	return i;
}
EXPORT_SYMBOL(vprintf);

/* test if ctrl-c was pressed */
int ctrlc (void)
{
	if (tstc() && getc() == 3)
		return 1;
	return 0;
}
EXPORT_SYMBOL(ctrlc);

#ifdef CONFIG_HAS_EARLY_INIT

void early_console_start(const char *name, int baudrate)
{
	void *base = get_early_console_base(name);

	if (base) {
		early_console_init(base, baudrate);
		INITDATA(initialized) = CONSOLE_INIT_EARLY;
		INITDATA(early_console_base) = base;
	}
}

#endif

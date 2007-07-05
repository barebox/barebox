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
#include <exports.h>
#include <serial.h>
#include <driver.h>
#include <fs.h>

static struct console_device *first_console;

static int console_std_set(struct device_d *dev, struct param_d *param, const char *val)
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

int console_register(struct console_device *newcdev)
{
        struct console_device *cdev = first_console;
	struct device_d *dev = newcdev->dev;

	newcdev->active_param.set = console_std_set;
	newcdev->active_param.name  = "active";
	newcdev->active_param.value = newcdev->active;
	dev_add_param(dev, &newcdev->active_param);
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

int getc (void)
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

int fgetc(int fd)
{
	char c;

	if (!fd)
		return getc();
	return read(fd, &c, 1);
}

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

void console_putc(unsigned int ch, char c)
{
	struct console_device *cdev = first_console;

	while (cdev) {
		if (cdev->f_active & ch)
			cdev->putc(cdev, c);
		cdev = cdev->next;
	}
}

int fputc(int fd, char c)
{
	if (fd == 1)
		putc(c);
	else if (fd == 2)
		eputc(c);
	else
		return write(fd, &c, 1);
	return 0;
}

void console_puts(unsigned int ch, const char *str)
{
	struct console_device *cdev = first_console;

	while (cdev) {
		if (cdev->f_active & ch) {
			const char *s = str;
			while (*s) {
				cdev->putc(cdev, *s);
				if (*s == '\n') {
					cdev->putc(cdev, '\r');
				}
				s++;
			}
		}
		cdev = cdev->next;
	}
}

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

void printf (const char *fmt, ...)
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
}


void vprintf (const char *fmt, va_list args)
{
	uint i;
	char printbuffer[CFG_PBSIZE];

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf (printbuffer, fmt, args);

	/* Print the string */
	puts (printbuffer);
}

/* test if ctrl-c was pressed */
int ctrlc (void)
{
	if (tstc() && getc() == 3)
		return 1;
	return 0;
}


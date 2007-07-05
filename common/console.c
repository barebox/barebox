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

#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <console.h>
#include <exports.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_AMIGAONEG3SE
int console_changed = 0;
#endif

#ifdef CFG_CONSOLE_IS_IN_ENV
/*
 * if overwrite_console returns 1, the stdin, stderr and stdout
 * are switched to the serial port, else the settings in the
 * environment are used
 */
#ifdef CFG_CONSOLE_OVERWRITE_ROUTINE
extern int overwrite_console (void);
#define OVERWRITE_CONSOLE overwrite_console ()
#else
#define OVERWRITE_CONSOLE 0
#endif /* CFG_CONSOLE_OVERWRITE_ROUTINE */

#endif /* CFG_CONSOLE_IS_IN_ENV */

/** U-Boot INITIAL CONSOLE-NOT COMPATIBLE FUNCTIONS *************************/

void serial_printf (const char *fmt, ...)
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

	serial_puts (printbuffer);
}
#if 0
int fgetc (int file)
{
	if (file < MAX_FILES)
		return stdio_devices[file]->getc ();

	return -1;
}

int ftstc (int file)
{
	if (file < MAX_FILES)
		return stdio_devices[file]->tstc ();

	return -1;
}

void fputc (int file, const char c)
{
	if (file < MAX_FILES)
		stdio_devices[file]->putc (c);
}

void fputs (int file, const char *s)
{
	if (file < MAX_FILES)
		stdio_devices[file]->puts (s);
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

	/* Send to desired file */
	fputs (file, printbuffer);
}
#endif
/** U-Boot INITIAL CONSOLE-COMPATIBLE FUNCTION *****************************/

int getc (void)
{
	return serial_getc ();
}

int tstc (void)
{
	return serial_tstc ();
}

void putc (const char c)
{
#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT)
		return;
#endif

	serial_putc (c);
}

void puts (const char *s)
{
#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT)
		return;
#endif

	serial_puts (s);
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
static int ctrlc_disabled = 0;	/* see disable_ctrl() */
static int ctrlc_was_pressed = 0;
int ctrlc (void)
{
	if (!ctrlc_disabled && gd->have_console) {
		if (tstc ()) {
			switch (getc ()) {
			case 0x03:		/* ^C - Control C */
				ctrlc_was_pressed = 1;
				return 1;
			default:
				break;
			}
		}
	}
	return 0;
}

/* pass 1 to disable ctrlc() checking, 0 to enable.
 * returns previous state
 */
int disable_ctrlc (int disable)
{
	int prev = ctrlc_disabled;	/* save previous state */

	ctrlc_disabled = disable;
	return prev;
}

int had_ctrlc (void)
{
	return ctrlc_was_pressed;
}

void clear_ctrlc (void)
{
	ctrlc_was_pressed = 0;
}

/* Called before relocation - use serial functions */
int console_init_f (void)
{
	gd->have_console = 1;

#ifdef CONFIG_SILENT_CONSOLE
	if (getenv("silent") != NULL)
		gd->flags |= GD_FLG_SILENT;
#endif

	return (0);
}

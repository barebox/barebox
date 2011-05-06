#include <config.h>
#include <common.h>
#include <fs.h>
#include <errno.h>

LIST_HEAD(console_list);
EXPORT_SYMBOL(console_list);
static struct console_device *console;

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
	puts(printbuffer);

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
	fputs(file, printbuffer);
}
EXPORT_SYMBOL(fprintf);

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

void console_putc(unsigned int ch, char c)
{
	if (!console)
		return;

	console->putc(console, c);
	if (c == '\n')
		console->putc(console, '\r');
}
EXPORT_SYMBOL(console_putc);

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

int tstc(void)
{
	if (!console)
		return 0;

	return console->tstc(console);
}
EXPORT_SYMBOL(tstc);

int getc(void)
{
	if (!console)
		return -EINVAL;
	return console->getc(console);
}
EXPORT_SYMBOL(getc);

void console_flush(void)
{
	if (console && console->flush)
		console->flush(console);
}
EXPORT_SYMBOL(console_flush);

#ifndef ARCH_HAS_CTRLC
/* test if ctrl-c was pressed */
int ctrlc (void)
{
	if (tstc() && getc() == 3)
		return 1;
	return 0;
}
EXPORT_SYMBOL(ctrlc);
#endif /* ARCH_HAS_CTRC */

int console_register(struct console_device *newcdev)
{
	if (!console) {
		console = newcdev;
		console_list.prev = console_list.next = &newcdev->list;
		newcdev->list.prev = newcdev->list.next = &console_list;

		barebox_banner();
	}
	return 0;
}

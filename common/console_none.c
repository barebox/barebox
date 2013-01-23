#include <config.h>
#include <common.h>
#include <fs.h>
#include <errno.h>
#include <debug_ll.h>

int fputc(int fd, char c)
{
	if (fd != 1 && fd != 2)
		return write(fd, &c, 1);
	return 0;
}
EXPORT_SYMBOL(fputc);

int fputs(int fd, const char *s)
{
	if (fd != 1 && fd != 2)
		return write(fd, s, strlen(s));
	return 0;
}
EXPORT_SYMBOL(fputs);

int fprintf(int file, const char *fmt, ...)
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

	return i;
}
EXPORT_SYMBOL(fprintf);

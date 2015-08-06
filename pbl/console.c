#include <common.h>
#include <debug_ll.h>

static void (*__putc)(void *ctx, int c);
static void *putc_ctx;

/**
 * pbl_set_putc() - setup UART used for PBL console
 * @putc: The putc function.
 * @ctx: The context pointer passed back to putc
 *
 * This sets the putc function which is afterwards used to output
 * characters in the PBL.
 */
void pbl_set_putc(void (*putcf)(void *ctx, int c), void *ctx)
{
	__putc = putcf;
	putc_ctx = ctx;
}

void console_putc(unsigned int ch, char c)
{
	if (!__putc)
		putc_ll(c);
	else
		__putc(putc_ctx, c);
}

int console_puts(unsigned int ch, const char *str)
{
	int n = 0;

	while (*str) {
		if (*str == '\n')
			putc_ll('\r');

		console_putc(CONSOLE_STDOUT, *str);
		str++;
		n++;
	}

	return n;
}

int printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CFG_PBSIZE];

	va_start(args, fmt);
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	console_puts(CONSOLE_STDOUT, printbuffer);

	return i;
}

int pr_print(int level, const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CFG_PBSIZE];

	va_start(args, fmt);
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	console_puts(CONSOLE_STDOUT, printbuffer);

	return i;
}

int ctrlc(void)
{
	return 0;
}

// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <debug_ll.h>
#include <asm/sections.h>
#include <linux/err.h>

/*
 * Put these in the data section so that they survive the clearing of the
 * BSS segment.
 */
static __attribute__ ((section(".data"))) ulong putc_offset;
static __attribute__ ((section(".data"))) void *putc_ctx;

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
	putc_offset = (ulong)putcf - (ulong)_text;
	putc_ctx = ctx;
}

static void __putc(void *ctx, int c)
{
	void (*putc)(void *, int) = (void *)_text + putc_offset;
	putc(ctx, c);
}

void console_putc(unsigned int ch, char c)
{
	if (putc_offset)
		__putc(putc_ctx, c);
	else
		putc_ll(c);
}

int console_puts(unsigned int ch, const char *str)
{
	int n = 0;

	while (*str) {
		if (*str == '\n')
			console_putc(ch, '\r');

		console_putc(ch, *str);
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
	i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
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
	i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
	va_end(args);

	console_puts(CONSOLE_STDERR, printbuffer);

	return i;
}

int ctrlc(void)
{
	return 0;
}

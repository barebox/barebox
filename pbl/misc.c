// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

void __noreturn hang(void)
{
	while (1);
}

void __noreturn panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	printf(fmt, args);
	va_end(args);
	while(1);
}

void __noreturn panic_no_stacktrace(const char *fmt, ...)
	__alias(panic);

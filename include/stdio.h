/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <console.h>
#include <printk.h>

/*
 * STDIO based functions (can always be used)
 */

/* serial stuff */
void serial_printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));

int sprintf(char *buf, const char *fmt, ...) __attribute__ ((format(__printf__, 2, 3)));
int snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__ ((format(__printf__, 3, 4)));
int scnprintf(char *buf, size_t size, const char *fmt, ...) __attribute__ ((format(__printf__, 3, 4)));
int vsprintf(char *buf, const char *fmt, va_list args);
char *basprintf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
int asprintf(char **strp, const char *fmt, ...)  __attribute__ ((format(__printf__, 2, 3)));
char *bvasprintf(const char *fmt, va_list ap);
int vasprintf(char **strp, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

#ifndef CONFIG_CONSOLE_NONE
/* stdin */
int tstc(void);

/* stdout */
void console_putc(unsigned int ch, const char c);
int getchar(void);
int console_puts(unsigned int ch, const char *s);
void console_flush(void);

int vprintf(const char *fmt, va_list args);
#else
static inline int tstc(void)
{
	return 0;
}

static inline int console_puts(unsigned int ch, const char *str)
{
	return 0;
}

static inline int getchar(void)
{
	return -EINVAL;
}

static inline void console_putc(unsigned int ch, char c) {}

static inline void console_flush(void) {}

static inline int vprintf(const char *fmt, va_list args)
{
	return 0;
}

#ifndef CONFIG_ARCH_HAS_CTRLC
/* test if ctrl-c was pressed */
static inline int ctrlc (void)
{
	return 0;
}
#endif /* CONFIG_ARCH_HAS_CTRLC */

#endif

#if (!defined(__PBL__) && !defined(CONFIG_CONSOLE_NONE)) || \
	(defined(__PBL__) && defined(CONFIG_PBL_CONSOLE))
static inline int puts(const char *s)
{
	return console_puts(CONSOLE_STDOUT, s);
}

static inline void putchar(char c)
{
	console_putc(CONSOLE_STDOUT, c);
}
#else
static inline int puts(const char *s)
{
	return 0;
}

static inline void putchar(char c)
{
	return;
}
#endif

/*
 * FILE based functions
 */

/* stderr */
#define eprintf(fmt,args...)	dprintf(STDERR_FILENO, fmt ,##args)

#define STDIN_FILENO		0
#define STDOUT_FILENO		1
#define STDERR_FILENO		2
#define MAX_FILES	128

int dprintf(int file, const char *fmt, ...) __attribute__ ((format(__printf__, 2, 3)));
int dputs(int file, const char *s);
int dputc(int file, const char c);

#endif /* __STDIO_H */

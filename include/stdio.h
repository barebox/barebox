/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <console.h>
#include <printf.h>
#include <xfuncs.h>
#include <linux/sprintf.h>

/*
 * STDIO based functions (can always be used)
 */

#ifdef CONFIG_ARCH_HAS_CTRLC
int arch_ctrlc(void);
#endif

#ifndef CONFIG_CONSOLE_NONE
/* stdin */
int tstc(void);

/* stdout */
void console_putc(unsigned int ch, const char c);
int getchar(void);
int console_puts(unsigned int ch, const char *s);
void console_flush(void);

int vprintf(const char *fmt, va_list args);

int ctrlc(void);
int ctrlc_non_interruptible(void);
void ctrlc_handled(void);
void console_ctrlc_allow(void);
void console_ctrlc_forbid(void);
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

/* test if ctrl-c was pressed */
static inline int ctrlc (void)
{
	return 0;
}

static inline int ctrlc_non_interruptible(void)
{
	return 0;
}

static inline void ctrlc_handled(void)
{
}

static inline void console_ctrlc_allow(void) { }
static inline void console_ctrlc_forbid(void) { }
#endif

int readline(const char *prompt, char *buf, int len);

#if (IN_PROPER && !defined(CONFIG_CONSOLE_NONE)) || \
	(IN_PBL && defined(CONFIG_PBL_CONSOLE))
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

#ifdef __PBL__
#define eprintf			printf
#define veprintf		vprintf
#define eputchar		putchar
#else
#define eprintf(fmt,args...)	dprintf(STDERR_FILENO, fmt ,##args)
#define veprintf(fmt,args)	vdprintf(STDERR_FILENO, fmt, args)
#define eputchar(ch)		dputc(STDERR_FILENO, ch)
#endif

#define STDIN_FILENO		0
#define STDOUT_FILENO		1
#define STDERR_FILENO		2
#define MAX_FILES	128

int vdprintf(int fd, const char *fmt, va_list args) ;
int dprintf(int file, const char *fmt, ...) __printf(2, 3);
int dputs(int file, const char *s);
int dputc(int file, const char c);

#endif /* __STDIO_H */

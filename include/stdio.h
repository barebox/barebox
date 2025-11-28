/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <console.h>
#include <printf.h>
#include <xfuncs.h>
#include <linux/sprintf.h>

#ifndef CONFIG_CONSOLE_NONE
/* stdin */
int tstc(void);
int getchar(void);
int vprintf(const char *fmt, va_list args);
#else
static inline int tstc(void) { return 0; }
static inline int vprintf(const char *fmt, va_list args) { return 0; }
static inline int getchar(void) { return -EINVAL; }
#endif

int readline(const char *prompt, char *buf, int len);

#if (IN_PROPER && !defined(CONFIG_CONSOLE_NONE)) || \
	(IN_PBL && defined(CONFIG_PBL_CONSOLE))
static inline int puts(const char *s) { return console_puts(CONSOLE_STDOUT, s); }
static inline void putchar(char c) { console_putc(CONSOLE_STDOUT, c); }
#else
static inline int puts(const char *s) { return 0; }
static inline void putchar(char c) {}
#endif

int readline(const char *prompt, char *buf, int len);

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

int vdprintf(int fd, const char *fmt, va_list args) ;
int dprintf(int file, const char *fmt, ...) __printf(2, 3);
int dputs(int file, const char *s);
int dputc(int file, const char c);

#endif /* __STDIO_H */

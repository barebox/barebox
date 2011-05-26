#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <console.h>

/*
 * STDIO based functions (can always be used)
 */

/* serial stuff */
void	serial_printf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));

/* stdin */
int	tstc(void);

/* stdout */
void	console_putc(unsigned int ch, const char c);
int	getc(void);
void	console_puts(unsigned int ch, const char *s);
void	console_flush(void);

static inline void puts(const char *s)
{
	console_puts(CONSOLE_STDOUT, s);
}

static inline void putchar(char c)
{
	console_putc(CONSOLE_STDOUT, c);
}

int	printf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
int	vprintf(const char *fmt, va_list args);
int	sprintf(char *buf, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
int	snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__ ((format(printf, 3, 4)));
int	vsprintf(char *buf, const char *fmt, va_list args);
char	*asprintf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
char	*vasprintf(const char *fmt, va_list ap);
int	vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int	vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

/* stderr */
#define eputc(c)		console_putc(CONSOLE_STDERR, c)
#define eputs(s)		console_puts(CONSOLE_STDERR, s)
#define eprintf(fmt,args...)	fprintf(stderr,fmt ,##args)

/*
 * FILE based functions
 */

#define stdin		0
#define stdout		1
#define stderr		2
#define MAX_FILES	128

void	fprintf(int file, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
int	fputs(int file, const char *s);
int	fputc(int file, const char c);
int	ftstc(int file);
int	fgetc(int file);

#endif /* __STDIO_H */

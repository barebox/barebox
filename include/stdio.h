#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <console.h>

/*
 * STDIO based functions (can always be used)
 */

/* serial stuff */
void	serial_printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));

int	sprintf(char *buf, const char *fmt, ...) __attribute__ ((format(__printf__, 2, 3)));
int	snprintf(char *buf, size_t size, const char *fmt, ...) __attribute__ ((format(__printf__, 3, 4)));
int	vsprintf(char *buf, const char *fmt, va_list args);
char	*asprintf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
char	*vasprintf(const char *fmt, va_list ap);
int	vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int	vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

#ifndef CONFIG_CONSOLE_NONE
/* stdin */
int	tstc(void);

/* stdout */
void	console_putc(unsigned int ch, const char c);
int	getc(void);
int	console_puts(unsigned int ch, const char *s);
void	console_flush(void);


int	printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
int	vprintf(const char *fmt, va_list args);
#else
static inline int tstc(void)
{
	return 0;
}

static inline int console_puts(unsigned int ch, const char *str)
{
	return 0;
}

static inline int getc(void)
{
	return -EINVAL;
}

static inline void console_putc(unsigned int ch, char c) {}

static inline void console_flush(void) {}

static int printf(const char *fmt, ...) __attribute__ ((format(__printf__, 1, 2)));
static inline int printf(const char *fmt, ...)
{
	return 0;
}


static inline int vprintf(const char *fmt, va_list args)
{
	return 0;
}

#ifndef ARCH_HAS_CTRLC
/* test if ctrl-c was pressed */
static inline int ctrlc (void)
{
	return 0;
}
#endif /* ARCH_HAS_CTRLC */

#endif

static inline int puts(const char *s)
{
	return console_puts(CONSOLE_STDOUT, s);
}

static inline void putchar(char c)
{
	console_putc(CONSOLE_STDOUT, c);
}

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

int	fprintf(int file, const char *fmt, ...) __attribute__ ((format(__printf__, 2, 3)));
int	fputs(int file, const char *s);
int	fputc(int file, const char c);
int	fgetc(int file);

#endif /* __STDIO_H */

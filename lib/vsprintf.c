/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#include <stdarg.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm-generic/div64.h>
#include <malloc.h>
#include <kallsyms.h>

#include <common.h>
#include <led.h>

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}
EXPORT_SYMBOL(simple_strtoul);

long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}
EXPORT_SYMBOL(simple_strtol);

unsigned long long simple_strtoull (const char *cp, char **endp, unsigned int base)
{
	unsigned long long result = 0, value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit (cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit (*cp) && (value = isdigit (*cp)
				? *cp - '0'
				: (islower (*cp) ? toupper (*cp) : *cp) - 'A' + 10) < base) {
		result = result * base + value;
		cp++;
	}
	if (endp)
		*endp = (char *) cp;
	return result;
}
EXPORT_SYMBOL(simple_strtoull);

/* we use this so that we can do without the ctype library */
#define is_digit(c)	((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
	int i=0;

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SMALL	32		/* Must be 32 == 0x20 */
#define SPECIAL	64		/* 0x */

static char *number(char *buf, char *end, unsigned long long num, int base, int size, int precision, int type)
{
	/* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
	static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

	char tmp[66];
	char sign;
	char locase;
	int need_pfx = ((type & SPECIAL) && base != 10);
	int i;

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	 * produces same digits or (maybe lowercased) letters */
	locase = (type & SMALL);
	if (type & LEFT)
		type &= ~ZEROPAD;
	sign = 0;
	if (type & SIGN) {
		if ((signed long long) num < 0) {
			sign = '-';
			num = - (signed long long) num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (need_pfx) {
		size--;
		if (base == 16)
			size--;
	}
	/* generate full string in tmp[], in reverse order */
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else do {
		tmp[i++] = (digits[do_div(num,base)] | locase);
	} while (num != 0);

	/* printing 100 using %2d gives "100", not "00" */
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type & (ZEROPAD+LEFT))) {
		while(--size >= 0) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}
	/* sign */
	if (sign) {
		if (buf < end)
			*buf = sign;
		++buf;
	}
	/* "0x" / "0" prefix */
	if (need_pfx) {
		if (buf < end)
			*buf = '0';
		++buf;
		if (base == 16) {
			if (buf < end)
				*buf = ('X' | locase);
			++buf;
		}
	}
	/* zero or space padding */
	if (!(type & LEFT)) {
		char c = (type & ZEROPAD) ? '0' : ' ';
		while (--size >= 0) {
			if (buf < end)
				*buf = c;
			++buf;
		}
	}
	/* hmm even more zero padding? */
	while (i <= --precision) {
		if (buf < end)
			*buf = '0';
		++buf;
	}
	/* actual digits of result */
	while (--i >= 0) {
		if (buf < end)
			*buf = tmp[i];
		++buf;
	}
	/* trailing space padding */
	while (--size >= 0) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}
	return buf;
}

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static char *string(char *buf, char *end, char *s, int field_width, int precision, int flags)
{
	int len, i;

	if ((unsigned long)s < PAGE_SIZE)
		s = "<NULL>";

	len = strnlen(s, precision);

	if (!(flags & LEFT)) {
		while (len < field_width--) {
			if (buf < end)
				*buf = ' ';
			++buf;
		}
	}
	for (i = 0; i < len; ++i) {
		if (buf < end)
			*buf = *s;
		++buf; ++s;
	}
	while (len < field_width--) {
		if (buf < end)
			*buf = ' ';
		++buf;
	}
	return buf;
}

static char *symbol_string(char *buf, char *end, void *ptr, int field_width, int precision, int flags)
{
	unsigned long value = (unsigned long) ptr;
#ifdef CONFIG_KALLSYMS
	char sym[KSYM_SYMBOL_LEN];
	sprint_symbol(sym, value);
	return string(buf, end, sym, field_width, precision, flags);
#else
	field_width = 2*sizeof(void *);
	flags |= SPECIAL | SMALL | ZEROPAD;
	return number(buf, end, value, 16, field_width, precision, flags);
#endif
}

/*
 * Show a '%p' thing.  A kernel extension is that the '%p' is followed
 * by an extra set of alphanumeric characters that are extended format
 * specifiers.
 *
 * Right now we handle:
 *
 * - 'S' For symbolic direct pointers
 *
 * Note: The difference between 'S' and 'F' is that on ia64 and ppc64
 * function pointers are really function descriptors, which contain a
 * pointer to the real address.
 */
static char *pointer(const char *fmt, char *buf, char *end, void *ptr, int field_width, int precision, int flags)
{
	switch (*fmt) {
	case 'S':
		return symbol_string(buf, end, ptr, field_width, precision, flags);
	}
	flags |= SMALL;
	if (field_width == -1) {
		field_width = 2*sizeof(void *);
		flags |= ZEROPAD;
	}
	return number(buf, end, (unsigned long) ptr, 16, field_width, precision, flags);
}

/**
 * vsnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * This function follows C99 vsnprintf, but has some extensions:
 * %pS output the name of a text symbol
 * %pF output the name of a function pointer
 * %pR output the address range in a struct resource
 *
 * The return value is the number of characters which would
 * be generated for the given input, excluding the trailing
 * '\0', as per ISO C99. If you want to have the exact
 * number of characters written into @buf as return value
 * (not including the trailing '\0'), use vscnprintf(). If the
 * return is greater than or equal to @size, the resulting
 * string is truncated.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want snprintf() instead.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	unsigned long long num;
	int base;
	char *str, *end, c;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */
				/* 'z' support added 23/7/1999 S.H.    */
				/* 'z' changed to 'Z' --davidm 1/25/99 */
				/* 't' added for ptrdiff_t */

	/* Reject out-of-range values early.  Large positive sizes are
	   used for unknown buffer sizes. */
	if (unlikely((int) size < 0))
		return 0;

	str = buf;
	end = buf + size;

	/* Make sure end is always >= buf */
	if (end < buf) {
		end = ((void *)-1);
		size = end - buf;
	}

	for (; *fmt ; ++fmt) {
		if (*fmt != '%') {
			if (str < end)
				*str = *fmt;
			++str;
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
			}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;	
			if (isdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
		    *fmt =='Z' || *fmt == 'z' || *fmt == 't') {
			qualifier = *fmt;
			++fmt;
			if (qualifier == 'l' && *fmt == 'l') {
				qualifier = 'L';
				++fmt;
			}
		}

		/* default base */
		base = 10;

		switch (*fmt) {
			case 'c':
				if (!(flags & LEFT)) {
					while (--field_width > 0) {
						if (str < end)
							*str = ' ';
						++str;
					}
				}
				c = (unsigned char) va_arg(args, int);
				if (str < end)
					*str = c;
				++str;
				while (--field_width > 0) {
					if (str < end)
						*str = ' ';
					++str;
				}
				continue;

			case 's':
				str = string(str, end, va_arg(args, char *), field_width, precision, flags);
				continue;

			case 'p':
				str = pointer(fmt+1, str, end,
						va_arg(args, void *),
						field_width, precision, flags);
				/* Skip all alphanumeric pointer suffixes */
				while (isalnum(fmt[1]))
					fmt++;
				continue;

			case 'n':
				/* FIXME:
				* What does C99 say about the overflow case here? */
				if (qualifier == 'l') {
					long * ip = va_arg(args, long *);
					*ip = (str - buf);
				} else if (qualifier == 'Z' || qualifier == 'z') {
					size_t * ip = va_arg(args, size_t *);
					*ip = (str - buf);
				} else {
					int * ip = va_arg(args, int *);
					*ip = (str - buf);
				}
				continue;

			case '%':
				if (str < end)
					*str = '%';
				++str;
				continue;

				/* integer number formats - set up the flags and "break" */
			case 'o':
				base = 8;
				break;

			case 'x':
				flags |= SMALL;
			case 'X':
				base = 16;
				break;

			case 'd':
			case 'i':
				flags |= SIGN;
			case 'u':
				break;

			default:
				if (str < end)
					*str = '%';
				++str;
				if (*fmt) {
					if (str < end)
						*str = *fmt;
					++str;
				} else {
					--fmt;
				}
				continue;
		}
		if (qualifier == 'L')
			num = va_arg(args, long long);
		else if (qualifier == 'l') {
			num = va_arg(args, unsigned long);
			if (flags & SIGN)
				num = (signed long) num;
		} else if (qualifier == 'Z' || qualifier == 'z') {
			num = va_arg(args, size_t);
		} else if (qualifier == 't') {
			num = va_arg(args, ptrdiff_t);
		} else if (qualifier == 'h') {
			num = (unsigned short) va_arg(args, int);
			if (flags & SIGN)
				num = (signed short) num;
		} else {
			num = va_arg(args, unsigned int);
			if (flags & SIGN)
				num = (signed int) num;
		}
		str = number(str, end, num, base,
				field_width, precision, flags);
	}
	if (size > 0) {
		if (str < end)
			*str = '\0';
		else
			end[-1] = '\0';
	}
	/* the trailing null byte doesn't count towards the total */
	return str-buf;
}
EXPORT_SYMBOL(vsnprintf);

/**
 * vscnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * The return value is the number of characters which have been written into
 * the @buf not including the trailing '\0'. If @size is <= 0 the function
 * returns 0.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want scnprintf() instead.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	int i;

	i=vsnprintf(buf,size,fmt,args);
	return (i >= size) ? (size - 1) : i;
}
EXPORT_SYMBOL(vscnprintf);

/**
 * vsprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * The function returns the number of characters written
 * into @buf. Use vsnprintf() or vscnprintf() in order to avoid
 * buffer overflows.
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want sprintf() instead.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
	return vsnprintf(buf, INT_MAX, fmt, args);
}
EXPORT_SYMBOL(vsprintf);

int sprintf(char * buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);
	return i;
}
EXPORT_SYMBOL(sprintf);

int snprintf(char * buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);
	return i;
}
EXPORT_SYMBOL(snprintf);

/* Simplified asprintf. */
char *vasprintf(const char *fmt, va_list ap)
{
	unsigned int len;
	char *p;
	va_list aq;

	va_copy(aq, ap);
	len = vsnprintf(NULL, 0, fmt, aq);
	va_end(aq);

	p = malloc(len + 1);
	if (!p)
		return NULL;

	vsnprintf(p, len + 1, fmt, ap);

	return p;
}
EXPORT_SYMBOL(vasprintf);

char *asprintf(const char *fmt, ...)
{
	va_list ap;
	char *p;

	va_start(ap, fmt);
	p = vasprintf(fmt, ap);
	va_end(ap);

	return p;
}
EXPORT_SYMBOL(asprintf);

void __noreturn panic(const char *fmt, ...)
{
	va_list	args;
	va_start(args, fmt);
	vprintf(fmt, args);
	putchar('\n');
	va_end(args);

	dump_stack();

	led_trigger(LED_TRIGGER_PANIC, TRIGGER_ENABLE);

	if (IS_ENABLED(CONFIG_PANIC_HANG)) {
		hang();
	} else {
		udelay(100000);	/* allow messages to go out */
		reset_cpu(0);
	}
}
EXPORT_SYMBOL(panic);

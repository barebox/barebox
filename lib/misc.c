/*
 * misc.c - various assorted functions
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <string.h>
#include <linux/ctype.h>

/*
 * Like simple_strtoull() but handles an optional G, M, K or k
 * suffix for Gigabyte, Megabyte or Kilobyte
 */
unsigned long long strtoull_suffix(const char *str, char **endp, int base)
{
	unsigned long long val;
	char *end;

	val = simple_strtoull(str, &end, base);

	switch (*end) {
	case 'G':
		val *= 1024;
	case 'M':
		val *= 1024;
	case 'k':
	case 'K':
		val *= 1024;
		end++;
	default:
		break;
	}

	if (strncmp(end, "iB", 2) == 0)
		end += 2;

	if (endp)
		*endp = end;

	return val;
}
EXPORT_SYMBOL(strtoull_suffix);

unsigned long strtoul_suffix(const char *str, char **endp, int base)
{
	return strtoull_suffix(str, endp, base);
}
EXPORT_SYMBOL(strtoul_suffix);

/*
 * This function parses strings in the form <startadr>[-endaddr]
 * or <startadr>[+size] and fills in start and size accordingly.
 * <startadr> and <endadr> can be given in decimal or hex (with 0x prefix)
 * and can have an optional G, M, K or k suffix.
 *
 * examples:
 * 0x1000-0x2000 -> start = 0x1000, size = 0x1001
 * 0x1000+0x1000 -> start = 0x1000, size = 0x1000
 * 0x1000        -> start = 0x1000, size = ~0
 * 1M+1k         -> start = 0x100000, size = 0x400
 */
int parse_area_spec(const char *str, loff_t *start, loff_t *size)
{
	char *endp;
	loff_t end, _start, _size;

	if (!isdigit(*str))
		return -1;

	_start = strtoull_suffix(str, &endp, 0);

	str = endp;

	if (!*str) {
		/* beginning given, but no size, assume maximum size */
		_size = ~0;
		goto success;
	}

	if (*str == '-') {
		/* beginning and end given */
		if (!isdigit(*(str + 1)))
			return -1;

		end = strtoull_suffix(str + 1, &endp, 0);
		str = endp;
		if (end < _start) {
			printf("end < start\n");
			return -1;
		}
		_size = end - _start + 1;
		goto success;
	}

	if (*str == '+') {
		/* beginning and size given */
		if (!isdigit(*(str + 1)))
			return -1;

		_size = strtoull_suffix(str + 1, &endp, 0);
		str = endp;
		goto success;
	}

	return -1;

success:
	if (*str && !isspace(*str))
		return -1;
	*start = _start;
	*size = _size;
	return 0;
}
EXPORT_SYMBOL(parse_area_spec);

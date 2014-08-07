/*
 * wchar.c - wide character support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#include <wchar.h>
#include <malloc.h>
#include <string.h>

size_t wcslen(const wchar_t *s)
{
	size_t len = 0;

	while (*s++)
		len++;

	return len;
}

char *strcpy_wchar_to_char(char *dst, const wchar_t *src)
{
	char *ret = dst;

	while (*src)
		*dst++ = *src++ & 0xff;

	*dst = 0;

	return ret;
}

wchar_t *strcpy_char_to_wchar(wchar_t *dst, const char *src)
{
	wchar_t *ret = dst;

	while (*src)
		*dst++ = *src++;

	*dst = 0;

	return ret;
}

wchar_t *strdup_char_to_wchar(const char *src)
{
	wchar_t *dst = malloc((strlen(src) + 1) * sizeof(wchar_t));

	if (!dst)
		return NULL;

	strcpy_char_to_wchar(dst, src);

	return dst;
}

char *strdup_wchar_to_char(const wchar_t *src)
{
	char *dst = malloc((wcslen(src) + 1));

	if (!dst)
		return NULL;

	strcpy_wchar_to_char(dst, src);

	return dst;
}

/*
 * xfuncs.c - safe malloc funcions
 *
 * based on busybox
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
#include <module.h>

void *xmalloc(size_t size)
{
	void *p = NULL;

	if (!(p = malloc(size)))
		panic("ERROR: out of memory\n");

	debug("xmalloc %p (size %zu)\n", p, size);

	return p;
}
EXPORT_SYMBOL(xmalloc);

void *xrealloc(void *ptr, size_t size)
{
	void *p = NULL;

	if (!(p = realloc(ptr, size)))
		panic("ERROR: out of memory\n");

	debug("xrealloc %p -> %p (size %zu)\n", ptr, p, size);

	return p;
}
EXPORT_SYMBOL(xrealloc);

void *xzalloc(size_t size)
{
	void *ptr = xmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}
EXPORT_SYMBOL(xzalloc);

char *xstrdup(const char *s)
{
	char *p = strdup(s);

	if (!p)
		panic("ERROR: out of memory\n");
	return p;
}
EXPORT_SYMBOL(xstrdup);

void* xmemalign(size_t alignment, size_t bytes)
{
	void *p = memalign(alignment, bytes);
	if (!p)
		panic("ERROR: out of memory\n");
	return p;
}
EXPORT_SYMBOL(xmemalign);

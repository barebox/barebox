/*
 * Keep all the ugly #ifdef for system stuff here
 */

#ifndef __XALLOC_H__
#define __XALLOC_H__

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

__attribute__ ((__returns_nonnull__))
static inline void *xmalloc(size_t size)
{
	void *p = NULL;

	if (!(p = malloc(size))) {
		printf("ERROR: out of memory\n");
		exit(1);
	}

	return p;
}

__attribute__ ((__returns_nonnull__))
static inline void *xzalloc(size_t size)
{
	void *p = xmalloc(size);
	memset(p, 0, size);
	return p;
}

static inline void *xcalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);

	if (!p)
		exit(1);
	return p;
}

static inline void *xrealloc(void *p, size_t size)
{
	p = realloc(p, size);
	if (!p)
		exit(1);
	return p;
}

static inline char *xstrdup(const char *s)
{
	char *p = strdup(s);

	if (!p)
		exit(1);
	return p;
}

static inline char *xstrndup(const char *s, size_t n)
{
	char *p = strndup(s, n);

	if (!p)
		exit(1);
	return p;
}

#endif

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

#endif

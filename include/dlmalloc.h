/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DLMALLOC_H
#define __DLMALLOC_H

#include <linux/compiler.h>
#include <types.h>

void *dlmalloc(size_t) __alloc_size(1);
size_t dlmalloc_usable_size(void *);
void dlfree(void *);
void *dlrealloc(void *, size_t) __realloc_size(2);
void *dlmemalign(size_t, size_t) __alloc_size(2);
void *dlcalloc(size_t, size_t) __alloc_size(1, 2);

#endif /* __DLMALLOC_H */

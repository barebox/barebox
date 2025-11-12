/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Talloc is a replacement for the standard memory allocation routines that
 * provides structure aware allocations.
 *
 * @author Dario Sneidermanis
 */

#ifndef __TALLOC_H__
#define __TALLOC_H__

#include <linux/types.h>
#include <linux/compiler_types.h>

struct talloc {
	struct talloc *child;
	struct talloc *next;
	union {
		struct talloc *prev;
		struct talloc *parent; /* Valid only when is_first(mem) */
	};
} __aligned(8); /* align to 8 byte to maintain malloc alignment */

void mem_set_parent(struct talloc *child, struct talloc *parent);

void *talloc_new(const void *parent);
size_t talloc_usable_size(void *mem);
void *talloc_size(const void *parent, size_t size);
void *talloc_zero_size(const void *parent, size_t size);
void *talloc_realloc_size(const void *parent, void *mem, size_t size);
void talloc_free(void *mem);

char *talloc_strdup(const void *parent, const char *str);
const char *talloc_strdup_const(const void *parent, const char *str);
void talloc_free_const(const void *ptr);

void *talloc_parent(const void *mem);
void talloc_steal(const void *parent, void *mem);
void talloc_steal_children(const void *parent, void *mem);

size_t talloc_total_blocks(const void *mem);

#endif /* __TALLOC_H__ */

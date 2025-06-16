// SPDX-License-Identifier: GPL-2.0-only
/*
 * Helpers for formatting and printing strings
 *
 * Copyright 31 August 2008 James Bottomley
 * Copyright (C) 2013, Intel Corporation
 */

#include <linux/types.h>
#include <linux/bug.h>
#include <linux/printk.h>
#include <linux/linkage.h>

void __read_overflow2_field(size_t avail, size_t wanted);
void __write_overflow_field(size_t avail, size_t wanted);
void fortify_panic(const char *name);

#ifdef CONFIG_FORTIFY_SOURCE
/* These are placeholders for fortify compile-time warnings. */
void __read_overflow2_field(size_t avail, size_t wanted) { }
EXPORT_SYMBOL(__read_overflow2_field);
void __write_overflow_field(size_t avail, size_t wanted) { }
EXPORT_SYMBOL(__write_overflow_field);

void fortify_panic(const char *name)
{
	panic("detected buffer overflow in %s\n", name);
}
EXPORT_SYMBOL(fortify_panic);
#endif /* CONFIG_FORTIFY_SOURCE */

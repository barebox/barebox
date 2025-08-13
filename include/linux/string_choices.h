/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_STRING_CHOICES_H_
#define _LINUX_STRING_CHOICES_H_

#include <linux/types.h>

/**
 * str_plural - Return the simple pluralization based on English counts
 * @num: Number used for deciding pluralization
 *
 * If @num is 1, returns empty string, otherwise returns "s".
 */
static inline const char *str_plural(size_t num)
{
	return num == 1 ? "" : "s";
}

#endif

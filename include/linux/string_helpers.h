/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_STRING_HELPERS_H_
#define _LINUX_STRING_HELPERS_H_

#include <linux/string.h>
#include <linux/types.h>

static inline bool string_is_terminated(const char *s, int len)
{
	return memchr(s, '\0', len) ? true : false;
}

#endif

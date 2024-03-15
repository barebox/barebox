/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_NAMEI_H
#define _LINUX_NAMEI_H

#include <linux/kernel.h>
#include <linux/path.h>

enum { MAX_NESTED_LINKS = 8 };

#define MAXSYMLINKS 40

/*
 * Type of the last component on LOOKUP_PARENT
 */
enum {LAST_NORM, LAST_ROOT, LAST_DOT, LAST_DOTDOT, LAST_BIND};

/*
 * The bitmask for a lookup event:
 *  - follow links at the end
 *  - require a directory
 *  - ending slashes ok even for nonexistent files
 */
#define LOOKUP_FOLLOW		0x0001
#define LOOKUP_DIRECTORY	0x0002
#define LOOKUP_AUTOMOUNT	0x0004

#define LOOKUP_PARENT		0x0010

/*
 * Intent data
 */

#define LOOKUP_JUMPED		0x1000

#endif /* _LINUX_NAMEI_H */

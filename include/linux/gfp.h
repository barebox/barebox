/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_GFP_H
#define _LINUX_GFP_H

/* unused in barebox, just bogus values */
#define GFP_KERNEL	0
#define GFP_NOFS	0
#define GFP_USER	0
#define GFP_ATOMIC	0
#define GFP_NOWAIT	0
#define __GFP_NOWARN	0

/* Helper macro to avoid gfp flags if they are the default one */
#define __default_gfp(a,b,...) b
#define default_gfp(...) __default_gfp(,##__VA_ARGS__,GFP_KERNEL)

#endif

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_BUILD_BUG_H
#define _LINUX_BUILD_BUG_H

#define static_assert(expr, ...) __static_assert(expr, ##__VA_ARGS__, #expr)
#define __static_assert(expr, msg, ...) _Static_assert(expr, msg)

#endif	/* _LINUX_BUILD_BUG_H */

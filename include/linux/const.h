/* SPDX-License-Identifier: GPL-2.0-only */

/* const.h: Macros for dealing with constants.  */

#ifndef _LINUX_CONST_H
#define _LINUX_CONST_H

#include <uapi/linux/const.h>

#define UL(x)		(_UL(x))
#define ULL(x)		(_ULL(x))


/*
 * This returns a constant expression while determining if an argument is
 * a constant expression, most importantly without evaluating the argument.
 * Glory to Martin Uecker <Martin.Uecker@med.uni-goettingen.de>
 */
#define __is_constexpr(x) \
	(sizeof(int) == sizeof(*(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))

#endif /* !(_LINUX_CONST_H) */

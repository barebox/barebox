/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ALIGN_H
#define _LINUX_ALIGN_H

#include <linux/const.h>

#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define ALIGN_DOWN(x, a)	__ALIGN_KERNEL((x) - ((a) - 1), (a))
#define __ALIGN_MASK(x, mask)	__ALIGN_KERNEL_MASK((x), (mask))
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))
#define PTR_ALIGN_DOWN(p, a)	((typeof(p))ALIGN_DOWN((unsigned long)(p), (a)))
#define PTR_IS_ALIGNED(x, a)	IS_ALIGNED((unsigned long)(x), (a))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)

/*
 * The STACK_ALIGN_ARRAY macro is used to allocate a buffer on the stack that
 * meets a minimum alignment requirement.
  */
#define STACK_ALIGN_ARRAY(type, name, nelems, align)		\
	char __##name[sizeof(type) * (nelems) + (align) - 1];	\
	type *name = (type *)ALIGN((uintptr_t)__##name, align)

#endif

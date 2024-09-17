/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ALIGN_H
#define _LINUX_ALIGN_H

#define ALIGN(x, a)		__ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_DOWN(x, a)	ALIGN((x) - ((a) - 1), (a))
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))
#define PTR_ALIGN_DOWN(p, a)	((typeof(p))ALIGN_DOWN((unsigned long)(p), (a)))
#define PTR_IS_ALIGNED(x, a)	IS_ALIGNED((unsigned long)(x), (a))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)

/*
 * The STACK_ALIGN_ARRAY macro is used to allocate a buffer on the stack that
 * meets a minimum alignment requirement.
 *
 * Note that the size parameter is the number of array elements to allocate,
 * not the number of bytes.
 */
#define STACK_ALIGN_ARRAY(type, name, size, align)		\
	char __##name[sizeof(type) * (size) + (align) - 1];	\
	type *name = (type *)ALIGN((unsigned long)__##name, align)

#endif

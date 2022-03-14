/* SPDX-License-Identifier: GPL-2.0 */
/* Misc low level processor primitives */
#ifndef _LINUX_PROCESSOR_H
#define _LINUX_PROCESSOR_H

#include <linux/barebox-wrapper.h>

/*
 * spin_until_cond can be used to wait for a condition to become true. It
 * may be expected that the first iteration will true in the common case
 * (no spinning), so that callers should not require a first "likely" test
 * for the uncontended case before using this primitive.
 *
 * Usage and implementation guidelines are the same as for the spin_begin
 * primitives, above.
 */
#ifndef spin_until_cond
#define spin_until_cond(cond)					\
do {								\
	if (unlikely(!(cond))) {				\
		do {						\
			cpu_relax();				\
		} while (!(cond));				\
	}							\
} while (0)

#endif

#endif /* _LINUX_PROCESSOR_H */

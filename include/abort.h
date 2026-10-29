/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ABORT_H
#define __ABORT_H

#include <linux/types.h>

#if (defined CONFIG_ARCH_HAS_DATA_ABORT_MASK && IN_PROPER) || \
    (defined CONFIG_ARCH_HAS_DATA_ABORT_MASK_PBL && IN_PBL)

/*
 * data_abort_mask - ignore data aborts
 *
 * If data aborts are ignored the data abort handler
 * will just return.
 */
void data_abort_mask(void);

/*
 * data_abort_unmask - Enable data aborts
 *
 * returns true if a data abort has happened between calling data_abort_mask()
 * and data_abort_unmask()
 */
int data_abort_unmask(void);

#else

static inline void data_abort_mask(void)
{
}

static inline int data_abort_unmask(void)
{
	return 0;
}

#endif

#if (defined CONFIG_ARCH_HAS_ASYNC_ABORT_MASK && IN_PROPER) || \
    (defined CONFIG_ARCH_HAS_ASYNC_ABORT_MASK_PBL && IN_PBL)

/*
 * async_abort_mask - ignore asynchronous aborts
 *
 * If asynchronous aborts are ignored the asynchronous abort handler
 * will just return.
 */
void async_abort_mask(void);

/*
 * async_abort_unmask - enable asynchronous aborts
 *
 * returns true if an asynchronous abort has happened between calling
 * async_abort_mask() and async_abort_unmask(). Note that asynchronous aborts
 * are delivered asynchronously, so the code triggering the abort is not
 * guaranteed to trigger an immediate delivery. As a best effort
 * synchronization a data and instruction synchronization barrier is issued to
 * force the abort to be taken before enabling the default "crashing" handler
 * again, ymmv.
 */
int async_abort_unmask(void);

#else

static inline void async_abort_mask(void)
{
}

static inline int async_abort_unmask(void)
{
	return 0;
}

#endif

void set_exception_level_routing(bool enable);

#endif /* __ABORT_H */

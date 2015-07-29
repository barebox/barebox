#ifndef __ABORT_H
#define __ABORT_H

#include <asm/barebox.h>

#if defined ARCH_HAS_DATA_ABORT_MASK && !defined __PBL__

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

#endif /* __ABORT_H */

/* SPDX License Identifier: GPL-2.0 */
#ifndef __BAREBOX_SCHED_H_
#define __BAREBOX_SCHED_H_

#include <poller.h>

static inline void resched(void)
{
	poller_call();
}

#endif

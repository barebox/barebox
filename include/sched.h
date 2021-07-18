/* SPDX License Identifier: GPL-2.0 */
#ifndef __BAREBOX_SCHED_H_
#define __BAREBOX_SCHED_H_

#ifdef CONFIG_HAS_SCHED
void resched(void);
#else
static inline void resched(void)
{
}
#endif

#endif

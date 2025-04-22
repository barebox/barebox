/* SPDX License Identifier: GPL-2.0 */
#ifndef __BAREBOX_SCHED_H_
#define __BAREBOX_SCHED_H_

#if defined CONFIG_HAS_SCHED && IN_PROPER
void resched(void);
#else
static inline void resched(void)
{
}
#endif

#endif

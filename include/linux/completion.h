/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) Copyright 2021 Ahmad Fatoum
 *
 * Async wait-for-completion handler data structures.
 * This allows one bthread to wait for another
 */

#ifndef __LINUX_COMPLETION_H
#define __LINUX_COMPLETION_H

#include <stdio.h>
#include <errno.h>
#include <bthread.h>

struct completion {
	unsigned int done;
};

static inline void init_completion(struct completion *x)
{
	x->done = 0;
}

static inline void reinit_completion(struct completion *x)
{
	x->done = 0;
}

static inline int wait_for_completion_interruptible(struct completion *x)
{
	while (!x->done) {
		switch (bthread_should_stop()) {
		case -EINTR:
			if (!ctrlc())
				continue;
		case 1:
			return -ERESTARTSYS;
		}
	}

	return 0;
}

#define wait_for_completion_killable	\
	wait_for_completion_interruptible

static inline bool completion_done(struct completion *x)
{
	return x->done;
}

static inline void complete(struct completion *x)
{
	x->done = 1;
}

#endif

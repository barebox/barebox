/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __NOTIFIER_H
#define __NOTIFIER_H

#include <linux/list.h>

/*
 * Notifer chains loosely based on the according Linux framework
 */

struct notifier_block {
	int (*notifier_call)(struct notifier_block *, unsigned long, void *);
	struct list_head list;
};

struct notifier_head {
	struct list_head blocks;
};

int notifier_chain_register(struct notifier_head *nh, struct notifier_block *n);
int notifier_chain_unregister(struct notifier_head *nh, struct notifier_block *n);

int notifier_call_chain(struct notifier_head *nh, unsigned long val, void *v);

/*
 * Register a function which is called upon changes of
 * clock frequencies in the system.
 */
int clock_register_client(struct notifier_block *nb);
int clock_unregister_client(struct notifier_block *nb);
int clock_notifier_call_chain(void);

#define NOTIFIER_HEAD(name)					\
	struct notifier_head name = {				\
		.blocks = LIST_HEAD_INIT((name).blocks),	\
	};

#define NOTIFY_DONE		0x0000		/* Don't care */
#define NOTIFY_OK		0x0001		/* Suits me */
#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)
						/* Bad/Veto action */
/*
 * Clean way to return from the notifier and stop further calls.
 */
#define NOTIFY_STOP		(NOTIFY_OK|NOTIFY_STOP_MASK)

/* Encapsulate (negative) errno value (in particular, NOTIFY_BAD <=> EPERM). */
static inline int notifier_from_errno(int err)
{
	if (err)
		return NOTIFY_STOP_MASK | (NOTIFY_OK - err);

	return NOTIFY_OK;
}

#endif /* __NOTIFIER_H */

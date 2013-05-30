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

#endif /* __NOTIFIER_H */


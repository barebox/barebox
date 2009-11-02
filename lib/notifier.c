#include <common.h>
#include <linux/list.h>
#include <notifier.h>

int notifier_chain_register(struct notifier_head *nh, struct notifier_block *n)
{
	list_add_tail(&n->list, &nh->blocks);
	return 0;
}

int notifier_chain_unregister(struct notifier_head *nh, struct notifier_block *n)
{
	list_del(&n->list);
	return 0;
}

int notifier_call_chain(struct notifier_head *nh, unsigned long val, void *v)
{
	struct notifier_block *entry;

	list_for_each_entry(entry, &nh->blocks, list)
		entry->notifier_call(entry, val, v);

	return 0;
}

/*
 *	Notifier list for code which wants to be called at clock
 *	frequency changes.
 */
static NOTIFIER_HEAD(clock_notifier_list);

/**
 *	clock_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 */
int clock_register_client(struct notifier_block *nb)
{
	return notifier_chain_register(&clock_notifier_list, nb);
}

/**
 *	clock_register_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int clock_unregister_client(struct notifier_block *nb)
{
	return notifier_chain_unregister(&clock_notifier_list, nb);
}

/**
 * clock_register_client - notify clients of clock frequency changes
 *
 */
int clock_notifier_call_chain(void)
{
	return notifier_call_chain(&clock_notifier_list, 0, NULL);
}


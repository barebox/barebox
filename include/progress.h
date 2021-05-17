/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PROGRSS_H
#define __PROGRSS_H

#include <linux/types.h>
#include <notifier.h>
#include <errno.h>

/* Initialize a progress bar. If max > 0 a one line progress
 * bar is printed where 'max' corresponds to 100%. If max == 0
 * a multi line progress bar is printed.
 */
void init_progression_bar(loff_t max);

/* update a progress bar to a new value. If now < 0 then a
 * spinner is printed.
 */
void show_progress(loff_t now);

extern struct notifier_head progress_notifier;

enum progress_stage {
	PROGRESS_UNSPECIFIED = 0,
	PROGRESS_UPDATING,
	PROGRESS_UPDATE_SUCCESS,
	PROGRESS_UPDATE_FAIL,
};

/*
 * Notifier list for code which wants to be called at progress
 * This could use by board code to e.g. flash a LED during updates
 */
extern struct notifier_head progress_notifier_list;

/* generic client that just logs the state */
extern struct notifier_block progress_log_client;

static inline int progress_register_client(struct notifier_block *nb)
{
	if (!IS_ENABLED(CONFIG_PROGRESS_NOTIFIER))
		return -ENOSYS;
	return notifier_chain_register(&progress_notifier_list, nb);
}

static inline int progress_unregister_client(struct notifier_block *nb)
{
	if (!IS_ENABLED(CONFIG_PROGRESS_NOTIFIER))
		return -ENOSYS;
	return notifier_chain_unregister(&progress_notifier_list, nb);
}

static inline int progress_notifier_call_chain(enum progress_stage stage, const char *prefix)
{
	if (!IS_ENABLED(CONFIG_PROGRESS_NOTIFIER))
		return -ENOSYS;

	/* clients should not modify the prefix */
	return notifier_call_chain(&progress_notifier_list, stage, (char *)(prefix ?: ""));
}

#endif /*  __PROGRSS_H */

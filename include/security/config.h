/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BAREBOX_SECURITY_CONFIG_H
#define __BAREBOX_SECURITY_CONFIG_H

/*
 * Security policies is an access control mechanism to control when
 * security-sensitive code is allowed to run.
 *
 * This header is included by security-sensitive code to consult
 * the policy and enforce it.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <notifier.h>
#include <security/defs.h>

extern const char *sconfig_names[SCONFIG_NUM];

int sconfig_lookup(const char *name);

extern struct notifier_head sconfig_notifier_list;

bool is_allowed(const struct security_policy *policy, unsigned option);

#define IF_ALLOWABLE(opt, then, else) \
	({ __if_defined(opt##_DEFINED, then, else); })

#ifdef CONFIG_SECURITY_POLICY
#define IS_ALLOWED(opt)		IF_ALLOWABLE(opt, is_allowed(NULL, (opt)), 0)
#define ALLOWABLE_VALUE(opt)	IF_ALLOWABLE(opt, opt, -1)
#else
#define IS_ALLOWED(opt)		1
#define ALLOWABLE_VALUE(opt)	(-1)
#endif

static inline int sconfig_register_handler(struct notifier_block *nb,
					   int (*cb)(struct notifier_block *,
						     unsigned long, void *))
{
	if (!IS_ENABLED(CONFIG_SECURITY_POLICY))
		return -ENOSYS;

	nb->notifier_call = cb;
	return notifier_chain_register(&sconfig_notifier_list, nb);
}

static inline int sconfig_unregister_handler(struct notifier_block *nb)
{
	if (!IS_ENABLED(CONFIG_SECURITY_POLICY))
		return -ENOSYS;
	return notifier_chain_unregister(&sconfig_notifier_list, nb);
}

struct sconfig_notifier_block;
typedef void (*sconfig_notifier_callback_t)(struct sconfig_notifier_block *,
					    enum security_config_option,
					    bool val);

struct sconfig_notifier_block {
	struct notifier_block nb;
	enum security_config_option opt;
	sconfig_notifier_callback_t cb_filtered;
};

int __sconfig_register_handler_filtered(struct sconfig_notifier_block *nb,
					sconfig_notifier_callback_t cb,
					enum security_config_option);

#define sconfig_register_handler_filtered(nb, cb, opt) ({ \
	int __sopt = ALLOWABLE_VALUE(opt); \
	__sopt != -1	?  __sconfig_register_handler_filtered((nb), (cb), __sopt) \
			: -ENOSYS; \
})

#endif /* __BAREBOX_SECURITY_CONFIG_H */

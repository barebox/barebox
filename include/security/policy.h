/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BAREBOX_SECURITY_POLICY_H
#define __BAREBOX_SECURITY_POLICY_H

/*
 * Security policies is an access control mechanism to control when
 * security-sensitive code is allowed to run.
 *
 * This header is included by board code that registers and
 * selects (activates) these security policies.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <notifier.h>
#include <security/defs.h>

/*
 * It's recommended to use the following names for the
 * "standard" policies
 */
#define POLICY_DEVEL		"devel"
#define POLICY_FACTORY		"factory"
#define POLICY_LOCKDOWN		"lockdown"
#define POLICY_TAMPER		"tamper"
#define POLICY_FIELD_RETURN	"return"

extern const struct security_policy *active_policy;

const struct security_policy *security_policy_get(const char *name);

int security_policy_activate(const struct security_policy *policy);
int security_policy_select(const char *name);
void security_policy_list(void);

#ifdef CONFIG_SECURITY_POLICY
int __security_policy_register(const struct security_policy policy[]);
#else
static inline int __security_policy_register(const struct security_policy policy[])
{
	return -ENOSYS;
}
#endif

#ifdef CONFIG_CMD_SCONFIG
void security_policy_unregister_one(const struct security_policy *policy);
#endif

#define security_policy_add(name) ({				\
	extern const struct security_policy __policy_##name[];	\
	__security_policy_register(__policy_##name);		\
})

#endif /* __BAREBOX_SECURITY_POLICY_H */

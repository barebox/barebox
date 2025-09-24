// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "policy: " fmt

#include <init.h>
#include <linux/printk.h>
#include <linux/bitmap.h>
#include <param.h>
#include <device.h>
#include <stdio.h>

#include <security/policy.h>
#include <security/config.h>

static const char *sconfig_name(unsigned option)
{
	static char name[sizeof("4294967295")];

	if (IS_ENABLED(CONFIG_SECURITY_POLICY_NAMES))
		return sconfig_names[option];

	snprintf(name, sizeof(name), "%u", option);
	return name;
}

#define policy_debug(policy, opt, fmt, ...) \
	pr_debug("(%s) %s: " fmt, \
		 (policy) ? (policy)->name : "default", \
		 sconfig_name(opt), ##__VA_ARGS__)

#ifdef CONFIG_SECURITY_POLICY_DEFAULT_PERMISSIVE
#define policy_err			pr_err
#define policy_warn_once		pr_warn_once
#define policy_WARN			WARN
#else
#define policy_err(fmt, ...)		panic(pr_fmt(fmt), ##__VA_ARGS__)
#define policy_warn_once(fmt, ...)	panic(pr_fmt(fmt), ##__VA_ARGS__)
#define policy_WARN			BUG
#endif

struct policy_list_entry {
	const struct security_policy *policy;
	struct list_head list;
};

const struct security_policy *active_policy;

static LIST_HEAD(policy_list);
NOTIFIER_HEAD(sconfig_notifier_list);

static bool __is_allowed(const struct security_policy *policy, unsigned option)
{
	if (!policy)
		return true;

	return policy->policy[option];
}

bool is_allowed(const struct security_policy *policy, unsigned option)
{
	policy = policy ?: active_policy;

	if (WARN(option > SCONFIG_NUM))
		return false;

	if (!policy && *CONFIG_SECURITY_POLICY_INIT) {
		security_policy_select(CONFIG_SECURITY_POLICY_INIT);
		policy = active_policy;
	}

	if (policy) {
		bool allow = __is_allowed(policy, option);

		policy_debug(policy, option, "%s for %pS\n",
			 allow ? "allowed" : "denied", (void *)_RET_IP_);

		return allow;
	}

	if (IS_ENABLED(CONFIG_SECURITY_POLICY_DEFAULT_PERMISSIVE))
		pr_warn_once("option %s checked before security policy was set!\n",
			     sconfig_name(option));
	else
		panic(pr_fmt("option %s checked before security policy was set!"),
			sconfig_name(option));

	return true;
}

int security_policy_activate(const struct security_policy *policy)
{
	const struct security_policy *old_policy = active_policy;

	if (policy == old_policy)
		return 0;

	active_policy = policy;

	for (int i = 0; i < SCONFIG_NUM; i++) {
		if (__is_allowed(policy, i) == __is_allowed(old_policy, i))
			continue;

		notifier_call_chain(&sconfig_notifier_list, i, NULL);
	}

	return 0;
}

const struct security_policy *security_policy_get(const char *name)
{
	const struct policy_list_entry *entry;

	list_for_each_entry(entry, &policy_list, list) {
		if (!strcmp(name, entry->policy->name))
			return entry->policy;
	}

	return NULL;
}

int security_policy_select(const char *name)
{
	const struct security_policy *policy;

	policy = security_policy_get(name);
	if (!policy) {
		policy_err("Policy '%s' not found!\n", name);
		return -ENOENT;
	}

	return security_policy_activate(policy);
}

int __security_policy_register(const struct security_policy policy[])
{
	int ret = 0;

	do {
		struct policy_list_entry *entry;

		if (security_policy_get(policy->name)) {
			policy_err("policy '%s' already registered\n", policy->name);
			ret = -EBUSY;
			continue;
		}

		entry = xzalloc(sizeof(*entry));
		entry->policy = policy;
		list_add_tail(&entry->list, &policy_list);
	} while ((policy++)->chained);

	return ret;
}

#ifdef CONFIG_CMD_SCONFIG
void security_policy_unregister_one(const struct security_policy *policy)
{
	struct policy_list_entry *entry;

	if (!policy)
		return;

	list_for_each_entry(entry, &policy_list, list) {
		if (entry->policy == policy) {
			list_del(&entry->list);
			return;
		}
	}
}
#endif

void security_policy_list(void)
{
	const struct policy_list_entry *entry;

	list_for_each_entry(entry, &policy_list, list) {
		printf("%s\n", entry->policy->name);
	}
}

static int sconfig_handler_filtered(struct notifier_block *nb,
				    unsigned long opt, void *data)
{
	struct sconfig_notifier_block *snb
		= container_of(nb, struct sconfig_notifier_block, nb);
	bool allow;

	if (snb->opt != opt)
		return NOTIFY_DONE;

	allow = is_allowed(NULL, opt);

	policy_debug(active_policy, opt, "calling %pS to %s\n",
		     snb->cb_filtered, allow ? "allow" : "deny");

	snb->cb_filtered(snb, opt, is_allowed(NULL, opt));
	return NOTIFY_OK;
}

int __sconfig_register_handler_filtered(struct sconfig_notifier_block *snb,
					sconfig_notifier_callback_t cb,
					enum security_config_option opt)
{
	snb->cb_filtered = cb;
	snb->opt = opt;
	return sconfig_register_handler(&snb->nb, sconfig_handler_filtered);
}

struct device security_device = {
	.name = "security",
	.id = DEVICE_ID_SINGLE,
};

static char *policy_name = "";

static int security_policy_get_name(struct param_d *param, void *priv)
{
	if (!active_policy) {
		policy_name = "";
		return 0;
	}

	free_const(policy_name);
	policy_name = strdup(active_policy->name);
	return 0;
}

static int security_init(void)
{
	register_device(&security_device);

	dev_add_param_string(&security_device, "policy", param_set_readonly,
			     security_policy_get_name, &policy_name, NULL);

	if (*CONFIG_SECURITY_POLICY_PATH)
		security_policy_add(default);

	return 0;
}
pure_initcall(security_init);

// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/printk.h>
#include <pinctrl.h>
#include <security/policy.h>
#include <security/config.h>

/* Maximum number of names to be available in pinctrl-names. Best guess.*/
#define MAX_NAMES 10

/**
 * Replace 'default' with 'old_default' and 'barebox,policy-<active_policy>-default'
 * with 'default', if both are found in pinctrl.
 * Same for every other name, that starts with barebox,policy-<active_policy>- and
 * has another name without the prefix.
 */
static int kernel_of_fixup_pinctrl(struct device_node *root, void *unused)
{
	char *basename;
	int base_len;
	struct device_node *node;
	const struct security_policy *active_policy = security_policy_get_active();

	if (!active_policy || !active_policy->name)
		return -EINVAL;
	basename = basprintf("barebox,policy-%s-", active_policy->name);
	base_len = strlen(basename);

	list_for_each_entry(node, &root->list, list) {
		const char *names[MAX_NAMES];
		int num_read;
		bool changed = false;

		num_read = of_property_read_string_array(node, "pinctrl-names", names, MAX_NAMES);
		if (num_read == MAX_NAMES) {
			int total = of_property_count_strings(node, "pinctrl-names");

			if (total > MAX_NAMES)
				pr_warn("Node %s has more then %d pinctrl names defined. Skipping last %d\n",
					node->full_name, MAX_NAMES, total - MAX_NAMES);
		}

		for (int i = 0; i < num_read; i++) {
			if (strncmp(basename, names[i], base_len) == 0) {
				const char *name = names[i] + base_len;

				for (int ii = 0; ii < num_read; ii++) {
					if (strcmp(name, names[ii]) == 0) {
						/* this never gets freed,
						 * but we are starting the kernel,
						 * so there should be no problem.
						 */
						names[ii] = basprintf("old_%s", name);
						names[i] = name;
						changed = true;
						break;
					}
				}
			}
		}
		if (changed)
			of_property_write_string_array(node, "pinctrl-names", names, num_read);
	}
	free(basename);
	return 0;
}

static int policy_kernel_pinctrl_fixup_init(void)
{
	return of_register_fixup(kernel_of_fixup_pinctrl, NULL);
}
late_initcall(policy_kernel_pinctrl_fixup_init);

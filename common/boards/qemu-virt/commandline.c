// SPDX-License-Identifier: GPL-2.0-or-later

#define pr_fmt(fmt) "qemu-virt-commandline: " fmt

#include <linux/parser.h>
#include <of.h>
#include <string.h>
#include <security/policy.h>
#include <xfuncs.h>
#include <stdio.h>
#include "commandline.h"

enum {
	/* String options */
	Opt_policy,
	/* Error token */
	Opt_err
};

static const match_table_t tokens = {
	{Opt_policy, "barebox.security.policy=%s"},
	{Opt_err, NULL}
};

int qemu_virt_parse_commandline(struct device_node *np)
{
	const char *bootargs;
	char *p, *options, *tmp_options, *policy = NULL;
	substring_t args[MAX_OPT_ARGS];
	int ret;

	np = of_get_child_by_name(np, "chosen");
	if (!np)
		return -ENOENT;

	ret = of_property_read_string(np, "bootargs", &bootargs);
	if (ret < 0)
		return 0;

	options = tmp_options = xstrdup(bootargs);

	while ((p = strsep(&options, " ")) != NULL) {
		int token;

		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_policy:
			if (!IS_ENABLED(CONFIG_SECURITY_POLICY)) {
				pr_err("CONFIG_SECURITY_POLICY support is missing\n");
				continue;
			}

			policy = match_strdup(&args[0]);
			if (!policy) {
				ret = -ENOMEM;
				goto out;
			}
			ret = security_policy_select(policy);
			if (ret)
				goto out;
		default:
			continue;
		}
	}

	ret = 0;
out:
	free(policy);
	free(tmp_options);
	return ret;
}

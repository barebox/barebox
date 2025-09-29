// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <command.h>
#include <environment.h>
#include <getopt.h>
#include <globalvar.h>
#include <linux/ctype.h>
#include <stdio.h>

#include <security/policy.h>
#include <security/config.h>

#define RED	"\e[1;31m"
#define GREEN	"\e[1;32m"
#define YELLOW	"\e[1;33m"
#define PINK	"\e[1;35m"
#define CYAN	"\e[1;36m"
#define NC	"\e[0m"

static struct security_policy *last_override;
static int debug_iteration;
static struct notifier_block sconfig_notifier;

static const char *red, *green, *nc;

static void sconfig_print(const struct security_policy *policy)
{
	for (int i = 0; i < SCONFIG_NUM; i++) {
		bool allow = is_allowed(policy, i);

		printf("[%s%s%s] %s\n", allow ? green : red,
		       allow ? "✓" : "✗", nc, sconfig_names[i]);
	}
}

static int sconfig_command_notify(struct notifier_block *nb,
				  unsigned long opt, void *unused)
{
	bool allow = is_allowed(NULL, opt);

	printf("%s%s%s%s\n", allow ? green : red, allow ? "+" : "-", nc,
	       sconfig_names[opt]);

	return 0;
}

static int do_sconfig_flags(int argc, char *argv[])
{
	const struct security_policy *policy;
	const char *set = NULL;
	int ret = 0, opt;

	while ((opt = getopt(argc, argv, "li:vs:")) > 0) {
		switch (opt) {
		case 'l':
			security_policy_list();
			break;
		case 'i':
			policy = security_policy_get(optarg);
			if (!policy) {
				ret = -ENOENT;
				goto out;
			}

			sconfig_print(policy);
			break;
		case 'v':
			if (!IS_ENABLED(CONFIG_CMD_SCONFIG_MODIFY))
				return COMMAND_ERROR_USAGE;
			sconfig_register_handler(&sconfig_notifier,
						 sconfig_command_notify);
			break;
		case 's':
			if (!IS_ENABLED(CONFIG_CMD_SCONFIG_MODIFY))
				return COMMAND_ERROR_USAGE;
			set = optarg;
			break;
		default:
			ret = COMMAND_ERROR_USAGE;
			goto out;
		}
	}

	if (argc != optind) {
		ret = COMMAND_ERROR_USAGE;
		goto out;
	}

	if (set) {
		policy = security_policy_get(set);
		if (!policy) {
			printf("Policy '%s' not found\n", set);
			ret = -ENOENT;
			goto out;
		}

		ret = security_policy_activate(policy);
	}
out:
	if (sconfig_notifier.notifier_call) {
		sconfig_unregister_handler(&sconfig_notifier);
		sconfig_notifier.notifier_call = NULL;
	}

	return ret;
}

static struct security_policy *alloc_policy(void)
{
	struct security_policy *override;

	override = xmalloc(sizeof(*override));

	if (active_policy)
		memcpy(override->policy, active_policy->policy,
		       sizeof(override->policy));
	else
		memset(override->policy, 0x1, sizeof(override->policy));

	override->name = xasprintf("debug%u", debug_iteration++);
	override->chained = false;

	return override;
}

static void free_policy(struct security_policy *policy)
{
	security_policy_unregister_one(policy);
	free(policy);
}

static int do_sconfig(int argc, char *argv[])
{
	struct security_policy *override;
	bool allow_color;
	int i, ret;

	allow_color = !streq_ptr(globalvar_get("allow_color"), "0");
	if (allow_color) {
		red = RED;
		green = GREEN;
		nc = NC;
	} else {
		red = green = nc = "";
	}

	if (argc == 1) {
		if (active_policy)
			printf("Active Policy: %s\n\n", active_policy->name);
		else
			printf("No Policy is Active\n\n");

		sconfig_print(NULL);
		return 0;
	}

	for (i = 1; i < argc; i++) {
		if ((*argv[i] != '-' && *argv[i] != '+' ) ||
		    !strstarts(argv[i] + 1, "SCONFIG_"))
			return do_sconfig_flags(argc, argv);;
	}

	if (!IS_ENABLED(CONFIG_CMD_SCONFIG_MODIFY)) {
		printf("Runtime security policy modification disallowed\n");
		return COMMAND_ERROR_USAGE;
	}

	override = alloc_policy();

	for (i = 1; i < argc; i++) {
		char ch_action = *argv[i];
		enum security_config_option sopt;

		ret = sconfig_lookup(argv[i] + 1);
		if (ret < 0) {
			printf("No such option: %s\n", argv[i] + 1);
			return ret;
		}

		sopt = ret;

		switch (ch_action) {
		case '+':
			override->policy[sopt] = true;
			break;
		case '-':
			override->policy[sopt] = false;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	__security_policy_register(override);

	ret = security_policy_activate(override);
	if (ret) {
		free_policy(override);
		return ret;
	}

	free_policy(last_override);
	last_override = override;

	return 0;
}

BAREBOX_CMD_HELP_START(sconfig)
BAREBOX_CMD_HELP_TEXT("Interact with security policies")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l",  "list registered security policies")
BAREBOX_CMD_HELP_OPT ("-i POLICY",  "show security options of specified POLICY")
#ifdef CONFIG_CMD_SCONFIG_MODIFY
BAREBOX_CMD_HELP_OPT ("-v",  "verbose output: report options as they are modified")
BAREBOX_CMD_HELP_OPT ("-s POLICY",  "select specified POLICY")
#endif
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sconfig)
        .cmd            = do_sconfig,
        BAREBOX_CMD_DESC("interact with security policies")
        BAREBOX_CMD_OPTS("[-vlsi] [<+->SCONFIG_*]...")
        BAREBOX_CMD_GROUP(CMD_GRP_SECURITY)
        BAREBOX_CMD_HELP(cmd_sconfig_help)
BAREBOX_CMD_END

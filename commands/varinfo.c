// SPDX-License-Identifier: GPL-2.0-or-later

#include <command.h>
#include <common.h>
#include <complete.h>
#include <driver.h>
#include <environment.h>
#include <fnmatch.h>

struct iter_ctx {
	const char *prefix;
	bool found;
};

static int printenv_by_prefix(struct variable_d *v, void *data)
{
	struct iter_ctx *ctx = data;

	if (strstarts(var_name(v), ctx->prefix)) {
		printf("%s: %s (environment variable)\n", var_name(v), var_val(v));
		ctx->found = true;
	}

	return 0;
}

static int do_varinfo(int argc, char *argv[])
{
	struct device *dev;
	struct param_d *param;
	const char *prefix = NULL;
	char *arg, *dot;
	bool found = false;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	arg = argv[1];

	dot = strchr(arg, '.');
	if (dot) {
		*dot = '\0';
		if (dot[1])
			prefix = &dot[1];
	} else {
		struct iter_ctx ctx = { arg };

		envvar_for_each(printenv_by_prefix, &ctx);
		if (!ctx.found)
			goto not_found;

		return 0;
	}

	dev = get_device_by_name(arg);
	if (!dev)
		return -ENODEV;

	list_for_each_entry(param, &dev->parameters, list) {
		if (prefix && !strstarts(param->name, prefix))
			continue;

		printf("%s: %s (type: %s)", param->name,
		       dev_get_param(dev, param->name), get_param_type(param));
		if (param->info)
			param->info(param);
		printf("\n");
		found = true;
	}

	if (!found)
		goto not_found;

	return 0;
not_found:
	printf("%s: no matching variable found\n", arg);
	return 1;
}

BAREBOX_CMD_HELP_START(varinfo)
BAREBOX_CMD_HELP_TEXT("shows information about the variable in its argument")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(varinfo)
	.cmd		= do_varinfo,
	BAREBOX_CMD_DESC("show information about variables")
	BAREBOX_CMD_OPTS("VAR")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_varinfo_help)
	BAREBOX_CMD_COMPLETE(env_param_plain_complete)
BAREBOX_CMD_END

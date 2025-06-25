// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <fs.h>
#include <getopt.h>
#include <malloc.h>
#include <linux/stat.h>
#include <linux/ctype.h>
#include <environment.h>
#include <block.h>
#include <stringlist.h>

static int devlookup_process(struct cdev *cdev, void *sl)
{
	string_list_add(sl, cdev->name);
	return 1;
}

static int do_devlookup(int argc, char *argv[])
{
	const char *variable = NULL, *devicefile, *paramname;
	struct cdev *cdev = NULL;
	int opt, ret;
	bool alias = false, kernelopt = false;
	const char *val;
	char *aliasbuf = NULL, *buf = NULL;
	struct string_list sl;

	while ((opt = getopt(argc, argv, "v:ka")) > 0) {
		switch(opt) {
		case 'v':
			variable = optarg;
			break;
		case 'k':
			kernelopt = true;
			break;
		case 'a':
			alias = true;
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc == 0 || argc > 2 || (kernelopt && argc != 1))
		return COMMAND_ERROR_USAGE;

	devicefile = argv[0];
	paramname  = argv[1];

	string_list_init(&sl);

	ret = cdev_alias_resolve_for_each(devicefile, devlookup_process, &sl);
	if (ret < 0)
		goto out;
	else if (ret > 1) {
		aliasbuf = string_list_join(&sl, " ");
		if (string_list_count(&sl) > 1 && (kernelopt || paramname)) {
			printf("Option not supported for multi cdev alias\n");
			ret = COMMAND_ERROR;
			goto out;
		}

		if (alias) {
			ret = cmd_export_val(variable, aliasbuf);
			goto out;
		}

		devicefile = aliasbuf;
	}

	cdev = cdev_open_by_path_name(devicefile, O_RDONLY);
	if (!cdev) {
		printf("devlookup: cdev %s not found\n", devicefile);
		ret = -ENOENT;
		goto out;
	}

	if (!cdev->dev) {
		printf("devlookup: cdev %s not associated with a device\n", devicefile);
		ret = -ENODEV;
		goto out;
	}

	if (kernelopt)
		val = buf = cdev_get_linux_rootarg(cdev);
	else if (paramname)
		val = dev_get_param(cdev->dev, paramname);
	else
		val = dev_name(cdev->dev);

	ret = cmd_export_val(variable, val);
out:
	string_list_free(&sl);
	free(aliasbuf);
	free(buf);
	if (cdev)
		cdev_close(cdev);

	return ret;
}

BAREBOX_CMD_HELP_START(devlookup)
BAREBOX_CMD_HELP_TEXT("Detects the device behind a device file and outputs it,")
BAREBOX_CMD_HELP_TEXT("unless a second argument is given. In that case the device")
BAREBOX_CMD_HELP_TEXT("parameter with that name is looked up.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v",  "write output to VARIABLE instead of printing it")
BAREBOX_CMD_HELP_OPT ("-k",  "output kernel rootarg line")
BAREBOX_CMD_HELP_OPT ("-a",  "output resolution of cdev alias")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(devlookup)
	.cmd		= do_devlookup,
	BAREBOX_CMD_DESC("look up device behind device file and its parameters")
	BAREBOX_CMD_OPTS("[-v VAR] [-ka] /dev/DEVICE [parameter]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_devlookup_help)
BAREBOX_CMD_END


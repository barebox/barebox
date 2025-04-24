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

static int do_devlookup(int argc, char *argv[])
{
	const char *variable = NULL, *devicefile, *paramname;
	struct cdev *cdev;
	int opt, ret;
	bool kernelopt = false;
	const char *val;
	char *buf = NULL;

	while ((opt = getopt(argc, argv, "v:k")) > 0) {
		switch(opt) {
		case 'v':
			variable = optarg;
			break;
		case 'k':
			kernelopt = true;
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc == 0 || argc > 2 || (kernelopt && argc != 1))
		return COMMAND_ERROR_USAGE;

	devicefile = argv[0];
	paramname  = argv[1];

	devicefile = devpath_to_name(devicefile);

	cdev = cdev_open_by_name(devicefile, O_RDONLY);
	if (!cdev) {
		printf("devlookup: cdev %s not found\n", devicefile);
		return -ENOENT;
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
	free(buf);
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
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(devlookup)
	.cmd		= do_devlookup,
	BAREBOX_CMD_DESC("look up device behind device file and its parameters")
	BAREBOX_CMD_OPTS("[-v VAR] [-k] /dev/DEVICE [parameter]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_devlookup_help)
BAREBOX_CMD_END


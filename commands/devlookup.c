// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <fs.h>
#include <getopt.h>
#include <malloc.h>
#include <linux/stat.h>
#include <linux/ctype.h>
#include <environment.h>

static int report(const char *variable, const char *val)
{
	if (!val)
		return -(errno ?: EINVAL);

	if (variable)
		return setenv(variable, val);

	printf("%s\n", val);
	return 0;
}

static int do_devlookup(int argc, char *argv[])
{
	const char *variable = NULL, *devicefile, *paramname;
	struct cdev *cdev;
	int opt, ret;

	while ((opt = getopt(argc, argv, "v:")) > 0) {
		switch(opt) {
		case 'v':
			variable = optarg;
			break;
		}
	}

	if (argc - optind == 0 || argc - optind > 2)
		return COMMAND_ERROR_USAGE;

	devicefile = argv[optind];
	paramname  = argv[optind + 1];

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

	if (paramname)
		ret = report(variable, dev_get_param(cdev->dev, paramname));
	else
		ret = report(variable, dev_name(cdev->dev));
out:
	cdev_close(cdev);

	return ret;
}

BAREBOX_CMD_HELP_START(devlookup)
BAREBOX_CMD_HELP_TEXT("Detects the device behind a device file and outputs it,")
BAREBOX_CMD_HELP_TEXT("unless a second argument is given. In that case the device")
BAREBOX_CMD_HELP_TEXT("parameter with that name is looked up. Specifying -v VARIABLE")
BAREBOX_CMD_HELP_TEXT("will write output to VARIABLE instead of printing it")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(devlookup)
	.cmd		= do_devlookup,
	BAREBOX_CMD_DESC("look up device behind device file and its parameters")
	BAREBOX_CMD_OPTS("[-v VAR] /dev/DEVICE [parameter]")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_devlookup_help)
BAREBOX_CMD_END


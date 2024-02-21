// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2021 Ahmad Fatoum <a.fatoum@pengutronix.de>, Pengutronix

#include <command.h>
#include <common.h>
#include <complete.h>
#include <driver.h>
#include <getopt.h>

static int do_devunbind(int argc, char *argv[])
{
	bool unregister = false;
	struct device *dev;
	int ret = COMMAND_SUCCESS, i, opt;

	while ((opt = getopt(argc, argv, "fl")) > 0) {
		switch (opt) {
		case 'f':
			unregister = true;
			break;
		case 'l':
			list_for_each_entry(dev, &active_device_list, active) {
				void *rm_dev;

				BUG_ON(!dev->driver);

				rm_dev = dev->bus->remove ?: dev->driver->remove;

				if (rm_dev)
					printf("%pS(%s, %s)\n", rm_dev,
					       dev->driver->name, dev_name(dev));
			}
			return 0;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!argv[optind])
		return COMMAND_ERROR_USAGE;

	for (i = optind; i < argc; i++) {
		dev = get_device_by_name(argv[i]);
		if (!dev) {
			printf("skipping missing %s\n", argv[i]);
			ret = -ENODEV;
			continue;
		}

		if (unregister) {
			unregister_device(dev);
			continue;
		}

		if (!device_remove(dev)) {
			printf("no remove callback registered for %s\n", argv[i]);
			ret = COMMAND_ERROR;
			continue;
		}

		dev->driver = NULL;
		list_del(&dev->active);
	}

	return ret;
}

BAREBOX_CMD_HELP_START(devunbind)
BAREBOX_CMD_HELP_TEXT("Debugging aid to unbind device from driver at runtime")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-f",  "unbind driver and force removal of device and children")
BAREBOX_CMD_HELP_OPT ("-l",  "list remove callbacks in shutdown order")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(devunbind)
	.cmd		= do_devunbind,
	BAREBOX_CMD_DESC("unbind device(s) from driver")
	BAREBOX_CMD_OPTS("[-fl] DEVICES..")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_devunbind_help)
	BAREBOX_CMD_COMPLETE(device_complete)
BAREBOX_CMD_END

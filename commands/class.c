// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <command.h>
#include <complete.h>

static int do_class(int argc, char *argv[])
{
	struct class *class;
	struct device *dev;

	class_for_each(class) {
		printf("%s:\n", class->name);
		class_for_each_device(class, dev) {
			printf("    %s\n", dev_name(dev));
		}
	}

	return 0;
}

BAREBOX_CMD_HELP_START(class)
BAREBOX_CMD_HELP_TEXT("Show information about registered classes and their devices")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(class)
	.cmd = do_class,
	BAREBOX_CMD_DESC("show information about classes")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

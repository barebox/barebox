#include <common.h>
#include <command.h>
#include <getopt.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <string.h>
#include <environment.h>
#include <aiodev.h>

static int do_hwmon(int argc, char *argv[])
{
	int i;
	struct aiodevice *aiodev;

	for_each_aiodevice(aiodev) {
		for (i = 0; i < aiodev->num_channels; i++) {
			struct aiochannel *chan = aiodev->channels[i];
			int value;
			int ret = aiochannel_get_value(chan, &value);

			if (!ret)
				printf("%s: %d %s\n", chan->name, value, chan->unit);
			else
				printf("%s: failed to read (%d)\n", chan->name, ret);
		}
	}

	return 0;
}

BAREBOX_CMD_START(hwmon)
	.cmd		= do_hwmon,
	BAREBOX_CMD_DESC("query hardware sensors (HWMON)")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END

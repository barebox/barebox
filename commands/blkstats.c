// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <command.h>
#include <block.h>
#include <getopt.h>
#include <fs.h>

static int do_blkstats(int argc, char *argv[])
{
	struct block_device *blk;
	const char *name;
	bool first = false;
	int opt;

	while ((opt = getopt(argc, argv, "l")) > 0) {
		switch (opt) {
		case 'l':
			for_each_block_device(blk) {
				printf("%s\n", blk->cdev.name);
			}
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	name = argv[0];

	for_each_block_device(blk) {
		struct block_device_stats *stats;

		if (name && strcmp(name, blk->cdev.name))
			continue;

		if (first) {
			printf("%-16s %10s %10s %10s\n",
			       "Device", "Read", "Write", "Erase");
			first = true;
		}

		stats = &blk->stats;

		printf("%-16s %10llu %10llu %10llu\n", blk->cdev.name,
		       stats->read_sectors, stats->write_sectors, stats->erase_sectors);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(blkstats)
BAREBOX_CMD_HELP_TEXT("Display a block device's number of read, written and erased sectors")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-l",  "list all currently registered block devices")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(blkstats)
	.cmd		= do_blkstats,
	BAREBOX_CMD_DESC("display block layer statistics")
	BAREBOX_CMD_OPTS("[-l] [DEVICE]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_blkstats_help)
BAREBOX_CMD_END

// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <command.h>
#include <block.h>
#include <getopt.h>
#include <fcntl.h>
#include <disks.h>
#include <linux/sizes.h>
#include <partitions.h>
#include <linux/math64.h>
#include <range.h>

static struct partition_desc *gpdesc;
static bool table_needs_write;
static const char *gunit_str = "KiB";
static uint64_t gunit = 1024;

struct unit {
	const char *str;
	uint64_t size;
};

static struct unit units[] = {
	{ .str = "B", .size = 1 },
	{ .str = "s", .size = 512 },
	{ .str = "KiB", .size = SZ_1K },
	{ .str = "MiB", .size = SZ_1M },
	{ .str = "GiB", .size = SZ_1G },
	{ .str = "TiB", .size = SZ_1T },
	{ .str = "KB", .size = 1000ULL },
	{ .str = "MB", .size = 1000ULL * 1000 },
	{ .str = "GB", .size = 1000ULL * 1000 * 1000 },
	{ .str = "TB", .size = 1000ULL * 1000 * 1000 * 1000 },
	{ .str = "k", .size = SZ_1K },
	{ .str = "K", .size = SZ_1K },
	{ .str = "M", .size = SZ_1M },
	{ .str = "G", .size = SZ_1G },
};

static int parted_strtoull(const char *str, uint64_t *val, uint64_t *mult)
{
	char *end;
	int i;

	*val = simple_strtoull(str, &end, 0);

	if (!*end) {
		*mult = 0;
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(units); i++) {
		if (!strcmp(end, units[i].str)) {
			*mult = units[i].size;
			return 0;
		}
	}

	printf("Error: Cannot read \"%s\" as number\n", str);

	return -EINVAL;
}

static struct partition_desc *pdesc_get(struct block_device *blk)
{
	if (gpdesc)
		return gpdesc;

	gpdesc = partition_table_read(blk);
	if (!gpdesc) {
		printf("Cannot read partition table\n");
		return NULL;
	}

	return gpdesc;
}

static int do_unit(struct block_device *blk, int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		printf("Error: missing unit\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(units); i++) {
		if (!strcmp(units[i].str, argv[1])) {
			gunit_str = units[i].str;
			gunit = units[i].size;
			return 2;
		}
	}

	printf("invalid unit: %s\n", argv[1]);

	return -EINVAL;
}

static int do_print(struct block_device *blk, int argc, char *argv[])
{
	struct partition_desc *pdesc;
	struct partition *part;

	pdesc = pdesc_get(blk);
	if (!pdesc) {
		printf("Error: Cannot get partition table from %s\n", blk->cdev.name);
		return -EINVAL;
	}

	printf("Disk /dev/%s: %s\n", blk->cdev.name,
	       size_human_readable(blk->num_blocks << SECTOR_SHIFT));
	printf("Partition Table: %s\n", pdesc->parser->name);

	printf("Number      Start           End          Size     Name\n");

	list_for_each_entry(part, &pdesc->partitions, list) {
		uint64_t start = part->first_sec << SECTOR_SHIFT;
		uint64_t size = part->size << SECTOR_SHIFT;
		uint64_t end = start + size - SECTOR_SIZE;

		printf(" %3d   %10llu%-3s %10llu%-3s %10llu%-3s  %-36s\n",
			part->num,
			div64_u64(start, gunit), gunit_str,
			div64_u64(end, gunit), gunit_str,
			div64_u64(size, gunit), gunit_str,
			part->name);
	}

	return 1;
}

static int do_mkpart(struct block_device *blk, int argc, char *argv[])
{
	struct partition_desc *pdesc;
	uint64_t start, end;
	const char *name, *fs_type;
	int ret;
	uint64_t mult;

	if (argc < 5) {
		printf("Error: Missing required arguments\n");
		return -EINVAL;
	}

	name = argv[1];
	fs_type = argv[2];

	ret = parted_strtoull(argv[3], &start, &mult);
	if (ret)
		return ret;

	ret = parted_strtoull(argv[4], &end, &mult);
	if (ret)
		return ret;

	if (!mult)
		mult = gunit;

	start *= mult;
	end *= mult;

	/* If not on sector boundaries move start up and end down */
	start = ALIGN(start, SZ_1M);
	end = ALIGN_DOWN(end, SZ_1M);

	/* convert to LBA */
	start >>= SECTOR_SHIFT;
	end >>= SECTOR_SHIFT;

	if (end == start) {
		printf("Error: After alignment the partition has zero size\n");
		return -EINVAL;
	}

	/*
	 * When unit is >= KB then substract one sector for user convenience.
	 * It allows to start the next partition where the previous ends
	 */
	if (mult >= 1000)
		end -= 1;

	pdesc = pdesc_get(blk);
	if (!pdesc)
		return -EINVAL;

	ret = partition_create(pdesc, name, fs_type, start, end);

	if (!ret)
		table_needs_write = true;

	return ret < 0 ? ret : 5;
}

static int do_mkpart_size(struct block_device *blk, int argc, char *argv[])
{
	struct partition_desc *pdesc;
	uint64_t size, start;
	const char *name, *fs_type;
	int ret;
	uint64_t mult;

	if (argc < 4) {
		printf("Error: Missing required arguments\n");
		return -EINVAL;
	}

	name = argv[1];
	fs_type = argv[2];

	ret = parted_strtoull(argv[3], &size, &mult);
	if (ret)
		return ret;

	if (!mult)
		mult = gunit;

	size *= mult;

	/* If not on sector boundaries move start up and end down */
	size = ALIGN(size, PARTITION_ALIGN_SIZE);

	/* convert to LBA */
	size >>= SECTOR_SHIFT;

	pdesc = pdesc_get(blk);
	if (!pdesc)
		return -EINVAL;

	ret = partition_find_free_space(pdesc, size, &start);
	if (ret) {
		printf("No free space for %llu sectors found\n", size);
		return ret;
	}

	printf("%s: creating partition with %llu blocks at %llu\n", __func__, size, start);

	ret = partition_create(pdesc, name, fs_type, start, start + size - 1);

	if (!ret)
		table_needs_write = true;

	return ret < 0 ? ret : 4;
}

static int do_rmpart(struct block_device *blk, int argc, char *argv[])
{
	struct partition_desc *pdesc;
	unsigned long num;
	int ret;

	if (argc < 2) {
		printf("Error: Expecting a partition number.\n");
		return -EINVAL;
	}

	ret = kstrtoul(argv[1], 0, &num);
	if (ret)
		return ret;

	pdesc = pdesc_get(blk);
	if (!pdesc)
		return -EINVAL;

	ret = partition_remove(pdesc, num);
	if (ret)
		return ret;

	table_needs_write = true;

	return 2;
}

static int do_mklabel(struct block_device *blk, int argc, char *argv[])
{
	struct partition_desc *pdesc;

	if (argc < 2) {
		printf("Error: Expecting a disk label type.\n");
		return -EINVAL;
	}

	pdesc = partition_table_new(blk, argv[1]);
	if (IS_ERR(pdesc)) {
		printf("Error: Cannot create partition table: %pe\n", pdesc);
		return PTR_ERR(pdesc);
	}

	table_needs_write = true;

	if (gpdesc)
		partition_table_free(gpdesc);
	gpdesc = pdesc;

	return 2;
}

static int do_refresh(struct block_device *blk, int argc, char *argv[])
{
	struct partition_desc *pdesc;

	pdesc = pdesc_get(blk);
	if (!pdesc)
		return -EINVAL;

	table_needs_write = true;

	return 1;
}

struct parted_command {
	const char *name;
	int (*command)(struct block_device *blk, int argc, char *argv[]);
};

struct parted_command parted_commands[] = {
	{
		.name = "mkpart",
		.command = do_mkpart,
	}, {
		.name = "mkpart_size",
		.command = do_mkpart_size,
	},  {
		.name = "print",
		.command = do_print,
	}, {
		.name = "rm",
		.command = do_rmpart,
	}, {
		.name = "mklabel",
		.command = do_mklabel,
	}, {
		.name = "unit",
		.command = do_unit,
	}, {
		.name = "refresh",
		.command = do_refresh,
	},
};

static int parted_run_command(struct block_device *blk, int argc, char *argv[])
{
	int i;

	for (i = 0; i < ARRAY_SIZE(parted_commands); i++) {
		struct parted_command *cmd = &parted_commands[i];

		if (!strcmp(argv[0], cmd->name))
			return cmd->command(blk, argc, argv);
	}

	printf("No such command: %s\n", argv[0]);

	return COMMAND_ERROR;
}

static int do_parted(int argc, char *argv[])
{
	struct cdev *cdev;
	struct block_device *blk;
	int ret = 0;

	table_needs_write = false;
	gpdesc = NULL;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	cdev = cdev_open_by_name(argv[1], O_RDWR);
	if (!cdev) {
		printf("Cannot open %s\n", argv[1]);
		return COMMAND_ERROR;
	}

	blk = cdev_get_block_device(cdev);
	if (!blk) {
		ret = -EINVAL;
		goto err;
	}

	argc -= 2;
	argv += 2;

	while (argc) {
		debug("---> run command %s\n", argv[0]);
		ret = parted_run_command(blk, argc, argv);
		if (ret < 0)
			break;

		argc -= ret;
		argv += ret;

		ret = 0;
	}

	if (!ret && gpdesc && table_needs_write)
		ret = partition_table_write(gpdesc);

err:
	if (gpdesc)
		partition_table_free(gpdesc);

	cdev_close(cdev);

	return ret;
}

BAREBOX_CMD_HELP_START(parted)
BAREBOX_CMD_HELP_TEXT("parted is a partition manipulation program with a behaviour similar to")
BAREBOX_CMD_HELP_TEXT("GNU Parted")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("commands:")
BAREBOX_CMD_HELP_OPT ("print", "print partitions")
BAREBOX_CMD_HELP_OPT ("mklabel <type>", "create a new partition table")
BAREBOX_CMD_HELP_OPT ("rm <num>", "remove a partition")
BAREBOX_CMD_HELP_OPT ("mkpart <name> <fstype> <start> <end>", "create a new partition")
BAREBOX_CMD_HELP_OPT ("mkpart_size <name> <fstype> <size>", "create a new partition")
BAREBOX_CMD_HELP_OPT ("unit <unit>", "change display/input units")
BAREBOX_CMD_HELP_OPT ("refresh", "refresh a partition table")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("<unit> can be one of \"s\" (sectors), \"B\" (bytes), \"kB\", \"MB\", \"GB\", \"TB\",")
BAREBOX_CMD_HELP_TEXT("\"KiB\", \"MiB\", \"GiB\" or \"TiB\"")
BAREBOX_CMD_HELP_TEXT("<type> must be \"gpt\" or \"msdos\"")
BAREBOX_CMD_HELP_TEXT("<fstype> can be one of  \"ext2\", \"ext3\", \"ext4\", \"fat16\", \"fat32\" or \"bbenv\"")
BAREBOX_CMD_HELP_TEXT("<name> for MBR partition tables can be one of \"primary\", \"extended\" or")
BAREBOX_CMD_HELP_TEXT("\"logical\". For GPT this is a name string.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(parted)
	.cmd            = do_parted,
	BAREBOX_CMD_DESC("edit partition tables")
	BAREBOX_CMD_OPTS("<device> [command [options...]...]")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_parted_help)
BAREBOX_CMD_END

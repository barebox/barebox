// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <command.h>
#include <device-mapper.h>
#include <libfile.h>
#include <stdio.h>

static struct dm_device *dmsetup_find(const char *name)
{
	struct dm_device *dm;

	dm = dm_find_by_name(name);
	if (IS_ERR_OR_NULL(dm)) {
		printf("Found no device named \"%s\"\n", name);
		return NULL;
	}

	return dm;
}

static int dmsetup_remove(int argc, char *argv[])
{
	struct dm_device *dm;

	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	dm = dmsetup_find(argv[0]);
	if (!dm)
		return COMMAND_ERROR;

	dm_destroy(dm);

	printf("Removed %s\n", argv[0]);
	return COMMAND_SUCCESS;
}

static int dmsetup_info_one(struct dm_device *dm, void *_n)
{
	int *n = _n;
	char *str;

	if (*n)
		putchar('\n');

	str = dm_asprint(dm);
	puts(str);
	free(str);
	(*n)++;
	return 0;
}

static int dmsetup_info(int argc, char *argv[])
{
	struct dm_device *dm;
	int n = 0;

	if (argc == 0) {
		dm_foreach(dmsetup_info_one, &n);
		if (n == 0)
			printf("No devices found\n");
		return COMMAND_SUCCESS;
	}

	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	dm = dmsetup_find(argv[0]);
	if (!dm)
		return COMMAND_ERROR;

	dmsetup_info_one(dm, &n);
	return COMMAND_SUCCESS;
}


static int dmsetup_create(int argc, char *argv[])
{
	struct dm_device *dm;
	char *table;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	table = read_file(argv[1], NULL);
	if (!table) {
		printf("Failed to read table from %s: %m\n", argv[1]);
		return COMMAND_ERROR;
	}

	dm = dm_create(argv[0], table);
	free(table);
	if (IS_ERR_OR_NULL(dm)) {
		printf("Failed to create %s: %pe\n", argv[0], dm);
		return COMMAND_ERROR;
	}

	printf("Created %s\n", argv[0]);
	return COMMAND_SUCCESS;
}

static int do_dmsetup(int argc, char *argv[])
{
	const char *cmd;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	cmd = argv[1];
	argc -= 2;
	argv += 2;

	if (!strcmp(cmd, "create"))
		return dmsetup_create(argc, argv);
	else if (!strcmp(cmd, "info"))
		return dmsetup_info(argc, argv);
	else if (!strcmp(cmd, "remove"))
		return dmsetup_remove(argc, argv);

	printf("Unknown command: %s\n", cmd);
	return -EINVAL;
}

BAREBOX_CMD_HELP_START(dmsetup)
BAREBOX_CMD_HELP_TEXT("dmsetup - low level interface to the device mapper")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Compose virtual block devices from a table of mappings from")
BAREBOX_CMD_HELP_TEXT("logical block addresses to various data sources, such as")
BAREBOX_CMD_HELP_TEXT("linear ranges from other existing devices.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("commands:")
BAREBOX_CMD_HELP_OPT("create <name> <table-file>", "Create new device")
BAREBOX_CMD_HELP_OPT("info [<name>]", "Show device information")
BAREBOX_CMD_HELP_OPT("remove <name>", "Remove device")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dmsetup)
	.cmd = do_dmsetup,
	BAREBOX_CMD_DESC("low level interface to the device mapper")
	BAREBOX_CMD_OPTS("<command> [args...]")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_dmsetup_help)
BAREBOX_CMD_END

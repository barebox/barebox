// SPDX-License-Identifier: GPL-2.0-only

#include <command.h>
#include <malloc.h>
#include <stdlib.h>
#include <partitions.h>
#include <driver.h>
#include <disks.h>
#include <xfuncs.h>
#include <getopt.h>
#include <efi/partition.h>
#include <bootsource.h>
#include <linux/kstrtox.h>
#include <fcntl.h>
#include <stdio.h>
#include <envfs.h>

static int do_createnv(int argc, char *argv[])
{
	uint64_t start, size = SZ_1M;
	struct cdev *cdev;
	struct block_device *blk;
	struct partition *part;
	struct partition_desc *pdesc = NULL;
	enum filetype filetype;
	guid_t partition_barebox_env_guid = PARTITION_BAREBOX_ENVIRONMENT_GUID;
	void *buf = NULL;
	bool force = false;
	int opt, ret;

	while ((opt = getopt(argc, argv, "s:f")) > 0) {
		switch (opt) {
		case 's':
			size = strtoull_suffix(optarg, NULL, 0);
			break;
		case 'f':
			force = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
        }

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		cdev = bootsource_of_cdev_find();
		if (!cdev) {
			printf("Cannot find bootsource cdev\n");
			return COMMAND_ERROR;
		}
		ret = cdev_open(cdev, O_RDWR);
		if (ret) {
			printf("Cannot open %s: %pe\n", cdev->name, ERR_PTR(ret));
			return COMMAND_ERROR;
		}
	} else {
		cdev = cdev_open_by_path_name(argv[0], O_RDWR);
		if (!cdev) {
			printf("Cannot open %s\n", argv[0]);
			return COMMAND_ERROR;
		}
	}

	blk = cdev_get_block_device(cdev);
	if (!blk) {
		ret = -ENOENT;
		goto err;
	}

	buf = xzalloc(2 * SECTOR_SIZE);

	ret = cdev_read(cdev, buf, 2 * SECTOR_SIZE, 0, 0);
	if (ret < 0) {
		printf("Cannot read from %s: %pe\n", cdev->name, ERR_PTR(ret));
		goto err;
	}

	filetype = file_detect_partition_table(buf, 2 * SECTOR_SIZE);

	switch (filetype) {
	case filetype_gpt:
		pdesc = partition_table_read(blk);
		if (!pdesc) {
			printf("Cannot read partition table\n");
			ret = -EINVAL;
			goto err;
		}
		break;
	case filetype_mbr:
		printf("%s contains a DOS partition table. Cannot create a barebox environment here\n",
		       cdev->name);
		ret = -EINVAL;
		goto err;
	default:
		printf("Will create a GPT on %s\n", cdev->name);
		pdesc = partition_table_new(blk, "gpt");
		break;
	}

	list_for_each_entry(part, &pdesc->partitions, list) {
		if (guid_equal(&part->typeuuid, &partition_barebox_env_guid)) {
			printf("%s already contains a barebox environment partition\n",
			       cdev->name);
			ret = -EINVAL;
			goto err;
		}
	}

	size >>= SECTOR_SHIFT;

	ret = partition_find_free_space(pdesc, size, &start);
	if (ret) {
		printf("No free space available\n");
		goto err;
	}

	ret = partition_create(pdesc, "barebox-environment", "bbenv", start, start + size - 1);
	if (ret) {
		printf("Creating partition failed: %pe\n", ERR_PTR(ret));
		goto err;
	}

	printf("Will create a barebox environment partition of size %llu bytes on %s\n",
	       size << SECTOR_SHIFT, cdev->name);

	if (!force) {
		char c;

		printf("Do you wish to continue? (y/n)\n");

		c = getchar();
		if (c != 'y') {
			ret = -EINTR;
			goto err;
		}
	}

	ret = partition_table_write(pdesc);
	if (ret) {
		printf("Cannot write partition table: %pe\n", ERR_PTR(ret));
		goto err;
	}

	if (!default_environment_path_get()) {
		char *path = xasprintf("/dev/%s.barebox-environment", cdev->name);
		default_environment_path_set(path);
		free(path);
	}

	printf("Created barebox environment partition on %s\n", cdev->name);

	ret = 0;
err:
	if (pdesc)
		partition_table_free(pdesc);

	free(buf);
	cdev_close(cdev);

	return ret ? COMMAND_ERROR : 0;
}

BAREBOX_CMD_HELP_START(createnv)
BAREBOX_CMD_HELP_TEXT("Create a barebox environment partition.")
BAREBOX_CMD_HELP_TEXT("This creates a barebox environment partition and eventually")
BAREBOX_CMD_HELP_TEXT("a GPT partition table when it does not exist.")
BAREBOX_CMD_HELP_OPT("-s <size>", "specify partition size (default 1MiB)")
BAREBOX_CMD_HELP_OPT("-f\t", "force. Do not ask")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(createnv)
        .cmd            = do_createnv,
        BAREBOX_CMD_DESC("Create a barebox environment partition")
        BAREBOX_CMD_OPTS("[-sf] [device]")
        BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
        BAREBOX_CMD_HELP(cmd_createnv_help)
BAREBOX_CMD_END

/*
 * partition.c - parse a linux-like mtd partition definition and register
 *               the new partitions
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef CONFIG_ENABLE_PARTITION_NOISE
# define DEBUG
#endif

#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <partition.h>
#include <errno.h>
#include <xfuncs.h>

static int dev_del_partitions(struct device_d *physdev)
{
	struct device_d *child, *tmp;
	int ret;

	device_for_each_child_safe(physdev, tmp, child) {
		struct partition *part = child->type_data;
	
		debug("delete partition: %s\n", child->id);

		if (part->flags & PARTITION_FIXED)
			continue;

		ret = unregister_device(child);
		if (ret) {
			printf("delete partition `%s' failed: %s\n", child->id, errno_str());
			return errno;
		}

		free(part);
	}

	return 0;
}

static int mtd_part_do_parse_one(struct partition *part, const char *str,
				 char **endp)
{
	ulong size;
	char *end;
	char buf[MAX_DRIVER_NAME];

	memset(buf, 0, MAX_DRIVER_NAME);

	if (*str == '-') {
		size = part->physdev->size - part->offset;
		end = (char *)str + 1;
	} else {
		size = strtoul_suffix(str, &end, 0);
	}

	if (size + part->offset > part->physdev->size) {
		printf("partition end is beyond device\n");
		return -EINVAL;
	}

	str = end;

	if (*str == '(') {
		str++;
		end = strchr(str, ')');
		if (!end) {
			printf("could not find matching ')'\n");
			return -EINVAL;
		}
		if (end - str >= MAX_DRIVER_NAME) {
			printf("device name too long\n");
			return -EINVAL;
		}

		memcpy(part->name, str, end - str);
		end++;
	}

	str = end;

	if (*str == 'r' && *(str + 1) == 'o') {
		part->flags |= PARTITION_READONLY;
		end = (char *)(str + 2);
	}

	if (endp)
		*endp = end;

	strcpy(part->device.name, "partition");
	part->device.size = size;

	return 0;
}

static int do_addpart(cmd_tbl_t * cmdtp, int argc, char *argv[])
{
	struct partition *part;
	struct device_d *dev;
	char *endp;
	int num = 0;
	unsigned long offset;

	if (argc != 3) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	dev = get_device_by_path(argv[1]);
	if (!dev) {
		printf("no such device: %s\n", argv[1]);
		return 1;
	}

	dev_del_partitions(dev);

	offset = 0;

	endp = argv[2];

	while (1) {
		part = xzalloc(sizeof(struct partition));

		part->offset = offset;
		part->physdev = dev;
		part->num = num;
		part->device.map_base = dev->map_base + offset;

		if (mtd_part_do_parse_one(part, endp, &endp)) {
			dev_del_partitions(dev);
			goto free_out;
		}

		offset += part->device.size;

		part->device.type_data = part;

		sprintf(part->device.id, "%s.%s", dev->id, part->name);
		if (register_device(&part->device))
			goto free_out;

		dev_add_child(dev, &part->device);

		num++;

		if (!*endp)
			break;

		if (*endp != ',') {
			printf("parse error\n");
			goto err_out;
		}
		endp++;
	}

	return 0;

free_out:
	free(part);
err_out:
	return 1;
}

static __maybe_unused char cmd_addpart_help[] =
"Usage: addpart <device> <partition description>\n"
"addpart adds a partition description to a device. The partition description\n"
"has the form\n"
"size1(name1)[ro],size2(name2)[ro],...\n"
"<device> is the device name under. Size can be given in decimal or if prefixed\n"
"with 0x in hex. Sizes can have an optional suffix K,M,G. The size of the last\n"
"partition can be specified as '-' for the remaining space of the device.\n"
"This format is the same as used in the Linux kernel for cmdline mtd partitions.\n"
"Note That this command has to be reworked and will probably change it's API.";

U_BOOT_CMD_START(addpart)
	.maxargs = 3,
	.cmd = do_addpart,
	.usage = "add a partition table to a device",
	U_BOOT_CMD_HELP(cmd_addpart_help)
U_BOOT_CMD_END

static int do_delpart(cmd_tbl_t * cmdtp, int argc, char *argv[])
{
	struct device_d *dev;

	if (argc != 2) {
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	dev = get_device_by_path(argv[1]);
	if (!dev) {
		printf("no such device: %s\n", argv[1]);
		return 1;
	}

	dev_del_partitions(dev);

	return 0;
}

static __maybe_unused char cmd_delpart_help[] =
"Usage: delpart <device>\n"
"Delete partitions previously added to a device with addpart.\n";

U_BOOT_CMD_START(delpart)
	.maxargs = 2,
	.cmd = do_delpart,
	.usage = "delete a partition table from a device",
	U_BOOT_CMD_HELP(cmd_delpart_help)
U_BOOT_CMD_END


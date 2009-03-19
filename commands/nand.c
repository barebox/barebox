/*
 * Copyright (c) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <xfuncs.h>
#include <init.h>
#include <ioctl.h>
#include <nand.h>
#include <linux/mtd/mtd-abi.h>
#include <fcntl.h>

struct nand_bb {
	struct device_d device;

	struct device_d *physdev;

	int open;

	struct mtd_info_user info;

	off_t offset;
};

static ssize_t nand_bb_read(struct device_d *dev, void *buf, size_t count,
	unsigned long offset, ulong flags)
{
	struct nand_bb *bb = dev->priv;
	int ret, bytes = 0, now;

	debug("%s %d %d\n", __func__, offset, count);

	while(count) {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			debug("skipping bad block at 0x%08x\n", bb->offset);
			bb->offset += bb->info.erasesize;
		}

		now = min(count, (size_t)(bb->info.erasesize -
				(bb->offset % bb->info.erasesize)));
		ret = dev_read(bb->physdev, buf, now, bb->offset, flags);
		if (ret < 0)
			return ret;
		buf += now;
		count -= now;
		bb->offset += now;
		bytes += now;
	};

	return bytes;
}

static ssize_t nand_bb_write(struct device_d *dev, const void *buf, size_t count,
	unsigned long offset, ulong flags)
{
	struct nand_bb *bb = dev->priv;
	int ret, bytes = 0, now;

	debug("%s %d %d\n", __func__, offset, count);

	while(count) {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			debug("skipping bad block at 0x%08x\n", bb->offset);
			bb->offset += bb->info.erasesize;
		}

		now = min(count, (size_t)(bb->info.erasesize -
				(bb->offset % bb->info.erasesize)));
		ret = dev_write(bb->physdev, buf, now, bb->offset, flags);
		if (ret < 0)
			return ret;
		buf += now;
		count -= now;
		bb->offset += now;
		bytes += now;
	};

	return bytes;
}

static int nand_bb_erase(struct device_d *dev, size_t count, unsigned long offset)
{
	struct nand_bb *bb = dev->priv;

	if (offset != 0 || count != dev->size) {
		printf("can only erase whole device\n");
		return -EINVAL;
	}

	return dev_erase(bb->physdev, dev->size, 0);
}

static int nand_bb_open(struct device_d *dev, struct filep *f)
{
	struct nand_bb *bb = dev->priv;

	if (bb->open)
		return -EBUSY;

	bb->open = 1;
	bb->offset = 0;

	return 0;
}

static int nand_bb_close(struct device_d *dev, struct filep *f)
{
	struct nand_bb *bb = dev->priv;

	bb->open = 0;

	return 0;
}

static int nand_bb_calc_size(struct nand_bb *bb)
{
	ulong pos = 0;
	int ret;

	while (pos < bb->physdev->size) {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)pos);
		if (ret < 0)
			return ret;
		if (!ret)
			bb->device.size += bb->info.erasesize;

		pos += bb->info.erasesize;
	}

	return 0;
}

static int nand_bb_probe(struct device_d *dev)
{
	struct nand_bb *bb = dev->priv;
	int ret;

	ret = dev_ioctl(bb->physdev, MEMGETINFO, &bb->info);
	if (ret)
		return ret;
	return nand_bb_calc_size(bb);
}

static void nand_bb_remove(struct device_d *dev)
{
}

struct driver_d nand_bb_driver = {
	.name  	= "nand_bb",
	.probe 	= nand_bb_probe,
	.remove = nand_bb_remove,
	.open   = nand_bb_open,
	.close  = nand_bb_close,
	.read  	= nand_bb_read,
	.write 	= nand_bb_write,
	.erase	= nand_bb_erase,
	.type	= DEVICE_TYPE_NAND_BB,
};

static int nand_bb_init(void)
{
	return register_driver(&nand_bb_driver);
}

device_initcall(nand_bb_init);

/**
 * Add a bad block aware device ontop of another (NAND) device 
 * @param[in] dev The device to add a partition on
 * @param[in] name Partition name (can be obtained with devinfo command)
 * @return The device representing the new partition.
 */
struct device_d *dev_add_bb_dev(struct device_d *dev, const char *name)
{
	struct nand_bb *bb;

	bb = xzalloc(sizeof(*bb));

	if (name)
		strcpy(bb->device.id, name);
	else
		sprintf(bb->device.id, "%s.bb", dev->id);
	strcpy(bb->device.name, "nand_bb");
	bb->device.priv = bb;
	bb->device.size = 0;
	bb->device.type	= DEVICE_TYPE_NAND_BB;
	bb->physdev = dev;

	if (register_device(&bb->device))
		goto free_out;

	dev_add_child(dev, &bb->device);

	return &bb->device;

free_out:
	free(bb);
	return 0;
}

#define NAND_ADD (1 << 0)
#define NAND_DEL (1 << 1)
#define NAND_MARKBAD (1 << 2)

static int do_nand (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int opt;
	struct device_d *dev;
	struct nand_bb *bb;
	int command = 0, badblock = 0;

	getopt_reset();

	while((opt = getopt(argc, argv, "adb:")) > 0) {
		if (command) {
			printf("only one command may be given\n");
			return 1;
		}

		switch (opt) {
		case 'a':
			command = NAND_ADD;
			break;
		case 'd':
			command = NAND_DEL;
			break;
		case 'b':
			command = NAND_MARKBAD;
			badblock = simple_strtoul(optarg, NULL, 0);
		}
	}

	if (command & NAND_ADD) {
		while (optind < argc) {
			dev = get_device_by_path(argv[optind]);
			if (!dev) {
				printf("no such device: %s\n", argv[optind]);
				return 1;
			}

			if (!dev_add_bb_dev(dev, NULL))
				return 1;

			optind++;
		}
	}

	if (command & NAND_DEL) {
		while (optind < argc) {
			dev = get_device_by_path(argv[optind]);
			if (!dev) {
				printf("no such device: %s\n", argv[optind]);
				return 1;
			}

			if (dev->type != DEVICE_TYPE_NAND_BB) {
				printf("not a nand bb device: %s\n", dev);
				return 1;
			}
			bb = dev->priv;
			unregister_device(dev);
			free(bb);
			optind++;
		}
	}

	if (command & NAND_MARKBAD) {
		if (optind < argc) {
			int ret = 0, fd;

			printf("marking block at 0x%08x on %s as bad\n", badblock, argv[optind]);

			fd = open(argv[optind], O_RDWR);
			if (fd < 0) {
				perror("open");
				return 1;
			}

			ret = ioctl(fd, MEMSETBADBLOCK, (void *)badblock);
			if (ret)
				perror("ioctl");

			close(fd);
			return ret;
		}
	}

	return 0;
}

static const __maybe_unused char cmd_nand_help[] =
"Usage: nand [OPTION]...\n"
"nand related commands\n"
"  -a  <dev>  register a bad block aware device ontop of a normal nand device\n"
"  -d  <dev>  deregister a bad block aware device\n"
"  -b  <ofs> <dev> mark block at offset ofs as bad\n";

U_BOOT_CMD_START(nand)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_nand,
	.usage		= "",
	U_BOOT_CMD_HELP(cmd_nand_help)
U_BOOT_CMD_END

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
	int ret;

	debug("%s %d %d\n", __func__, offset, count);

	do {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			printf("skipping bad block at 0x%08x\n", bb->offset);
			bb->offset += bb->info.erasesize;
		}
	} while (ret);

	ret = dev_read(bb->physdev, buf, count, bb->offset, flags);
	if (ret > 0)
		bb->offset += ret;

	return ret;
}

static ssize_t nand_bb_write(struct device_d *dev, const void *buf, size_t count,
	unsigned long offset, ulong flags)
{
	struct nand_bb *bb = dev->priv;
	int ret;

	debug("%s %d %d\n", __func__, offset, count);

	do {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			printf("skipping bad block at 0x%08x\n", bb->offset);
			bb->offset += bb->info.erasesize;
		}
	} while (ret);

	ret = dev_write(bb->physdev, buf, count, bb->offset, flags);
	if (ret > 0)
		bb->offset += ret;

	return ret;
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

static int nand_bb_probe(struct device_d *dev)
{
	struct nand_bb *bb = dev->priv;
	int ret = 0;

	ret = dev_ioctl(bb->physdev, MEMGETINFO, &bb->info);
	if (ret < 0)
		goto out;

	return 0;
out:
	free(bb);
	return ret;
}

static int nand_bb_remove(struct device_d *dev)
{
	return 0;
}

struct driver_d nand_bb_driver = {
	.name  	= "nand_bb",
	.probe 	= nand_bb_probe,
	.remove = nand_bb_remove,
	.open   = nand_bb_open,
	.close  = nand_bb_close,
	.read  	= nand_bb_read,
	.write 	= nand_bb_write,
	.type	= DEVICE_TYPE_NAND_BB,
};

static int nand_bb_init(void)
{
	return register_driver(&nand_bb_driver);
}

device_initcall(nand_bb_init);

static int do_nand (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int opt;
	char *add_device = NULL, *del_device = NULL;
	struct device_d *dev;
	struct nand_bb *bb;

	getopt_reset();

	while((opt = getopt(argc, argv, "a:d:")) > 0) {
		switch(opt) {
		case 'a':
			add_device = optarg;
			break;
		case 'd':
			del_device = optarg;
			break;
		}
	}

	if (add_device) {
		dev = get_device_by_path(add_device);
		if (!dev) {
			printf("no such device: %s\n", add_device);
			return 1;
		}

		if (dev->type != DEVICE_TYPE_NAND) {
			printf("not a nand device: %s\n", add_device);
			return 1;
		}

		bb = xzalloc(sizeof(*bb));
		sprintf(bb->device.id, "%s.bb", dev->id);
		strcpy(bb->device.name, "nand_bb");
		bb->device.priv = bb;
		bb->device.size = dev->size; /* FIXME: Bad blocks? */
		bb->device.type	= DEVICE_TYPE_NAND_BB;
		bb->physdev = dev;

		if (register_device(&bb->device))
			goto free_out;

		dev_add_child(dev, &bb->device);
	}

	if (del_device) {
		dev = get_device_by_path(del_device);
		if (!dev) {
			printf("no such device: %s\n", del_device);
			return 1;
		}

		if (dev->type != DEVICE_TYPE_NAND_BB) {
			printf("not a nand bb device: %s\n", dev);
			return 1;
		}

		bb = dev->priv;
		unregister_device(dev);
		free(bb);
	}

free_out:
	return 0;
}

static const __maybe_unused char cmd_nand_help[] =
"Usage: nand [OPTION]...\n"
"nand related commands\n"
"  -a  <dev>  register a bad block aware device ontop of a normal nand device\n"
"  -d  <dev>  deregister a bad block aware device\n";

U_BOOT_CMD_START(nand)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_nand,
	.usage		= "",
	U_BOOT_CMD_HELP(cmd_nand_help)
U_BOOT_CMD_END

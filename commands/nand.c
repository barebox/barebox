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

	printf("%s %d %d\n", __func__, offset, count);

	do {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			printf("block is bad\n");
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

	printf("%s %d %d\n", __func__, offset, count);

	do {
		ret = dev_ioctl(bb->physdev, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			printf("block is bad\n");
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
	int ret;

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

	printf("new info: %d\n", bb->info.erasesize);

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
};

static int nand_bb_init(void)
{
	return register_driver(&nand_bb_driver);
}

device_initcall(nand_bb_init);

static int do_nand (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int ret, opt;
	char *device = NULL;
	struct device_d *dev;
	struct nand_bb *bb;

	getopt_reset();

	while((opt = getopt(argc, argv, "m:")) > 0) {
		switch(opt) {
		case 'm':
			device = optarg;
			break;
		}
	}

	if (device) {
		dev = get_device_by_path(device);
		if (!dev) {
			printf("no such device: %s\n", argv[1]);
			return 1;
		}

		bb = xzalloc(sizeof(*bb));
		sprintf(bb->device.id, "%s.bb", dev->id);
		strcpy(bb->device.name, "nand_bb");
		bb->device.priv = bb;
		bb->device.size = dev->size; /* FIXME: Bad blocks? */
		bb->physdev = dev;

		if (register_device(&bb->device))
			goto free_out;

		dev_add_child(dev, &bb->device);

	}
free_out:
	return 0;
}

static const __maybe_unused char cmd_nand_help[] =
"Usage: nand bla\n";

U_BOOT_CMD_START(nand)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_nand,
	.usage		= "",
	U_BOOT_CMD_HELP(cmd_nand_help)
U_BOOT_CMD_END

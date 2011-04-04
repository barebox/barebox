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
#include <libgen.h>

struct nand_bb {
	char *devname;
	char *name;
	int open;
	int needs_write;

	struct mtd_info_user info;

	size_t raw_size;
	size_t size;
	int fd;
	off_t offset;
	void *writebuf;

	struct cdev cdev;
};

static ssize_t nand_bb_read(struct cdev *cdev, void *buf, size_t count,
	unsigned long offset, ulong flags)
{
	struct nand_bb *bb = cdev->priv;
	int ret, bytes = 0, now;

	debug("%s %d %d\n", __func__, offset, count);

	while(count) {
		ret = ioctl(bb->fd, MEMGETBADBLOCK, (void *)bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			printf("skipping bad block at 0x%08lx\n", bb->offset);
			bb->offset += bb->info.erasesize;
			continue;
		}

		now = min(count, (size_t)(bb->info.erasesize -
				(bb->offset % bb->info.erasesize)));
		lseek(bb->fd, bb->offset, SEEK_SET);
		ret = read(bb->fd, buf, now);
		if (ret < 0)
			return ret;
		buf += now;
		count -= now;
		bb->offset += now;
		bytes += now;
	};

	return bytes;
}

/* Must be a multiple of the largest NAND page size */
#define BB_WRITEBUF_SIZE	4096

#ifdef CONFIG_NAND_WRITE
static int nand_bb_write_buf(struct nand_bb *bb, size_t count)
{
	int ret, now;
	void *buf = bb->writebuf;
	int cur_ofs = bb->offset & ~(BB_WRITEBUF_SIZE - 1);

	while (count) {
		ret = ioctl(bb->fd, MEMGETBADBLOCK, (void *)cur_ofs);
		if (ret < 0)
			return ret;

		if (ret) {
			debug("skipping bad block at 0x%08x\n", cur_ofs);
			bb->offset += bb->info.erasesize;
			cur_ofs += bb->info.erasesize;
			continue;
		}

		now = min(count, (size_t)(bb->info.erasesize));
		lseek(bb->fd, cur_ofs, SEEK_SET);
		ret = write(bb->fd, buf, now);
		if (ret < 0)
			return ret;
		buf += now;
		count -= now;
		cur_ofs += now;
	};

	return 0;
}

static ssize_t nand_bb_write(struct cdev *cdev, const void *buf, size_t count,
	unsigned long offset, ulong flags)
{
	struct nand_bb *bb = cdev->priv;
	int bytes = count, now, wroffs, ret;

	debug("%s offset: 0x%08x count: 0x%08x\n", __func__, offset, count);

	while (count) {
		wroffs = bb->offset % BB_WRITEBUF_SIZE;
		now = min((int)count, BB_WRITEBUF_SIZE - wroffs);
		memcpy(bb->writebuf + wroffs, buf, now);

		if (wroffs + now == BB_WRITEBUF_SIZE) {
			bb->needs_write = 0;
			ret = nand_bb_write_buf(bb, BB_WRITEBUF_SIZE);
			if (ret)
				return ret;
		} else {
			bb->needs_write = 1;
		}

		bb->offset += now;
		count -= now;
		buf += now;
	}

	return bytes;
}

static int nand_bb_erase(struct cdev *cdev, size_t count, unsigned long offset)
{
	struct nand_bb *bb = cdev->priv;

	if (offset != 0) {
		printf("can only erase from beginning of device\n");
		return -EINVAL;
	}

	lseek(bb->fd, 0, SEEK_SET);

	return erase(bb->fd, bb->raw_size, 0);
}
#endif

static int nand_bb_open(struct cdev *cdev, struct filep *f)
{
	struct nand_bb *bb = cdev->priv;

	if (bb->open)
		return -EBUSY;

	bb->open = 1;
	bb->offset = 0;
	bb->needs_write = 0;
	bb->writebuf = xmalloc(BB_WRITEBUF_SIZE);

	return 0;
}

static int nand_bb_close(struct cdev *cdev, struct filep *f)
{
	struct nand_bb *bb = cdev->priv;

#ifdef CONFIG_NAND_WRITE
	if (bb->needs_write)
		nand_bb_write_buf(bb, bb->offset % BB_WRITEBUF_SIZE);
#endif
	bb->open = 0;
	free(bb->writebuf);

	return 0;
}

static int nand_bb_calc_size(struct nand_bb *bb)
{
	ulong pos = 0;
	int ret;

	while (pos < bb->raw_size) {
		ret = ioctl(bb->fd, MEMGETBADBLOCK, (void *)pos);
		if (ret < 0)
			return ret;
		if (!ret)
			bb->cdev.size += bb->info.erasesize;

		pos += bb->info.erasesize;
	}

	return 0;
}

static struct file_operations nand_bb_ops = {
	.open   = nand_bb_open,
	.close  = nand_bb_close,
	.read  	= nand_bb_read,
#ifdef CONFIG_NAND_WRITE
	.write 	= nand_bb_write,
	.erase	= nand_bb_erase,
#endif
};

/**
 * Add a bad block aware device ontop of another (NAND) device 
 * @param[in] dev The device to add a partition on
 * @param[in] name Partition name (can be obtained with devinfo command)
 * @return The device representing the new partition.
 */
int dev_add_bb_dev(char *path, const char *name)
{
	struct nand_bb *bb;
	int ret = -ENOMEM;
	struct stat s;

	bb = xzalloc(sizeof(*bb));
	bb->devname = asprintf("/dev/%s", basename(path));
	if (!bb->devname)
		goto out1;

	if (name)
		bb->cdev.name = strdup(name);
	else
		bb->cdev.name = asprintf("%s.bb", basename(path));

	if (!bb->cdev.name)
		goto out2;

	ret = stat(bb->devname, &s);
	if (ret)
		goto out3;

	bb->raw_size = s.st_size;

	bb->fd = open(bb->devname, O_RDWR);
	if (bb->fd < 0) {
		ret = -ENODEV;
		goto out3;
	}

	ret = ioctl(bb->fd, MEMGETINFO, &bb->info);
	if (ret)
		goto out4;

	nand_bb_calc_size(bb);
	bb->cdev.ops = &nand_bb_ops;
	bb->cdev.priv = bb;

	devfs_create(&bb->cdev);

	return 0;

out4:
	close(bb->fd);
out3:
	free(bb->cdev.name);
out2:
	free(bb->devname);
out1:
	free(bb);
	return ret;
}

#define NAND_ADD (1 << 0)
#define NAND_DEL (1 << 1)
#define NAND_MARKBAD (1 << 2)

static int do_nand(struct command *cmdtp, int argc, char *argv[])
{
	int opt;
	struct nand_bb *bb;
	int command = 0, badblock = 0;

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
			if (dev_add_bb_dev(argv[optind], NULL))
				return 1;

			optind++;
		}
	}

	if (command & NAND_DEL) {
		while (optind < argc) {
			struct cdev *cdev;

			cdev = cdev_by_name(basename(argv[optind]));
			if (!cdev) {
				printf("no such device: %s\n", argv[optind]);
				return 1;
			}
			bb = cdev->priv;
			close(bb->fd);
			devfs_remove(cdev);
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

BAREBOX_CMD_START(nand)
	.cmd		= do_nand,
	.usage		= "NAND specific handling",
	BAREBOX_CMD_HELP(cmd_nand_help)
BAREBOX_CMD_END

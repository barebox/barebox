/*
 * imx-bbu-external-nand.c - i.MX specific update functions for external
 *			     nand boot
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 */

#include <common.h>
#include <malloc.h>
#include <bbu.h>
#include <filetype.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/sizes.h>
#include <linux/mtd/mtd-abi.h>
#include <linux/stat.h>
#include <ioctl.h>
#include <mach/bbu.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm-head.h>

static int imx_bbu_external_nand_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct mtd_info_user meminfo;
	int fd;
	struct stat s;
	int size_available, size_need;
	int ret;
	uint32_t num_bb = 0, bbt = 0;
	uint64_t offset = 0;
	int block = 0, len, now, blocksize;
	void *image = data->image;

	ret = stat(data->devicefile, &s);
	if (ret)
		return ret;

	size_available = s.st_size;

	fd = open(data->devicefile, O_RDWR);
	if (fd < 0)
		return fd;

	ret = ioctl(fd, MEMGETINFO, &meminfo);
	if (ret)
		goto out;

	blocksize = meminfo.erasesize;

	size_need = data->len;

	/*
	 * Collect bad blocks and construct BBT
	 */
	while (size_need > 0) {
		ret = ioctl(fd, MEMGETBADBLOCK, &offset);
		if (ret < 0)
			goto out;

		if (ret) {
			if (!offset) {
				printf("1st block is bad. This is not supported\n");
				ret = -EINVAL;
				goto out;
			}

			debug("bad block at 0x%08llx\n", offset);
			num_bb++;
			bbt |= (1 << block);
			offset += blocksize;
			block++;
			continue;
		}
		size_need -= blocksize;
		size_available -= blocksize;
		offset += blocksize;
		block++;

		if (size_available < 0) {
			printf("device is too small.\n"
					"raw partition size: 0x%08llx\n"
					"partition size w/o bad blocks: 0x%08llx\n"
					"size needed: 0x%08x\n",
					s.st_size,
					s.st_size - num_bb * blocksize,
					data->len);
			ret = -ENOSPC;
			goto out;
		}
	}

	debug("total image size: 0x%08x. Space needed including bad blocks: 0x%08x\n",
			data->len, data->len + num_bb * blocksize);

	if (meminfo.writesize >= 2048) {
		uint32_t *image_bbt = image + ARM_HEAD_SPARE_OFS;

		debug(">= 2k nand detected. Creating in-image bbt\n");

		if (*image_bbt != ARM_HEAD_SPARE_MARKER) {
			if (!bbu_force(data, "Cannot find space for the BBT")) {
				ret = -EINVAL;
				goto out;
			}
		} else {
			*image_bbt++ = IMX_NAND_BBT_MAGIC;
			*image_bbt++ = bbt;
		}
	}

	if (data->len + num_bb * blocksize > s.st_size) {
		printf("needed space (0x%08x) exceeds partition space (0x%08llx)\n",
				data->len + num_bb * blocksize, s.st_size);
		ret = -ENOSPC;
		goto out;
	}

	len = data->len;
	offset = 0;

	/* last chance before erasing the flash */
	ret = bbu_confirm(data);
	if (ret)
		return ret;

	/*
	 * Write image to NAND skipping bad blocks
	 */
	while (len > 0) {
		now = min(len, blocksize);

		ret = ioctl(fd, MEMGETBADBLOCK, &offset);
		if (ret < 0)
			goto out;

		if (ret) {
			ret = lseek(fd, offset + blocksize, SEEK_SET);
			if (ret < 0)
				goto out;
			offset += blocksize;
			continue;
		}

		debug("writing %d bytes at 0x%08llx\n", now, offset);

		ret = erase(fd, blocksize, offset);
		if (ret)
			goto out;

		ret = write(fd, image, now);
		if (ret < 0)
			goto out;

		len -= now;
		image += now;
		offset += now;
	}

	ret = 0;

out:
	close(fd);

	return ret;
}

/*
 * Register a i.MX external nand boot update handler.
 *
 * For 512b page NANDs this handler simply writes the image to NAND skipping
 * bad blocks.
 *
 * For 2K page NANDs this handler embeds a bad block table in the flashed image.
 * This is necessary since we rely on an on-flash BBT for these flashes, but the
 * regular mtd BBT is too complex to be handled in the 2k the i.MX is able to
 * initially load from NAND. The BBT consists of a single 32bit value in which
 * each bit represents a single block. With 2k NAND flashes this is enough for
 * 4MiB size including bad blocks.
 */
int imx_bbu_external_nand_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->name = name;
	handler->flags = flags;
	handler->handler = imx_bbu_external_nand_update;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}

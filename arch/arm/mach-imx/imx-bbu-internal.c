/*
 * imx-bbu-internal.c - i.MX specific update functions for internal boot
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#define IMX_INTERNAL_NAND_BBU

#include <common.h>
#include <malloc.h>
#include <bbu.h>
#include <filetype.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <sizes.h>
#include <linux/mtd/mtd-abi.h>
#include <linux/stat.h>
#include <ioctl.h>
#include <mach/bbu.h>
#include <mach/imx-flash-header.h>

#define FLASH_HEADER_OFFSET_MMC		0x400

#define IMX_INTERNAL_FLAG_NAND		(1 << 0)
#define IMX_INTERNAL_FLAG_KEEP_DOSPART	(1 << 1)
#define IMX_INTERNAL_FLAG_ERASE		(1 << 2)

struct imx_internal_bbu_handler {
	struct bbu_handler handler;
	const void *dcd;
	int dcdsize;
	unsigned long app_dest;
	unsigned long flash_header_offset;
	size_t device_size;
	unsigned long flags;
};

/*
 * Actually write an image to the target device, eventually keeping a
 * DOS partition table on the device
 */
static int imx_bbu_write_device(struct imx_internal_bbu_handler *imx_handler,
		struct bbu_data *data, void *buf, int image_len)
{
	int fd, ret;

	fd = open(data->devicefile, O_RDWR | O_CREAT);
	if (fd < 0)
		return fd;

	if (imx_handler->flags & IMX_INTERNAL_FLAG_ERASE) {
		debug("%s: eraseing %s from 0 to 0x%08x\n", __func__,
				data->devicefile, image_len);
		ret = erase(fd, image_len, 0);
		if (ret) {
			printf("erasing %s failed with %s\n", data->devicefile,
					strerror(-ret));
			goto err_close;
		}
	}

	if (imx_handler->flags & IMX_INTERNAL_FLAG_KEEP_DOSPART) {
		void *mbr = xzalloc(512);

		debug("%s: reading DOS partition table in order to keep it\n", __func__);

		ret = read(fd, mbr, 512);
		if (ret < 0) {
			free(mbr);
			goto err_close;
		}

		memcpy(buf + 0x1b8, mbr + 0x1b8, 0x48);
		free(mbr);

		ret = lseek(fd, 0, SEEK_SET);
		if (ret)
			goto err_close;
	}

	ret = write(fd, buf, image_len);
	if (ret < 0)
		goto err_close;

	ret = 0;

err_close:
	close(fd);

	return ret;
}

static int imx_bbu_check_prereq(struct bbu_data *data)
{
	int ret;

	if (file_detect_type(data->image, data->len) != filetype_arm_barebox) {
		if (!bbu_force(data, "Not an ARM barebox image"))
			return -EINVAL;
	}

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	return 0;
}

/*
 * Update barebox on a v1 type internal boot (i.MX25, i.MX35, i.MX51)
 *
 * This constructs a DCD header, adds the specific DCD data and writes
 * the resulting image to the device. Currently this handles MMC/SD
 * devices.
 */
static int imx_bbu_internal_v1_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	struct imx_flash_header *flash_header;
	unsigned long flash_header_offset = imx_handler->flash_header_offset;
	u32 *dcd_image_size;
	void *imx_pre_image;
	int imx_pre_image_size = 0x2000;
	int ret, image_len;
	void *buf;

	ret = imx_bbu_check_prereq(data);
	if (ret)
		return ret;

	printf("updating to %s\n", data->devicefile);

	imx_pre_image = xzalloc(imx_pre_image_size);
	flash_header = imx_pre_image + flash_header_offset;

	flash_header->app_code_jump_vector = imx_handler->app_dest + 0x1000;
	flash_header->app_code_barker = APP_CODE_BARKER;
	flash_header->app_code_csf = 0;
	flash_header->dcd_ptr_ptr = imx_handler->app_dest + flash_header_offset +
		offsetof(struct imx_flash_header, dcd);
	flash_header->super_root_key = 0;
	flash_header->dcd = imx_handler->app_dest + flash_header_offset +
		offsetof(struct imx_flash_header, dcd_barker);
	flash_header->app_dest = imx_handler->app_dest;
	flash_header->dcd_barker = DCD_BARKER;
	flash_header->dcd_block_len = imx_handler->dcdsize;

	memcpy((void *)flash_header + sizeof(*flash_header), imx_handler->dcd, imx_handler->dcdsize);

	dcd_image_size = (imx_pre_image + flash_header_offset + sizeof(*flash_header) + imx_handler->dcdsize);

	*dcd_image_size = ALIGN(imx_pre_image_size + data->len, 4096);

	/* Create a buffer containing header and image data */
	image_len = data->len + imx_pre_image_size;
	buf = xzalloc(image_len);
	memcpy(buf, imx_pre_image, imx_pre_image_size);
	memcpy(buf + imx_pre_image_size, data->image, data->len);

	ret = imx_bbu_write_device(imx_handler, data, buf, image_len);

	free(buf);

	free(imx_pre_image);

	return ret;
}

#define DBBT_MAGIC	0x44424254
#define FCB_MAGIC	0x20424346

/*
 * Write an image to NAND. This creates a FCB header and a DBBT (Discovered Bad
 * Block Table). The DBBT is initialized with the bad blocks known from the mtd
 * layer.
 */
static int imx_bbu_internal_v2_write_nand_dbbt(struct imx_internal_bbu_handler *imx_handler,
		struct bbu_data *data, void *image, int image_len)
{
	struct mtd_info_user meminfo;
	int fd;
	struct stat s;
	int size_available, size_need;
	int ret;
	uint32_t *ptr, *num_bb, *bb;
	uint64_t offset;
	int block = 0, len, now, blocksize;

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

	ptr = image + 0x4;
	*ptr++ = FCB_MAGIC;	/* FCB */
	*ptr++ = 1;		/* FCB version */

	ptr = image + 0x78; /* DBBT start page */
	*ptr = 4;

	ptr = image + 4 * 2048 + 4;
	*ptr++ = DBBT_MAGIC;	/* DBBT */
	*ptr = 1;		/* DBBT version */

	ptr = (u32*)(image + 0x2010);
	/*
	 * This is marked as reserved in the i.MX53 reference manual, but
	 * must be != 0. Otherwise the ROM ignores the DBBT
	 */
	*ptr = 1;

	ptr = (u32*)(image + 0x4004); /* start of DBBT */
	num_bb = ptr;
	bb = ptr + 1;
	offset = 0;

	size_need = data->len + 0x8000;

	/*
	 * Collect bad blocks and construct DBBT
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
			*num_bb += 1;
			if (*num_bb == 425) {
				/* Maximum number of bad blocks the ROM supports */
				printf("maximum number of bad blocks reached\n");
				ret = -ENOSPC;
				goto out;
			}
			*bb++ = block;
			offset += blocksize;
			block++;
			continue;
		}
		size_need -= blocksize;
		size_available -= blocksize;
		offset += blocksize;
		block++;

		if (size_available < 0) {
			printf("device is too small");
			ret = -ENOSPC;
			goto out;
		}
	}

	debug("total image size: 0x%08x. Space needed including bad blocks: 0x%08x\n",
			data->len + 0x8000,
			data->len + 0x8000 + *num_bb * blocksize);

	if (data->len + 0x8000 + *num_bb * blocksize > imx_handler->device_size) {
		printf("needed space (0x%08x) exceeds partition space (0x%08x)\n",
				data->len + 0x8000 + *num_bb * blocksize,
				imx_handler->device_size);
		ret = -ENOSPC;
		goto out;
	}

	len = data->len + 0x8000;
	offset = 0;

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

static void imx_bbu_internal_v2_init_flash_header(struct bbu_handler *handler, struct bbu_data *data,
		void *imx_pre_image, int imx_pre_image_size)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	struct imx_flash_header_v2 *flash_header;
	unsigned long flash_header_offset = imx_handler->flash_header_offset;

	flash_header = imx_pre_image + flash_header_offset;

	flash_header->header.tag = IVT_HEADER_TAG;
	flash_header->header.length = cpu_to_be16(32);
	flash_header->header.version = IVT_VERSION;

	flash_header->entry = imx_handler->app_dest + imx_pre_image_size;
	if (imx_handler->dcdsize)
		flash_header->dcd_ptr = imx_handler->app_dest + flash_header_offset +
			offsetof(struct imx_flash_header_v2, dcd);
	flash_header->boot_data_ptr = imx_handler->app_dest +
		flash_header_offset + offsetof(struct imx_flash_header_v2, boot_data);
	flash_header->self = imx_handler->app_dest + flash_header_offset;

	flash_header->boot_data.start = imx_handler->app_dest;
	flash_header->boot_data.size = ALIGN(imx_pre_image_size + data->len, 4096);;

	if (imx_handler->dcdsize) {
		flash_header->dcd.header.tag = DCD_HEADER_TAG;
		flash_header->dcd.header.length = cpu_to_be16(sizeof(struct imx_dcd) +
				imx_handler->dcdsize);
		flash_header->dcd.header.version = DCD_VERSION;
	}

	/* Add dcd data */
	memcpy((void *)flash_header + sizeof(*flash_header), imx_handler->dcd, imx_handler->dcdsize);
}

#define IVT_BARKER		0x402000d1

/*
 * Update barebox on a v2 type internal boot (i.MX53)
 *
 * This constructs a DCD header, adds the specific DCD data and writes
 * the resulting image to the device. Currently this handles MMC/SD
 * and NAND devices.
 */
static int imx_bbu_internal_v2_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	void *imx_pre_image = NULL;
	int imx_pre_image_size;
	int ret, image_len;
	void *buf;

	ret = imx_bbu_check_prereq(data);
	if (ret)
		return ret;

	if (imx_handler->dcd) {
		imx_pre_image_size = 0x2000;
	} else {
		uint32_t *barker = data->image + imx_handler->flash_header_offset;

		if (*barker != IVT_BARKER) {
			printf("Board does not provide DCD data and this image is no imximage\n");
			return -EINVAL;
		}

		imx_pre_image_size = 0;
	}

	if (imx_handler->flags & IMX_INTERNAL_FLAG_NAND)
		/* NAND needs additional space for the DBBT */
		imx_pre_image_size += 0x6000;

	if (imx_pre_image_size)
		imx_pre_image = xzalloc(imx_pre_image_size);

	if (imx_handler->dcd)
		imx_bbu_internal_v2_init_flash_header(handler, data, imx_pre_image, imx_pre_image_size);

	/* Create a buffer containing header and image data */
	image_len = data->len + imx_pre_image_size;
	buf = xzalloc(image_len);
	if (imx_pre_image_size)
		memcpy(buf, imx_pre_image, imx_pre_image_size);
	memcpy(buf + imx_pre_image_size, data->image, data->len);

	if (imx_handler->flags & IMX_INTERNAL_FLAG_NAND) {
		ret = imx_bbu_internal_v2_write_nand_dbbt(imx_handler, data, buf,
				image_len);
		goto out_free_buf;
	}

	ret = imx_bbu_write_device(imx_handler, data, buf, image_len);

out_free_buf:
	free(buf);

	free(imx_pre_image);
	return ret;
}

/*
 * On the i.MX53 the dcd data can contain several commands. Each of them must
 * have its length encoded into it. We can't express that during compile time,
 * so use this function if you are using multiple dcd commands and wish to
 * concatenate them together to a single dcd table with the correct sizes for
 * each command.
 */
void *imx53_bbu_internal_concat_dcd_table(struct dcd_table *table, int num_entries)
{
	int i;
	unsigned int dcdsize = 0, pos = 0;
	void *dcdptr;

	for (i = 0; i < num_entries; i++)
		dcdsize += table[i].size;

	dcdptr = xmalloc(dcdsize);

	for (i = 0; i < num_entries; i++) {
		u32 *current = dcdptr + pos;
		memcpy(current, table[i].data, table[i].size);
		*current |= cpu_to_be32(table[i].size << 8);
		pos += table[i].size;
	}

	return dcdptr;
}

static struct imx_internal_bbu_handler *__init_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;
	struct bbu_handler *handler;

	imx_handler = xzalloc(sizeof(*imx_handler));
	handler = &imx_handler->handler;
	handler->devicefile = devicefile;
	handler->name = name;
	handler->flags = flags;

	return imx_handler;
}

static int __register_handler(struct imx_internal_bbu_handler *imx_handler)
{
	int ret;

	ret = bbu_register_handler(&imx_handler->handler);
	if (ret)
		free(imx_handler);

	return ret;
}

/*
 * Register a i.MX51 internal boot update handler for MMC/SD
 */
int imx51_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->dcd = dcd;
	imx_handler->dcdsize = dcdsize;
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	if (app_dest)
		imx_handler->app_dest = app_dest;
	else
		imx_handler->app_dest = 0x90000000;

	imx_handler->flags = IMX_INTERNAL_FLAG_KEEP_DOSPART;
	imx_handler->handler.handler = imx_bbu_internal_v1_update;

	return __register_handler(imx_handler);
}

#define DCD_WR_CMD(len)		cpu_to_be32(0xcc << 24 | (((len) & 0xffff) << 8) | 0x04)

static int imx53_bbu_internal_init_dcd(struct imx_internal_bbu_handler *imx_handler,
		void *dcd, int dcdsize)
{
	uint32_t *dcd32 = dcd;

	/*
	 * For boards which do not have a dcd (i.e. they do their SDRAM
	 * setup in C code)
	 */
	if (!dcd || !dcdsize)
		return 0;

	/*
	 * The DCD data we have compiled in does not have a DCD_WR_CMD at
	 * the beginning. Instead it is contained in struct imx_flash_header_v2.
	 * This is necessary to generate the DCD size at compile time. If
	 * we are passed such a DCD data here, prepend a DCD_WR_CMD.
	 */
	if ((*dcd32 & 0xff0000ff) != DCD_WR_CMD(0)) {
		__be32 *buf;

		debug("%s: dcd does not have a DCD_WR_CMD. Prepending one\n", __func__);

		buf = xmalloc(dcdsize + sizeof(__be32));

		*buf = DCD_WR_CMD(dcdsize + sizeof(__be32));
		memcpy(&buf[1], dcd, dcdsize);

		imx_handler->dcd = buf;
		imx_handler->dcdsize = dcdsize + sizeof(__be32);
	} else {
		debug("%s: dcd already has a DCD_WR_CMD. Using original dcd data\n", __func__);

		imx_handler->dcd = dcd;
		imx_handler->dcdsize = dcdsize;
	}

	return 0;
}

/*
 * Register a i.MX53 internal boot update handler for MMC/SD
 */
int imx53_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx53_bbu_internal_init_dcd(imx_handler, dcd, dcdsize);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	if (app_dest)
		imx_handler->app_dest = app_dest;
	else
		imx_handler->app_dest = 0x70000000;

	imx_handler->flags = IMX_INTERNAL_FLAG_KEEP_DOSPART;
	imx_handler->handler.handler = imx_bbu_internal_v2_update;

	return __register_handler(imx_handler);
}

/*
 * Register a i.MX53 internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int imx53_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx53_bbu_internal_init_dcd(imx_handler, dcd, dcdsize);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	if (app_dest)
		imx_handler->app_dest = app_dest;
	else
		imx_handler->app_dest = 0x70000000;

	imx_handler->flags = IMX_INTERNAL_FLAG_ERASE;
	imx_handler->handler.handler = imx_bbu_internal_v2_update;

	return __register_handler(imx_handler);
}

/*
 * Register a i.MX53 internal boot update handler for NAND
 */
int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		int partition_size, unsigned long app_dest)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, NULL, flags);
	imx53_bbu_internal_init_dcd(imx_handler, dcd, dcdsize);
	imx_handler->flash_header_offset = 0x400;

	if (app_dest)
		imx_handler->app_dest = app_dest;
	else
		imx_handler->app_dest = 0x70000000;

	imx_handler->handler.handler = imx_bbu_internal_v2_update;
	imx_handler->flags = IMX_INTERNAL_FLAG_NAND;
	imx_handler->handler.devicefile = "/dev/nand0";
	imx_handler->device_size = partition_size;

	return __register_handler(imx_handler);
}

/*
 * Register a i.MX6 internal boot update handler for MMC/SD
 */
int imx6_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	if (!app_dest)
		app_dest = 0x10000000;

	return imx53_bbu_internal_mmc_register_handler(name, devicefile,
		flags, dcd, dcdsize, app_dest);
}

/*
 * Register a i.MX53 internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int imx6_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	if (!app_dest)
		app_dest = 0x10000000;

	return imx53_bbu_internal_spi_i2c_register_handler(name, devicefile,
		flags, dcd, dcdsize, app_dest);
}

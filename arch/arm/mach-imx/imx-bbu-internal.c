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
#include <linux/sizes.h>
#include <linux/mtd/mtd-abi.h>
#include <linux/stat.h>
#include <ioctl.h>
#include <environment.h>
#include <mach/bbu.h>

#define FLASH_HEADER_OFFSET_MMC		0x400

#define IMX_INTERNAL_FLAG_NAND		(1 << 0)
#define IMX_INTERNAL_FLAG_KEEP_DOSPART	(1 << 1)
#define IMX_INTERNAL_FLAG_ERASE		(1 << 2)

struct imx_internal_bbu_handler {
	struct bbu_handler handler;
	unsigned long flash_header_offset;
	size_t device_size;
	unsigned long flags;
};

/*
 * Actually write an image to the target device, eventually keeping a
 * DOS partition table on the device
 */
static int imx_bbu_write_device(struct imx_internal_bbu_handler *imx_handler,
		const char *devicefile, struct bbu_data *data,
		void *buf, int image_len)
{
	int fd, ret;

	fd = open(devicefile, O_RDWR | O_CREAT);
	if (fd < 0)
		return fd;

	if (imx_handler->flags & IMX_INTERNAL_FLAG_ERASE) {
		debug("%s: unprotecting %s from 0 to 0x%08x\n", __func__,
				devicefile, image_len);
		ret = protect(fd, image_len, 0, 0);
		if (ret && ret != -ENOSYS) {
			printf("unprotecting %s failed with %s\n", devicefile,
					strerror(-ret));
			goto err_close;
		}

		debug("%s: erasing %s from 0 to 0x%08x\n", __func__,
				devicefile, image_len);
		ret = erase(fd, image_len, 0);
		if (ret) {
			printf("erasing %s failed with %s\n", devicefile,
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

	if (imx_handler->flags & IMX_INTERNAL_FLAG_ERASE) {
		debug("%s: protecting %s from 0 to 0x%08x\n", __func__,
				devicefile, image_len);
		ret = protect(fd, image_len, 0, 1);
		if (ret && ret != -ENOSYS) {
			printf("protecting %s failed with %s\n", devicefile,
					strerror(-ret));
		}
	}

	ret = 0;

err_close:
	close(fd);

	return ret;
}

static int imx_bbu_check_prereq(const char *devicefile, struct bbu_data *data)
{
	int ret;

	if (file_detect_type(data->image, data->len) != filetype_arm_barebox) {
		if (!bbu_force(data, "Not an ARM barebox image"))
			return -EINVAL;
	}

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	device_detect_by_name(devpath_to_name(devicefile));

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
	int ret;

	ret = imx_bbu_check_prereq(data->devicefile, data);
	if (ret)
		return ret;

	printf("updating to %s\n", data->devicefile);

	ret = imx_bbu_write_device(imx_handler, data->devicefile, data, data->image, data->len);

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
		struct bbu_data *data)
{
	struct mtd_info_user meminfo;
	int fd;
	struct stat s;
	int size_available, size_need;
	int ret;
	uint32_t *ptr, *num_bb, *bb;
	uint64_t offset;
	int block = 0, len, now, blocksize;
	int dbbt_start_page = 4;
	int firmware_start_page = 12;
	void *dbbt_base;
	void *image, *freep = NULL;
	int pre_image_size;

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

	pre_image_size = firmware_start_page * meminfo.writesize;
	image = freep = xzalloc(data->len + pre_image_size);
	memcpy(image + pre_image_size, data->image, data->len);

	blocksize = meminfo.erasesize;

	ptr = image + 0x4;
	*ptr++ = FCB_MAGIC;	/* FCB */
	*ptr++ = 1;		/* FCB version */

	ptr = image + 0x68; /* Firmware start page */
	*ptr = firmware_start_page;

	ptr = image + 0x78; /* DBBT start page */
	*ptr = dbbt_start_page;

	dbbt_base = image + dbbt_start_page * meminfo.writesize;
	ptr = dbbt_base + 4;
	*ptr++ = DBBT_MAGIC;	/* DBBT */
	*ptr = 1;		/* DBBT version */

	ptr = (u32*)(dbbt_base + 0x10);
	/*
	 * This is marked as reserved in the i.MX53 reference manual, but
	 * must be != 0. Otherwise the ROM ignores the DBBT
	 */
	*ptr = 1;

	ptr = (u32*)(dbbt_base + 4 * meminfo.writesize + 4); /* start of DBBT */
	num_bb = ptr;
	bb = ptr + 1;
	offset = 0;

	size_need = data->len + pre_image_size;

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

	debug("total image size: 0x%08zx. Space needed including bad blocks: 0x%08zx\n",
			data->len + pre_image_size,
			data->len + pre_image_size + *num_bb * blocksize);

	if (data->len + pre_image_size + *num_bb * blocksize > imx_handler->device_size) {
		printf("needed space (0x%08zx) exceeds partition space (0x%08zx)\n",
				data->len + pre_image_size + *num_bb * blocksize,
				imx_handler->device_size);
		ret = -ENOSPC;
		goto out;
	}

	len = data->len + pre_image_size;
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
	free(freep);

	return ret;
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
	int ret;
	uint32_t *barker;

	ret = imx_bbu_check_prereq(data->devicefile, data);
	if (ret)
		return ret;

	barker = data->image + imx_handler->flash_header_offset;

	if (*barker != IVT_BARKER) {
		printf("Board does not provide DCD data and this image is no imximage\n");
		return -EINVAL;
	}

	if (imx_handler->flags & IMX_INTERNAL_FLAG_NAND)
		ret = imx_bbu_internal_v2_write_nand_dbbt(imx_handler, data);
	else
		ret = imx_bbu_write_device(imx_handler, data->devicefile, data,
					   data->image, data->len);

	return ret;
}

static int imx_bbu_internal_v2_mmcboot_update(struct bbu_handler *handler,
					      struct bbu_data *data)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	int ret;
	uint32_t *barker;
	char *bootpartvar;
	const char *bootpart;
	char *devicefile;

	barker = data->image + imx_handler->flash_header_offset;

	if (*barker != IVT_BARKER) {
		printf("Board does not provide DCD data and this image is no imximage\n");
		return -EINVAL;
	}

	ret = asprintf(&bootpartvar, "%s.boot", data->devicefile);
	if (ret < 0)
		return ret;

	bootpart = getenv(bootpartvar);

	if (!strcmp(bootpart, "boot0")) {
		bootpart = "boot1";
	} else {
		bootpart = "boot0";
	}

	ret = asprintf(&devicefile, "/dev/%s.%s", data->devicefile, bootpart);
	if (ret < 0)
		goto free_bootpartvar;

	ret = imx_bbu_check_prereq(devicefile, data);
	if (ret)
		goto free_devicefile;

	ret = imx_bbu_write_device(imx_handler, devicefile, data, data->image, data->len);

	if (!ret)
		/* on success switch boot source */
		ret = setenv(bootpartvar, bootpart);

free_devicefile:
	free(devicefile);

free_bootpartvar:
	free(bootpartvar);

	return ret;
}

static int imx_bbu_external_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	int ret;

	ret = imx_bbu_check_prereq(data->devicefile, data);
	if (ret)
		return ret;

	return imx_bbu_write_device(imx_handler, data->devicefile, data,
				    data->image, data->len);
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
 * Register an i.MX51 internal boot update handler for MMC/SD
 */
int imx51_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	imx_handler->flags = IMX_INTERNAL_FLAG_KEEP_DOSPART;
	imx_handler->handler.handler = imx_bbu_internal_v1_update;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX53 internal boot update handler for MMC/SD
 */
int imx53_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	imx_handler->flags = IMX_INTERNAL_FLAG_KEEP_DOSPART;
	imx_handler->handler.handler = imx_bbu_internal_v2_update;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX6 internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int imx53_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	imx_handler->flags = IMX_INTERNAL_FLAG_ERASE;
	imx_handler->handler.handler = imx_bbu_internal_v2_update;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX53 internal boot update handler for NAND
 */
int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, int partition_size)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, NULL, flags);
	imx_handler->flash_header_offset = 0x400;

	imx_handler->handler.handler = imx_bbu_internal_v2_update;
	imx_handler->flags = IMX_INTERNAL_FLAG_NAND;
	imx_handler->handler.devicefile = "/dev/nand0";
	imx_handler->device_size = partition_size;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX6 internal boot update handler for MMC/SD
 */
int imx6_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	imx_handler->flags = IMX_INTERNAL_FLAG_KEEP_DOSPART;
	imx_handler->handler.handler = imx_bbu_internal_v2_update;

	return __register_handler(imx_handler);
}

/*
 * Register a handler that writes to the non-active boot partition of an mmc
 * medium and on success activates the written-to partition. So the machine can
 * still boot even after a failed try to write a boot image.
 *
 * Pass "devicefile" without partition name and /dev/ prefix. e.g. just "mmc2".
 * Note that no further partitioning of the boot partition is supported up to
 * now.
 */
int imx6_bbu_internal_mmcboot_register_handler(const char *name, char *devicefile,
					       unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	imx_handler->handler.handler = imx_bbu_internal_v2_mmcboot_update;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX53 internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int imx6_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = FLASH_HEADER_OFFSET_MMC;

	imx_handler->flags = IMX_INTERNAL_FLAG_ERASE;
	imx_handler->handler.handler = imx_bbu_internal_v2_update;

	return __register_handler(imx_handler);
}

int imx_bbu_external_nor_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flags = IMX_INTERNAL_FLAG_ERASE;
	imx_handler->handler.handler = imx_bbu_external_update;

	return __register_handler(imx_handler);
}

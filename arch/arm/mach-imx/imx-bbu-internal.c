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
#include <mach/generic.h>
#include <libfile.h>

#define IMX_INTERNAL_FLAG_ERASE		BIT(30)

struct imx_internal_bbu_handler {
	struct bbu_handler handler;
	int (*write_device)(struct imx_internal_bbu_handler *,
			    struct bbu_data *);
	unsigned long flash_header_offset;
	size_t device_size;
	enum filetype expected_type;
};

static bool
imx_bbu_erase_required(struct imx_internal_bbu_handler *imx_handler)
{
	return imx_handler->handler.flags & IMX_INTERNAL_FLAG_ERASE;
}

static int imx_bbu_protect(int fd, struct imx_internal_bbu_handler *imx_handler,
			   const char *devicefile, int offset, int image_len,
			   int prot)
{
	const char *prefix = prot ? "" : "un";
	int ret;

	if (!imx_bbu_erase_required(imx_handler))
		return 0;

	pr_debug("%s: %sprotecting %s from 0x%08x to 0x%08x\n", __func__,
		 prefix, devicefile, offset, image_len);

	ret = protect(fd, image_len, offset, prot);
	if (ret) {
		/*
		 * If protect() is not implemented for this device,
		 * just report success
		 */
		if (ret == -ENOSYS)
			return 0;

		pr_err("%sprotecting %s failed with %s\n", prefix, devicefile,
		       strerror(-ret));
	}

	return ret;
}

/*
 * Actually write an image to the target device, eventually keeping a
 * DOS partition table on the device
 */
static int imx_bbu_write_device(struct imx_internal_bbu_handler *imx_handler,
		const char *devicefile, struct bbu_data *data,
		const void *buf, int image_len)
{
	int fd, ret, offset = 0;

	fd = open(devicefile, O_RDWR | O_CREAT);
	if (fd < 0)
		return fd;

	if (imx_handler->handler.flags & (IMX_BBU_FLAG_KEEP_HEAD |
	    IMX_BBU_FLAG_PARTITION_STARTS_AT_HEADER)) {
		image_len -= imx_handler->flash_header_offset;
		buf += imx_handler->flash_header_offset;
	}

	if (imx_handler->handler.flags & IMX_BBU_FLAG_KEEP_HEAD)
		offset += imx_handler->flash_header_offset;

	ret = imx_bbu_protect(fd, imx_handler, devicefile, offset,
			      image_len, 0);
	if (ret)
		goto err_close;

	if (imx_bbu_erase_required(imx_handler)) {
		pr_debug("%s: erasing %s from 0x%08x to 0x%08x\n", __func__,
				devicefile, offset, image_len);
		ret = erase(fd, image_len, offset);
		if (ret) {
			pr_err("erasing %s failed with %s\n", devicefile,
					strerror(-ret));
			goto err_close;
		}
	}

	ret = pwrite_full(fd, buf, image_len, offset);
	if (ret < 0) {
		pr_err("writing to %s failed with %s\n", devicefile,
		       strerror(-ret));
		goto err_close;
	}

	imx_bbu_protect(fd, imx_handler, devicefile, offset,
			image_len, 1);

err_close:
	close(fd);

	return ret < 0 ? ret : 0;
}

static int __imx_bbu_write_device(struct imx_internal_bbu_handler *imx_handler,
				  struct bbu_data *data)
{
	return imx_bbu_write_device(imx_handler, data->devicefile, data,
				    data->image, data->len);
}

static int imx_bbu_check_prereq(struct imx_internal_bbu_handler *imx_handler,
				const char *devicefile, struct bbu_data *data,
				enum filetype expected_type)
{
	int ret;
	const void *blob;
	size_t len;
	enum filetype type;

	type = file_detect_type(data->image, data->len);

	switch (type) {
	case filetype_arm_barebox:
		/*
		 * Specifying expected_type as unknown will disable the
		 * inner image type check.
		 *
		 * The only user of this code is
		 * imx_bbu_external_nor_register_handler() used by
		 * i.MX27.
		 */
		if (expected_type == filetype_unknown)
			break;

		blob = data->image + imx_handler->flash_header_offset;
		len  = data->len   - imx_handler->flash_header_offset;
		type = file_detect_type(blob, len);

		if (type != expected_type) {
			pr_err("Expected image type: %s, "
			       "detected image type: %s\n",
			       file_type_to_string(expected_type),
			       file_type_to_string(type));
			return -EINVAL;
		}
		break;
	default:
		if (!bbu_force(data, "Not an ARM barebox image"))
			return -EINVAL;
	}

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	device_detect_by_name(devpath_to_name(devicefile));

	return 0;
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
				pr_err("1st block is bad. This is not supported\n");
				ret = -EINVAL;
				goto out;
			}

			debug("bad block at 0x%08llx\n", offset);
			*num_bb += 1;
			if (*num_bb == 425) {
				/* Maximum number of bad blocks the ROM supports */
				pr_err("maximum number of bad blocks reached\n");
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
			pr_err("device is too small");
			ret = -ENOSPC;
			goto out;
		}
	}

	debug("total image size: 0x%08zx. Space needed including bad blocks: 0x%08zx\n",
			data->len + pre_image_size,
			data->len + pre_image_size + *num_bb * blocksize);

	if (data->len + pre_image_size + *num_bb * blocksize > imx_handler->device_size) {
		pr_err("needed space (0x%08zx) exceeds partition space (0x%08zx)\n",
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

		pr_debug("writing %d bytes at 0x%08llx\n", now, offset);

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

static enum filetype imx_bbu_expected_filetype(void)
{
	if (cpu_is_mx8mq() ||
	    cpu_is_mx7()   ||
	    cpu_is_mx6()   ||
	    cpu_is_vf610() ||
	    cpu_is_mx53())
		return filetype_imx_image_v2;

	return filetype_imx_image_v1;
}

static unsigned long imx_bbu_flash_header_offset_mmc(void)
{
	unsigned long offset = SZ_1K;

	/*
	 * i.MX8MQ moved the header by 32K to accomodate for GPT
	 * partition tables
	 */
	if (cpu_is_mx8mq())
		offset += SZ_32K;

	return offset;
}

static int imx_bbu_update(struct bbu_handler *handler, struct bbu_data *data)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	int ret;

	ret = imx_bbu_check_prereq(imx_handler, data->devicefile, data,
				   imx_handler->expected_type);
	if (ret)
		return ret;

	return imx_handler->write_device(imx_handler, data);
}

static int imx_bbu_internal_v2_mmcboot_update(struct bbu_handler *handler,
					      struct bbu_data *data)
{
	struct imx_internal_bbu_handler *imx_handler =
		container_of(handler, struct imx_internal_bbu_handler, handler);
	int ret;
	char *bootpartvar;
	const char *bootpart;
	char *devicefile;

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

	ret = imx_bbu_check_prereq(imx_handler, devicefile, data,
				   filetype_imx_image_v2);
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

static struct imx_internal_bbu_handler *__init_handler(const char *name,
						       const char *devicefile,
						       unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;
	struct bbu_handler *handler;

	imx_handler = xzalloc(sizeof(*imx_handler));
	handler = &imx_handler->handler;
	handler->devicefile = devicefile;
	handler->name = name;
	handler->flags = flags;
	handler->handler = imx_bbu_update;

	imx_handler->expected_type = imx_bbu_expected_filetype();
	imx_handler->write_device = __imx_bbu_write_device;

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

static int
imx_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
				      unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags |
				     IMX_BBU_FLAG_KEEP_HEAD);
	imx_handler->flash_header_offset = imx_bbu_flash_header_offset_mmc();

	return __register_handler(imx_handler);
}

static int
imx_bbu_internal_spi_i2c_register_handler(const char *name,
					  const char *devicefile,
					  unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags |
				     IMX_INTERNAL_FLAG_ERASE);
	imx_handler->flash_header_offset = imx_bbu_flash_header_offset_mmc();

	return __register_handler(imx_handler);
}

int imx51_bbu_internal_spi_i2c_register_handler(const char *name,
						const char *devicefile,
						unsigned long flags)
	__alias(imx_bbu_internal_spi_i2c_register_handler);

/*
 * Register an i.MX51 internal boot update handler for MMC/SD
 */
int imx51_bbu_internal_mmc_register_handler(const char *name,
					    const char *devicefile,
					    unsigned long flags)
	__alias(imx_bbu_internal_mmc_register_handler);

/*
 * Register an i.MX53 internal boot update handler for MMC/SD
 */
int imx53_bbu_internal_mmc_register_handler(const char *name,
					    const char *devicefile,
					    unsigned long flags)
	__alias(imx_bbu_internal_mmc_register_handler);

/*
 * Register an i.MX6 internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int imx53_bbu_internal_spi_i2c_register_handler(const char *name,
						const char *devicefile,
						unsigned long flags)
	__alias(imx_bbu_internal_spi_i2c_register_handler);

/*
 * Register an i.MX53 internal boot update handler for NAND
 */
int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, int partition_size)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, "/dev/nand0", flags);
	imx_handler->flash_header_offset = imx_bbu_flash_header_offset_mmc();

	imx_handler->device_size = partition_size;
	imx_handler->write_device = imx_bbu_internal_v2_write_nand_dbbt;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX6 internal boot update handler for MMC/SD
 */
int imx6_bbu_internal_mmc_register_handler(const char *name,
					   const char *devicefile,
					   unsigned long flags)
	__alias(imx53_bbu_internal_mmc_register_handler);

/*
 * Register an VF610 internal boot update handler for MMC/SD
 */
int vf610_bbu_internal_mmc_register_handler(const char *name,
					    const char *devicefile,
					    unsigned long flags)
	__alias(imx6_bbu_internal_mmc_register_handler);

/*
 * Register an i.MX8MQ internal boot update handler for MMC/SD
 */
int imx8mq_bbu_internal_mmc_register_handler(const char *name,
					     const char *devicefile,
					     unsigned long flags)
	__alias(imx6_bbu_internal_mmc_register_handler);

/*
 * Register a handler that writes to the non-active boot partition of an mmc
 * medium and on success activates the written-to partition. So the machine can
 * still boot even after a failed try to write a boot image.
 *
 * Pass "devicefile" without partition name and /dev/ prefix. e.g. just "mmc2".
 * Note that no further partitioning of the boot partition is supported up to
 * now.
 */
int imx6_bbu_internal_mmcboot_register_handler(const char *name,
					       const char *devicefile,
					       unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags);
	imx_handler->flash_header_offset = imx_bbu_flash_header_offset_mmc();

	imx_handler->handler.handler = imx_bbu_internal_v2_mmcboot_update;

	return __register_handler(imx_handler);
}

/*
 * Register an i.MX53 internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int imx6_bbu_internal_spi_i2c_register_handler(const char *name,
					       const char *devicefile,
					       unsigned long flags)
	__alias(imx53_bbu_internal_spi_i2c_register_handler);

/*
 * Register an VFxxx internal boot update handler for i2c/spi
 * EEPROMs / flashes. Nearly the same as MMC/SD, but we do not need to
 * keep a partition table. We have to erase the device beforehand though.
 */
int vf610_bbu_internal_spi_i2c_register_handler(const char *name,
						const char *devicefile,
						unsigned long flags)
	__alias(imx6_bbu_internal_spi_i2c_register_handler);

int imx_bbu_external_nor_register_handler(const char *name,
					  const char *devicefile,
					  unsigned long flags)
{
	struct imx_internal_bbu_handler *imx_handler;

	imx_handler = __init_handler(name, devicefile, flags |
				     IMX_INTERNAL_FLAG_ERASE);

	imx_handler->expected_type = filetype_unknown;

	return __register_handler(imx_handler);
}

/*
 * am33xx_bbu_nand_xloadslots.c - barebox update handler for
 * the nand xload slots.
 *
 * Copyright (c) 2014 Wadim Egorov <w.egorov@phytec.de>, Phytec
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
#include <errno.h>
#include <bbu.h>
#include <fs.h>
#include <fcntl.h>
#include <libfile.h>
#include <filetype.h>
#include <linux/mtd/mtd.h>
#include <mtd/mtd-peb.h>
#include <mach/omap/bbu.h>

struct nand_bbu_handler {
	struct bbu_handler bbu_handler;
	char **devicefile;
	int num_devicefiles;
};

/*
 * Upate given nand partitions with an image
 */
static int nand_slot_update_handler(struct bbu_handler *handler,
						struct bbu_data *data)
{
	int ret, i;
	struct nand_bbu_handler *nh;
	const void *image = data->image;
	size_t size = data->len;

	nh = container_of(handler, struct nand_bbu_handler, bbu_handler);

	ret = bbu_confirm(data);
	if (ret != 0)
		return ret;

	/* check if the devicefile has been overwritten */
	if (strcmp(data->devicefile, nh->devicefile[0]) != 0) {
		ret = write_file_flash(data->devicefile, image, size);
		if (ret != 0)
			return ret;
	} else {
		for (i = 0; i < nh->num_devicefiles; i++) {
			ret =  write_file_flash(nh->devicefile[i], image, size);
			if (ret != 0)
				return ret;
		}
	}

	return 0;
}

/*
 * This handler updates all given xload slots in nand with an image.
 */
static int nand_xloadslots_update_handler(struct bbu_handler *handler,
					struct bbu_data *data)
{
	const void *image = data->image;
	size_t size = data->len;

	if (file_detect_type(image, size) != filetype_ch_image) {
		pr_err("%s is not a valid ch-image\n", data->imagefile);
		return -EINVAL;
	}

	return nand_slot_update_handler(handler, data);
}

int am33xx_bbu_nand_xloadslots_register_handler(const char *name,
						char **devicefile,
						int num_devicefiles)
{
	struct nand_bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->num_devicefiles = num_devicefiles;
	handler->bbu_handler.devicefile = devicefile[0];
	handler->bbu_handler.handler = nand_xloadslots_update_handler;
	handler->bbu_handler.name = name;

	ret = bbu_register_handler(&handler->bbu_handler);
	if (ret)
		free(handler);

	return ret;
}

static int nand_update_handler(struct bbu_handler *handler,
		struct bbu_data *data)
{
	const void *image = data->image;
	size_t size = data->len;

	if (file_detect_type(image, size) != filetype_arm_barebox) {
		pr_err("%s is not a valid barebox image\n", data->imagefile);
		return -EINVAL;
	}

	return nand_slot_update_handler(handler, data);
}

int am33xx_bbu_nand_slots_register_handler(const char *name, char **devicefile,
							int num_devicefiles)
{
	struct nand_bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->num_devicefiles = num_devicefiles;
	handler->bbu_handler.devicefile = devicefile[0];
	handler->bbu_handler.handler = nand_update_handler;
	handler->bbu_handler.name = name;

	ret = bbu_register_handler(&handler->bbu_handler);
	if (ret)
		free(handler);

	return ret;
}

#define XLOAD_BLOCKS 4

static int nand_update_handler_complete(struct bbu_handler *handler,
					struct bbu_data *data)
{
	const void *image = data->image;
	size_t size = data->len;
	enum filetype filetype;
	struct cdev *cdev;
	struct mtd_info *mtd;
	int ret, i;
	int npebs;

	filetype = file_detect_type(image, size);

	cdev = cdev_by_name(handler->devicefile);
	if (!cdev) {
		pr_err("%s: No NAND device found\n", __func__);
		return -ENODEV;
	}

        mtd = cdev->mtd;
	if (!mtd) {
		pr_err("%s: %s is not a mtd device\n", __func__,
		       handler->devicefile);
		return -EINVAL;
	}

	npebs = mtd_div_by_eb(mtd->size, mtd);

	/*
	 * Sanity check: We need at minimum 6 eraseblocks: 4 for the four xload
	 * binaries and 2 for the barebox images.
	 */
	if (npebs < XLOAD_BLOCKS + 2)
		return -EINVAL;

	if (filetype == filetype_arm_barebox) {
		int npebs_bb = (npebs - XLOAD_BLOCKS) / 2;

		pr_info("Barebox image detected, updating 2nd stage\n");

		/* last chance before erasing the flash */
		ret = bbu_confirm(data);
		if (ret)
			goto out;

		ret = mtd_peb_write_file(mtd, XLOAD_BLOCKS, npebs_bb, data->image,
					 data->len);
		if (ret)
			goto out;

		ret = mtd_peb_write_file(mtd, XLOAD_BLOCKS + npebs_bb, npebs_bb,
					 data->image, data->len);
		if (ret)
			goto out;

	} else if (filetype == filetype_ch_image) {
		int written = 0;
		void *buf;

		pr_info("xload image detected, updating 1st stage\n");

		if (data->len > mtd->erasesize) {
			pr_err("Image is bigger than eraseblock, this is not supported\n");
			ret = -EINVAL;
			goto out;
		}

		/* last chance before erasing the flash */
		ret = bbu_confirm(data);
		if (ret)
			goto out;

		buf = xzalloc(mtd->erasesize);
		memcpy(buf, data->image, data->len);

		for (i = 0; i < 4; i++) {
			if (mtd_peb_is_bad(mtd, i)) {
				pr_info("PEB%d is bad, skipping\n", i);
				continue;
			}

			ret = mtd_peb_erase(mtd, i);
			if (ret)
				continue;

			ret = mtd_peb_write(mtd, buf, i, 0, mtd->erasesize);
			if (ret) {
				pr_err("Failed to write MLO to PEB%d: %s\n", i,
				       strerror(-ret));
				continue;
			}
			written++;
		}

		free(buf);

		if (written)
			ret = 0;
		else
			ret = -EIO;
	} else {
		pr_err("%s of type %s is not a valid update file image\n",
		       data->imagefile, file_type_to_string(filetype));
		return -EINVAL;
	}
out:
	return ret;
}

/**
 * am33xx_bbu_nand_register_handler - register a NAND update handler
 * @device: The nand cdev name (usually "nand0.barebox")
 *
 * This registers an update handler suitable for updating barebox to NAND. This
 * update handler takes a single NAND partition for both the xload images and the
 * barebox images. The first four blocks are used for the 4 xload copies, the
 * remaining space is divided into two equally sized parts for two barebox images.
 * The update handler automatically detects based on the filetype if the xload
 * or the 2nd stage barebox shall be updated.
 *
 * FIXME: Currently for actually loading a barebox image from an xload image
 * flashed with this layout a suitable layout has to be registered by the xload
 * image using omap_set_barebox_part(). In the next step this should be the
 * default.
 */
int am33xx_bbu_nand_register_handler(const char *device)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = device;
	handler->handler = nand_update_handler_complete;
	handler->name = "nand";

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}

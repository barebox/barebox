// SPDX-License-Identifier: GPL-2.0-or-later
#include <common.h>
#include <malloc.h>
#include <bbu.h>
#include <filetype.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/sizes.h>
#include <linux/stat.h>
#include <ioctl.h>
#include <environment.h>
#include <mach/rockchip/bbu.h>
#include <libfile.h>
#include <linux/bitfield.h>
#include <mach/rockchip/rk3568-regs.h>
#include <mach/rockchip/bootrom.h>

/* The MaskROM looks for images on these locations: */
#define IMG_OFFSET_0	(0 * SZ_1K + SZ_32K)
#define IMG_OFFSET_1	(512 * SZ_1K + SZ_32K)
#define IMG_OFFSET_2	(1024 * SZ_1K + SZ_32K)
#define IMG_OFFSET_3	(1536 * SZ_1K + SZ_32K)
#define IMG_OFFSET_4	(2048 * SZ_1K + SZ_32K)

/*
 * The strategy here is:
 * The MaskROM iterates over the above five locations until it finds a valid
 * boot image. The images are protected with sha sums, so any change to an
 * image on disk is invalidating it. We first check if we have enough space to
 * write two copies of barebox. To make it simple we only use IMG_OFFSET_0 and
 * IMG_OFFSET_4 which leaves the maximum size for a single image. When there's
 * not enough free space on the beginning of the disk we only write a single
 * image. When we have enough space for two images we first write the inactive one
 * (leaving the active one intact). Afterwards we write the active one which
 * leaves the previously written inactive image as a fallback in case writing the
 * first one gets interrupted.
 */
static int rockchip_bbu_mmc_handler(struct bbu_handler *handler,
				    struct bbu_data *data)
{
	enum filetype filetype;
	int ret, fd, wr0, wr1;
	loff_t space;
	const char *cdevname;
	struct cdev *cdev;

	filetype = file_detect_type(data->image, data->len);
	if (filetype != filetype_rockchip_rkns_image) {
		if (!bbu_force(data, "incorrect image type. Expected: %s, got %s",
				file_type_to_string(filetype_rockchip_rkns_image),
				file_type_to_string(filetype)))
			return -EINVAL;
	}

	cdevname = devpath_to_name(data->devicefile);

	device_detect_by_name(cdevname);

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	cdev = cdev_open_by_name(cdevname, O_RDONLY);
	if (!cdev)
		return -ENOENT;

	space = cdev_unallocated_space(cdev);
	cdev_close(cdev);

	if (space < IMG_OFFSET_0 + data->len) {
		if (!bbu_force(data, "Unallocated space on %s (%lld) is too small for one image\n",
			       data->devicefile, space))
			return -ENOSPC;
	}

	fd = open(data->devicefile, O_WRONLY);
	if (fd < 0)
		return fd;

	if (space >= IMG_OFFSET_4 + data->len) {
		int slot = rockchip_bootsource_get_active_slot();

		pr_info("Unallocated space is enough for two copies, doing failsafe update\n");

		if (slot == 0) {
			wr0 = IMG_OFFSET_4;
			wr1 = IMG_OFFSET_0;
		} else {
			wr0 = IMG_OFFSET_0;
			wr1 = IMG_OFFSET_4;
		}
	} else {
		wr0 = IMG_OFFSET_0;
		wr1 = 0;
	}

	ret = pwrite_full(fd, data->image, data->len, wr0);
	if (ret < 0) {
		pr_err("writing to %s failed with %s\n", data->devicefile,
			strerror(-ret));
		goto err_close;
	}

	if (wr1) {
		ret = pwrite_full(fd, data->image, data->len, wr1);
		if (ret < 0) {
			pr_err("writing to %s failed with %s\n", data->devicefile,
				strerror(-ret));
			goto err_close;
		}
	}

	ret = 0;

err_close:
	close(fd);

	return ret;
}

int rockchip_bbu_mmc_register(const char *name, unsigned long flags,
			      const char *devicefile)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));

	handler->flags = flags;
	handler->devicefile = devicefile;
	handler->name = name;
	handler->handler = rockchip_bbu_mmc_handler;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}

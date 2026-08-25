// SPDX-License-Identifier: GPL-2.0-only
/*
 * barebox update handler for the PXA3xx NAND boot partition.
 */

#include <common.h>
#include <bbu.h>
#include <filetype.h>
#include <fs.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <mtd/mtd-peb.h>
#include <mach/pxa/bbu.h>

static int pxa_bbu_nand_handler(struct bbu_handler *handler,
				struct bbu_data *data)
{
	struct mtd_info *mtd;
	struct cdev *cdev;
	int ret;

	/*
	 * The Boot ROM expects an NTIM header at the start of the boot
	 * device. Writing anything else here leaves a board that only JTAG
	 * can talk to.
	 */
	if (file_detect_type(data->image, data->len) != filetype_pxa_ntim) {
		if (!bbu_force(data, "%s is not a PXA3xx NTIM image",
			       data->imagefile))
			return -EINVAL;
	}

	cdev = cdev_by_name(devpath_to_name(data->devicefile));
	if (!cdev) {
		pr_err("%s: no such device\n", data->devicefile);
		return -ENODEV;
	}

	mtd = cdev->mtd;
	if (!mtd) {
		pr_err("%s is not a mtd device\n", data->devicefile);
		return -EINVAL;
	}

	/* last chance before erasing the flash */
	ret = bbu_confirm(data);
	if (ret)
		return ret;

	ret = mtd_peb_write_file(mtd, 0, mtd_num_pebs(mtd), data->image,
				 data->len);
	if (ret == -ENOSPC)
		pr_err("%s holds %llu bytes, the image needs %zu\n",
		       data->devicefile, mtd->size, data->len);

	return ret;
}

/**
 * pxa_bbu_nand_register_handler - register a NAND update handler
 * @name:	Name of the handler
 * @devicefile:	the mtd partition the Boot ROM boots from
 *
 * Registers an update handler for the image built by scripts/pxa-image: the
 * NTIM header the Boot ROM reads, the OBM it copies into internal SRAM, and
 * the barebox image the OBM loads into DRAM.
 *
 * Unlike bbu_register_std_file_update() this goes through
 * mtd_peb_write_file(), so a factory bad block in the boot partition is
 * skipped rather than written to, and what lands in flash is the image with
 * the bad blocks taken out of it. That is what the other side expects:
 * pxa_nand_load_image() reads the flash offsets in the NTIM as offsets into
 * the good blocks and skips the bad ones the same way. The NTIM and the OBM
 * are in the first erase block, which cannot be bad, so the Boot ROM - which
 * has no idea about any of this - always finds them where it looks.
 *
 * Return: 0 if successful, negative error code otherwise
 */
int pxa_bbu_nand_register_handler(const char *name, const char *devicefile)
{
	struct bbu_handler *handler;
	int ret;

	handler = xzalloc(sizeof(*handler));
	handler->devicefile = devicefile;
	handler->name = name;
	handler->handler = pxa_bbu_nand_handler;
	handler->flags = BBU_HANDLER_FLAG_DEFAULT;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}

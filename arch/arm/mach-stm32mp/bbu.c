// SPDX-License-Identifier: GPL-2.0-or-later

#define pr_fmt(fmt) "stm32mp-bbu: " fmt
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
#include <mach/stm32mp/bbu.h>
#include <libfile.h>
#include <linux/bitfield.h>

#define STM32MP_BBU_IMAGE_HAVE_FSBL		BIT(0)
#define STM32MP_BBU_IMAGE_HAVE_FIP		BIT(1)

struct stm32mp_bbu_handler {
	struct bbu_handler handler;
	loff_t offset;
};

#define to_stm32mp_bbu_handler(h) container_of(h, struct stm32mp_bbu_handler, h)

static int stm32mp_bbu_gpt_part_update(struct bbu_handler *handler,
				       const struct bbu_data *data,
				       const char *part, bool optional)
{
	struct bbu_data gpt_data = *data;
	struct stat st;
	int ret;

	gpt_data.devicefile = xasprintf("%s.%s", gpt_data.devicefile, part);

	pr_debug("Attempting %s update\n", gpt_data.devicefile);

	ret = stat(gpt_data.devicefile, &st);
	if (ret == -ENOENT) {
		if (optional)
			return 0;
		pr_err("Partition %s does not exist\n", gpt_data.devicefile);
	}
	if (ret)
		goto out;

	ret = bbu_std_file_handler(handler, &gpt_data);
out:
	kfree_const(gpt_data.devicefile);
	return ret;
}

static int stm32mp_bbu_mmc_update(struct bbu_handler *handler,
				  struct bbu_data *data)
{
	struct stm32mp_bbu_handler *priv = to_stm32mp_bbu_handler(handler);
	int fd, ret;
	size_t image_len = data->len;
	const void *buf = data->image;
	struct stat st;

	pr_debug("Attempting eMMC boot partition update\n");

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	fd = open(data->devicefile, O_RDWR);
	if (fd < 0)
		return fd;

	ret = fstat(fd, &st);
	if (ret)
		goto close;

	if (st.st_size < priv->offset || image_len > st.st_size - priv->offset) {
		ret = -ENOSPC;
		goto close;
	}

	ret = pwrite_full(fd, buf, image_len, priv->offset);
	if (ret < 0)
		pr_err("writing to %s failed with %pe\n", data->devicefile, ERR_PTR(ret));

close:
	close(fd);

	return ret < 0 ? ret : 0;
}

/*
 * TF-A compiled with STM32_EMMC_BOOT will first check for FIP image
 * at offset SZ_256K and then in GPT partition of that name.
 */
static int stm32mp_bbu_mmc_fip_handler(struct bbu_handler *handler,
				       struct bbu_data *data)
{
	struct stm32mp_bbu_handler *priv = to_stm32mp_bbu_handler(handler);
	enum filetype filetype;
	int image_flags = 0, ret;
	bool is_emmc = true;

	filetype = file_detect_type(data->image, data->len);

	switch (filetype) {
	case filetype_stm32_image_fsbl_v1:
		priv->offset = 0;
		image_flags |= STM32MP_BBU_IMAGE_HAVE_FSBL;
		if (data->len > SZ_256K)
			image_flags |= STM32MP_BBU_IMAGE_HAVE_FIP;
		break;
	default:
		if (!bbu_force(data, "incorrect image type. Expected: %s, got %s",
				file_type_to_string(filetype_fip),
				file_type_to_string(filetype)))
			return -EINVAL;
		/* If forced assume it's a SSBL */
		filetype = filetype_fip;
		fallthrough;
	case filetype_fip:
		priv->offset = SZ_256K;
		image_flags |= STM32MP_BBU_IMAGE_HAVE_FIP;
		break;
	}

	pr_debug("Handling %s\n", file_type_to_string(filetype));

	handler->flags |= BBU_HANDLER_FLAG_MMC_BOOT_ACK;

	ret = bbu_mmcboot_handler(handler, data, stm32mp_bbu_mmc_update);
	if (ret == -ENOENT) {
		pr_debug("Not an eMMC, falling back to GPT fsbl1 partition\n");
		is_emmc = false;
		ret = 0;
	}
	if (ret < 0) {
		pr_debug("eMMC boot update failed: %pe\n", ERR_PTR(ret));
		return ret;
	}

	if (!is_emmc && (image_flags & STM32MP_BBU_IMAGE_HAVE_FSBL)) {
		struct bbu_data fsbl1_data = *data;

		fsbl1_data.len = min_t(size_t, fsbl1_data.len, SZ_256K);

		/*
		 * BootROM tells TF-A which fsbl slot was booted in r0, but TF-A
		 * doesn't yet propagate this to us, so for now always flash
		 * fsbl1
		 */
		ret = stm32mp_bbu_gpt_part_update(handler, &fsbl1_data, "fsbl1", false);
	}

	if (ret == 0 && (image_flags & STM32MP_BBU_IMAGE_HAVE_FIP)) {
		struct bbu_data fip_data = *data;

		if (image_flags & STM32MP_BBU_IMAGE_HAVE_FSBL) {
			fip_data.image += SZ_256K;
			fip_data.len -= SZ_256K;
		}

		/* No fip GPT partition in eMMC user area is usually ok, as
		 * that means TF-A is configured to load FIP from eMMC boot part
		 */
		ret = stm32mp_bbu_gpt_part_update(handler, &fip_data, "fip", is_emmc);
	}

	if (ret < 0)
		pr_debug("eMMC user area update failed: %pe\n", ERR_PTR(ret));

	return ret;
}

int stm32mp_bbu_mmc_fip_register(const char *name,
				 const char *devicefile,
				 unsigned long flags)
{
	struct stm32mp_bbu_handler *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->handler.flags = flags;
	priv->handler.devicefile = devicefile;
	priv->handler.name = name;
	priv->handler.handler = stm32mp_bbu_mmc_fip_handler;

	ret = bbu_register_handler(&priv->handler);
	if (ret)
		free(priv);

	return ret;
}

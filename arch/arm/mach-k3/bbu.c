// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "k3-bbu: " fmt

#include <bbu.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <filetype.h>
#include <linux/printk.h>
#include <unistd.h>
#include <mach/k3/common.h>
#include <linux/sizes.h>
#include <libfile.h>

static int k3_bbu_mmc_update(struct bbu_handler *handler,
				  struct bbu_data *data)
{
	int fd, ret;
	enum filetype type;

	if (data->len < K3_EMMC_BOOTPART_TIBOOT3_BIN_SIZE + SZ_1K) {
		pr_err("Image is too small\n");
		return -EINVAL;
	}

	type = file_detect_type(data->image + K3_EMMC_BOOTPART_TIBOOT3_BIN_SIZE, SZ_1K);

	if (type != filetype_fip) {
		pr_err("Cannot find FIP image at offset 1M\n");
		return -EINVAL;
	}

	pr_debug("Attempting eMMC boot partition update\n");

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	fd = open(data->devicefile, O_RDWR);
	if (fd < 0)
		return fd;

	ret = pwrite_full(fd, data->image, data->len, 0);
	if (ret < 0)
		pr_err("writing to %s failed with %pe\n", data->devicefile, ERR_PTR(ret));

	close(fd);

	return ret < 0 ? ret : 0;
}

static int k3_bbu_mmc_fip_handler(struct bbu_handler *handler,
				       struct bbu_data *data)
{
	return bbu_mmcboot_handler(handler, data, k3_bbu_mmc_update);
}

int k3_bbu_emmc_register(const char *name,
			 const char *devicefile,
			 unsigned long flags)
{
	struct bbu_handler *handler;
	int ret;

        handler = xzalloc(sizeof(*handler));

        handler->flags = flags | BBU_HANDLER_FLAG_MMC_BOOT_ACK;
        handler->devicefile = devicefile;
        handler->name = name;
        handler->handler = k3_bbu_mmc_fip_handler;

        ret = bbu_register_handler(handler);
        if (ret)
                free(handler);

        return ret;
}

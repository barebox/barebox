// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <bootm.h>
#include <memory.h>
#include <init.h>
#include <soc/imx9/flash_header.h>

static int do_bootm_imx_image_v3(struct image_data *data)
{
	void (*bb)(void);
	resource_size_t start, end;
	struct flash_header_v3 *hdr;
	u32 offset;
	int ret;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ret;

	ret = bootm_load_os(data, start);
	if (ret)
		return ret;

	hdr = (void *)start;
	offset = hdr->img[0].offset;

	if (data->verbose)
		printf("Loaded barebox image to 0x%08llx\n", start);

	shutdown_barebox();

	bb = (void *)start + offset;

	bb();

	return -EIO;
}

static struct image_handler imx_image_v3_handler = {
	.name = "",
	.bootm = do_bootm_imx_image_v3,
	.filetype = filetype_imx_image_v3,
};

static int imx_register_image_v3_handler(void)
{
	return register_image_handler(&imx_image_v3_handler);
}
late_initcall(imx_register_image_v3_handler);

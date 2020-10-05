// SPDX-License-Identifier: GPL-2.0

#include <bootm.h>
#include <common.h>
#include <init.h>
#include <memory.h>

static int do_bootm_zynqimage(struct image_data *data)
{
	resource_size_t start, end;
	void (*barebox)(void);
	u32 *header;
	int ret;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ret;

	ret = bootm_load_os(data, start);
	if (ret)
		return ret;

	header = (u32*)start;
	barebox = (void*)start + header[12];

	if (data->verbose)
		printf("Loaded barebox image to 0x%08lx\n",
		       (unsigned long)barebox);

	shutdown_barebox();

	barebox();

	return -EIO;
}

static struct image_handler zynq_image_handler = {
	.name = "Zynq image",
	.bootm = do_bootm_zynqimage,
	.filetype = filetype_zynq_image,
};

static int zynq_register_image_handler(void)
{
	return register_image_handler(&zynq_image_handler);
}
late_initcall(zynq_register_image_handler);

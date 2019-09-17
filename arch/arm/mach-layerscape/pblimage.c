#define pr_fmt(fmt) "pblimage: " fmt

#include <bootm.h>
#include <common.h>
#include <init.h>
#include <memory.h>
#include <linux/sizes.h>

#define BAREBOX_STAGE2_OFFSET	SZ_128K

static int do_bootm_layerscape_pblimage(struct image_data *data)
{
	void (*barebox)(unsigned long x0, unsigned long x1, unsigned long x2,
		       unsigned long x3);
	resource_size_t start, end;
	int ret;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ret;

	ret = bootm_load_os(data, start);
	if (ret)
		return ret;

	barebox = (void*)start + BAREBOX_STAGE2_OFFSET;

	if (data->verbose)
		printf("Loaded barebox image to 0x%08lx\n",
		       (unsigned long)barebox);

	shutdown_barebox();

	barebox(0, 0, 0, 0);

	return -EIO;
}

static struct image_handler image_handler_layerscape_pbl_image = {
	.name = "Layerscape image",
	.bootm = do_bootm_layerscape_pblimage,
	.filetype = filetype_layerscape_image,
};

static struct image_handler image_handler_layerscape_qspi_pbl_image = {
	.name = "Layerscape QSPI image",
	.bootm = do_bootm_layerscape_pblimage,
	.filetype = filetype_layerscape_qspi_image,
};

static int layerscape_register_pbl_image_handler(void)
{
	register_image_handler(&image_handler_layerscape_pbl_image);
	register_image_handler(&image_handler_layerscape_qspi_pbl_image);

	return 0;
}
late_initcall(layerscape_register_pbl_image_handler);

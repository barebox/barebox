#define pr_fmt(fmt) "stm32image: " fmt

#include <bootm.h>
#include <common.h>
#include <init.h>
#include <memory.h>
#include <linux/sizes.h>

#define BAREBOX_STAGE2_OFFSET	256

static int do_bootm_stm32image(struct image_data *data)
{
	void (*barebox)(void);
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

	barebox();

	return -EIO;
}

static struct image_handler image_handler_stm32_image_v1_handler = {
	.name = "STM32 image (v1)",
	.bootm = do_bootm_stm32image,
	.filetype = filetype_stm32_image_v1,
};

static int stm32mp_register_stm32image_image_handler(void)
{
	register_image_handler(&image_handler_stm32_image_v1_handler);

	return 0;
}
late_initcall(stm32mp_register_stm32image_image_handler);

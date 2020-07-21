#define pr_fmt(fmt) "at91-bootm-barebox: " fmt

#include <bootm.h>
#include <common.h>
#include <init.h>
#include <memory.h>
#include <mach/cpu.h>
#include <mach/sama5_bootsource.h>

static int do_bootm_at91_barebox_image(struct image_data *data)
{
	resource_size_t start, end;
	int ret;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ret;

	ret = bootm_load_os(data, start);
	if (ret)
		return ret;

	if (data->verbose)
		printf("Loaded barebox image to 0x%08zx\n", start);

	shutdown_barebox();

	sama5_boot_xload((void *)start, at91_bootsource);

	return -EIO;
}

static struct image_handler image_handler_at91_barebox_image = {
	.name = "AT91 barebox image",
	.bootm = do_bootm_at91_barebox_image,
	.filetype = filetype_arm_barebox,
};

static int at91_register_barebox_image_handler(void)
{
	if (!of_machine_is_compatible("atmel,sama5d2"))
	    return 0;

	return register_image_handler(&image_handler_at91_barebox_image);
}
late_initcall(at91_register_barebox_image_handler);

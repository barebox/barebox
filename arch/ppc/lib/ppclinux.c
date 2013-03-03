#define DEBUG

#include <common.h>
#include <command.h>
#include <image.h>
#include <init.h>
#include <environment.h>
#include <asm/bitops.h>
#include <boot.h>
#include <errno.h>
#include <fs.h>

static int do_bootm_linux(struct image_data *data)
{
	void	(*kernel)(void *, void *, unsigned long,
			unsigned long, unsigned long);

	if (!data->os_res)
		return -EINVAL;

	data->oftree = of_get_fixed_tree(data->of_root_node);
	if (!data->oftree) {
		pr_err("bootm: No devicetree given.\n");
		return -EINVAL;
	}

	fdt_add_reserve_map(data->oftree);

	kernel = (void *)(data->os_address + data->os_entry);

	/*
	 * Linux Kernel Parameters (passing device tree):
	 *   r3: ptr to OF flat tree, followed by the board info data
	 *   r4: physical pointer to the kernel itself
	 *   r5: NULL
	 *   r6: NULL
	 *   r7: NULL
	 */
	kernel(data->oftree, kernel, 0, 0, 0);

	reset_cpu(0);

	/* not reached */
	return -1;
}

static struct image_handler handler = {
	.name = "PowerPC Linux",
	.bootm = do_bootm_linux,
	.filetype = filetype_uimage,
	.ih_os = IH_OS_LINUX,
};

static int ppclinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(ppclinux_register_image_handler);

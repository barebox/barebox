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

#ifdef CONFIG_OF_FLAT_TREE
#include <ft_build.h>
#endif
extern bd_t *bd;

static int do_bootm_linux(struct image_data *data)
{
	void	(*kernel)(void *, void *, unsigned long,
			unsigned long, unsigned long);
	struct image_header *os_header = &data->os->header;

	kernel = (void *)image_get_ep(os_header);

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
	.bootm = do_bootm_linux,
	.image_type = IH_OS_LINUX,
};

static int ppclinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(ppclinux_register_image_handler);


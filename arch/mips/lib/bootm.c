#include <boot.h>
#include <common.h>
#include <init.h>
#include <fs.h>
#include <errno.h>
#include <binfmt.h>

#include <asm/byteorder.h>

static int do_bootm_barebox(struct image_data *data)
{
	void (*barebox)(void);

	barebox = read_file(data->os_file, NULL);
	if (!barebox)
		return -EINVAL;

	shutdown_barebox();

	barebox();

	reset_cpu(0);
}

static struct image_handler barebox_handler = {
	.name = "MIPS barebox",
	.bootm = do_bootm_barebox,
	.filetype = filetype_mips_barebox,
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_mips_barebox,
	.exec = "bootm",
};

static int mips_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	binfmt_register(&binfmt_barebox_hook);

	return 0;
}
late_initcall(mips_register_image_handler);

#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <zlib.h>
#include <init.h>
#include <fs.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>

#include <asm/byteorder.h>
#include <asm/global_data.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static int do_bootm_linux(struct image_data *data)
{
	void (*theKernel)(int zero, int arch, void *params);
	image_header_t *os_header = &data->os->header;

	if (image_get_type(os_header) == IH_TYPE_MULTI) {
		printf("Multifile images not handled at the moment\n");
		return -1;
	}

	theKernel = (void *)image_get_ep(os_header);

	debug("## Transferring control to Linux (at address 0x%p) ...\n",
	       theKernel);

	if (relocate_image(data->os, (void *)image_get_load(os_header)))
		return -1;

	if (data->initrd)
		if (relocate_image(data->initrd, (void *)image_get_load(&data->initrd->header)))
			return -1;

	/* we assume that the kernel is in place */
	printf("\nStarting kernel %s...\n\n", data->initrd ? "with initrd " : "");

	start_linux(theKernel, 0, data);

	return -1;
}

static int image_handle_cmdline_parse(struct image_data *data, int opt,
		char *optarg)
{
	int ret = 1;
	int no;

	switch (opt) {
	case 'a':
		no = simple_strtoul(optarg, NULL, 0);
		armlinux_set_architecture(no);
		ret = 0;
		break;
	case 'R':
		no = simple_strtoul(optarg, NULL, 0);
		armlinux_set_revision(no);
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}

static struct image_handler handler = {
	.cmdline_options = "a:R:",
	.cmdline_parse = image_handle_cmdline_parse,
	.help_string = " -a <arch>        use architecture number <arch>\n"
		       " -R <system_rev>  use system revison <system_rev>\n",

	.bootm = do_bootm_linux,
	.image_type = IH_OS_LINUX,
};

static int armlinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(armlinux_register_image_handler);

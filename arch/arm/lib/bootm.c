#include <boot.h>
#include <common.h>
#include <command.h>
#include <driver.h>
#include <environment.h>
#include <image.h>
#include <init.h>
#include <fs.h>
#include <linux/list.h>
#include <xfuncs.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>

#include <asm/byteorder.h>
#include <asm/setup.h>
#include <asm/barebox-arm.h>
#include <asm/armlinux.h>
#include <asm/system.h>

static int do_bootm_linux(struct image_data *data)
{
	void (*theKernel)(int zero, int arch, void *params);
	image_header_t *os_header = &data->os->header;

	theKernel = (void *)image_get_ep(os_header);

	debug("## Transferring control to Linux (at address 0x%p) ...\n",
	       theKernel);

	/* we assume that the kernel is in place */
	printf("\nStarting kernel %s...\n\n", data->initrd ? "with initrd " : "");

	start_linux(theKernel, 0, data);

	return -1;
}

static struct image_handler handler = {
	.bootm = do_bootm_linux,
	.image_type = IH_OS_LINUX,
};

static int armlinux_register_image_handler(void)
{
	return register_image_handler(&handler);
}

late_initcall(armlinux_register_image_handler);

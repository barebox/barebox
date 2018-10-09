#include <common.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#include <mach/xload.h>

int imx_image_size(void)
{
	/* i.MX header is 4k */
	return barebox_image_size + SZ_4K;
}

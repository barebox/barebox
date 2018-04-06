#include <common.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#include <mach/xload.h>

int imx_image_size(void)
{
	uint32_t *image_end = (void *)__image_end;
	uint32_t payload_len, pbl_len, imx_header_len, sizep;
	void *pg_start;

	pg_start = image_end + 1;

	/* i.MX header is 4k */
	imx_header_len = SZ_4K;

	/* The length of the PBL image */
	pbl_len = __image_end - _text;

	sizep = 4;

	/* The length of the payload is appended directly behind the PBL */
	payload_len = *(image_end);

	pr_debug("%s: payload_len: 0x%08x pbl_len: 0x%08x\n",
			__func__, payload_len, pbl_len);

	return imx_header_len + pbl_len + sizep + payload_len;
}

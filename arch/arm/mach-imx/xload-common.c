// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#include <mach/imx/xload.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/imx-header.h>
#include <asm/barebox-arm.h>

int imx_image_size(void)
{
	/* i.MX header is 4k */
	return barebox_image_size + SZ_4K;
}

int piggydata_size(void)
{
	return input_data_end - input_data;
}

struct imx_scratch_space *__imx8m_scratch_space(int ddr_buswidth)
{
	ulong endmem = MX8M_DDR_CSD1_BASE_ADDR +
		imx8m_barebox_earlymem_size(ddr_buswidth);

	return (void *)arm_mem_scratch(endmem);
}

#define HDR_SIZE	512

static int
imx_search_header(struct imx_flash_header_v2 **header_pointer,
		  void *buffer, u32 *offset, u32 ivt_offset,
		  int (*read)(void *dest, size_t len, void *priv),
		  void *priv)
{
	int ret;
	int i, header_count = 1;
	void *buf = buffer;
	struct imx_flash_header_v2 *hdr;

	for (i = 0; i < header_count; i++) {
		ret = read(buf, *offset + ivt_offset + HDR_SIZE, priv);
		if (ret)
			return ret;

		hdr = buf + *offset + ivt_offset;

		if (!is_imx_flash_header_v2(hdr)) {
			pr_debug("No IVT header! "
				 "Found tag: 0x%02x length: 0x%04x "
				 "version: %02x\n",
				 hdr->header.tag, hdr->header.length,
				 hdr->header.version);
			return -EINVAL;
		}

		if (IS_ENABLED(CONFIG_ARCH_IMX8MQ) &&
		    hdr->boot_data.plugin & PLUGIN_HDMI_IMAGE) {
			/*
			 * In images that include signed HDMI
			 * firmware, first v2 header would be
			 * dedicated to that and would not contain any
			 * useful for us information. In order for us
			 * to pull the rest of the bootloader image
			 * in, we need to re-read header from SD/MMC,
			 * this time skipping anything HDMI firmware
			 * related.
			 */
			*offset += hdr->boot_data.size + hdr->header.length;
			header_count++;
		}
	}
	*header_pointer = hdr;
	return 0;
}

int imx_load_image(ptrdiff_t address, ptrdiff_t entry, u32 offset,
		   u32 ivt_offset, bool start, unsigned int alignment,
		   int (*read)(void *dest, size_t len, void *priv),
		   void *priv)
{

	void *buf = (void *)address;
	struct imx_flash_header_v2 *hdr = NULL;
	int ret, len;
	void __noreturn (*bb)(void);
	unsigned int ofs;

	len = imx_image_size();
	if (alignment)
		len = ALIGN(len, alignment);

	ret = imx_search_header(&hdr, buf, &offset, ivt_offset, read, priv);
	if (ret)
		return ret;

	pr_debug("Check ok, loading image\n");

	ofs = offset + hdr->entry - hdr->boot_data.start;

	if (entry != address) {
		/*
		 * Passing entry different from address is interpreted
		 * as a request to place the image such that its entry
		 * point would be exactly at 'entry', that is:
		 *
		 *     buf + ofs = entry
		 *
		 * solving the above for 'buf' gives us the
		 * adjustment that needs to be made:
		 *
		 *     buf = entry - ofs
		 *
		 */
		if (WARN_ON(entry - ofs < address)) {
			/*
			 * We want to make sure we won't try to place
			 * the start of the image before the beginning
			 * of the memory buffer we were given in
			 * address.
			 */
			return -EINVAL;
		}

		buf = (void *)(entry - ofs);
	}

	ret = read(buf, ofs + len, priv);
	if (ret) {
		pr_err("Loading image failed with %d\n", ret);
		return ret;
	}

	pr_debug("Image loaded successfully\n");

	if (!start)
		return 0;

	bb = buf + ofs;

	sync_caches_for_execution();

	bb();
}

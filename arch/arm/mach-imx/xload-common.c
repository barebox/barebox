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

/**
 * imx_load_image - Load i.MX barebox image from boot medium
 * @address: Start address of SDRAM where barebox can be loaded into
 * @entry: Address where barebox entry point should be placed.
 *         This is ignored unless @start == false
 * @offset: Start offset for i.MX header search
 * @ivt_offset: offset between i.MX header and IVT
 * @start: whether image should be started after loading
 * @alignment: If nonzero, image size hardcoded in PBL will be aligned up
 *             to this value
 * @read: function pointer for reading from the beginning of the boot
 *        medium onwards
 * @priv: private data pointer passed to read function
 *
 * Return: A negative error code on failure.
 * On success, if @start == true, the function will not return.
 * If @start == false, the function will return 0 after placing the
 * barebox entry point (without header) at @entry.
 */
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

	if (!start) {
		/*
		 * When !start, the caller will start the image later on,
		 * expecting that it is placed such that its entry
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

	/*
	 * For SD/MMC High-Capacity support (> 2G), the offset for the block
	 * read command is in blocks, not bytes. We don't have the information
	 * whether we have a SDHC card or not, when we run here though, because
	 * card setup was done by BootROM. To workaround this, we just read
	 * from offset 0 as 0 blocks == 0 bytes.
	 *
	 * A result of this is that we will have to read the i.MX header and
	 * padding in front of the binary first to arrive at the barebox entry
	 * point.
	 */
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

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#define pr_fmt(fmt) "xload-esdhc: " fmt

#include <common.h>
#include <io.h>
#include <mci.h>
#include <linux/sizes.h>
#include <asm-generic/sections.h>
#include <asm/cache.h>
#include <mach/xload.h>
#ifdef CONFIG_ARCH_IMX
#include <mach/atf.h>
#include <mach/imx6-regs.h>
#include <mach/imx7-regs.h>
#include <mach/imx8mq-regs.h>
#include <mach/imx8mm-regs.h>
#include <mach/imx-header.h>
#endif
#include "sdhci.h"
#include "imx-esdhc.h"

#define SECTOR_SIZE 512
#define SECTOR_WML  (SECTOR_SIZE / sizeof(u32))

#define esdhc_send_cmd	__esdhc_send_cmd

static int esdhc_read_blocks(struct fsl_esdhc_host *host, void *dst, size_t len)
{
	struct mci_cmd cmd;
	struct mci_data data;
	u32 val;
	int ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE,
		      SDHCI_INT_CMD_COMPLETE | SDHCI_INT_XFER_COMPLETE |
		      SDHCI_INT_CARD_INT | SDHCI_INT_TIMEOUT | SDHCI_INT_CRC |
		      SDHCI_INT_END_BIT | SDHCI_INT_INDEX | SDHCI_INT_DATA_TIMEOUT |
		      SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_END_BIT | SDHCI_INT_DMA);

	val = sdhci_read32(&host->sdhci, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
	val |= SYSCTL_HCKEN | SYSCTL_IPGEN;
	sdhci_write32(&host->sdhci, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET, val);

	cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = len / SECTOR_SIZE;
	data.blocksize = SECTOR_SIZE;
	data.flags = MMC_DATA_READ;

	ret = esdhc_send_cmd(host, &cmd, &data);
	if (ret) {
		pr_debug("send command failed with %d\n", ret);
		return ret;
	}

	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1b;

	esdhc_send_cmd(host, &cmd, NULL);

	return 0;
}

#ifdef CONFIG_ARCH_IMX
static int esdhc_search_header(struct fsl_esdhc_host *host,
			       struct imx_flash_header_v2 **header_pointer,
			       void *buffer, u32 *offset, u32 ivt_offset)
{
	int ret;
	int i, header_count = 1;
	void *buf = buffer;
	struct imx_flash_header_v2 *hdr;

	for (i = 0; i < header_count; i++) {
		ret = esdhc_read_blocks(host, buf,
					*offset + ivt_offset + SECTOR_SIZE);
		if (ret)
			return ret;

		hdr = buf + *offset + ivt_offset;

		if (!is_imx_flash_header_v2(hdr)) {
			pr_debug("IVT header not found on SD card. "
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

static int
esdhc_load_image(struct fsl_esdhc_host *host, ptrdiff_t address,
		 ptrdiff_t entry, u32 offset, u32 ivt_offset, bool start)
{

	void *buf = (void *)address;
	struct imx_flash_header_v2 *hdr = NULL;
	int ret, len;
	void __noreturn (*bb)(void);
	unsigned int ofs;

	len = imx_image_size();
	len = ALIGN(len, SECTOR_SIZE);

	ret = esdhc_search_header(host, &hdr, buf, &offset, ivt_offset);
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
		 * solving the above for 'buf' gvies us the
		 * adjustement that needs to be made:
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

	ret = esdhc_read_blocks(host, buf, offset + len);
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

static void imx_esdhc_init(struct fsl_esdhc_host *host,
			   struct esdhc_soc_data *data)
{
	data->flags = ESDHC_FLAG_USDHC;
	host->socdata = data;
	esdhc_populate_sdhci(host);

	sdhci_write32(&host->sdhci, IMX_SDHCI_WML,
		      FIELD_PREP(WML_WR_BRST_LEN, 16)         |
		      FIELD_PREP(WML_WR_WML_MASK, SECTOR_WML) |
		      FIELD_PREP(WML_RD_BRST_LEN, 16)         |
		      FIELD_PREP(WML_RD_WML_MASK, SECTOR_WML));
}

static int imx8m_esdhc_init(struct fsl_esdhc_host *host,
			    struct esdhc_soc_data *data,
			    int instance)
{
	switch (instance) {
	case 0:
		host->regs = IOMEM(MX8M_USDHC1_BASE_ADDR);
		break;
	case 1:
		host->regs = IOMEM(MX8M_USDHC2_BASE_ADDR);
		break;
	case 2:
		/* Only exists on i.MX8MM, not on i.MX8MQ */
		host->regs = IOMEM(MX8MM_USDHC3_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	imx_esdhc_init(host, data);

	return 0;
}

/**
 * imx6_esdhc_start_image - Load and start an image from USDHC controller
 * @instance: The USDHC controller instance (0..4)
 *
 * This uses esdhc_start_image() to load an image from SD/MMC.  It is
 * assumed that the image is the currently running barebox image (This
 * information is used to calculate the length of the image). The
 * image is started afterwards.
 *
 * Return: If successful, this function does not return. A negative error
 * code is returned when this function fails.
 */
int imx6_esdhc_start_image(int instance)
{
	struct esdhc_soc_data data;
	struct fsl_esdhc_host host;

	switch (instance) {
	case 0:
		host.regs = IOMEM(MX6_USDHC1_BASE_ADDR);
		break;
	case 1:
		host.regs = IOMEM(MX6_USDHC2_BASE_ADDR);
		break;
	case 2:
		host.regs = IOMEM(MX6_USDHC3_BASE_ADDR);
		break;
	case 3:
		host.regs = IOMEM(MX6_USDHC4_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	imx_esdhc_init(&host, &data);

	return esdhc_load_image(&host, 0x10000000, 0x10000000, 0, SZ_1K, true);
}

/**
 * imx7_esdhc_start_image - Load and start an image from USDHC controller
 * @instance: The USDHC controller instance (0..2)
 *
 * This uses esdhc_start_image() to load an image from SD/MMC.  It is
 * assumed that the image is the currently running barebox image (This
 * information is used to calculate the length of the image). The
 * image is started afterwards.
 *
 * Return: If successful, this function does not return. A negative error
 * code is returned when this function fails.
 */
int imx7_esdhc_start_image(int instance)
{
	struct esdhc_soc_data data;
	struct fsl_esdhc_host host;

	switch (instance) {
	case 0:
		host.regs = IOMEM(MX7_USDHC1_BASE_ADDR);
		break;
	case 1:
		host.regs = IOMEM(MX7_USDHC2_BASE_ADDR);
		break;
	case 2:
		host.regs = IOMEM(MX7_USDHC3_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	imx_esdhc_init(&host, &data);

	return esdhc_load_image(&host, 0x80000000, 0x80000000, 0, SZ_1K, true);
}

/**
 * imx8m_esdhc_load_image - Load and optionally start an image from USDHC controller
 * @instance: The USDHC controller instance (0..2)
 * @start: Whether to directly start the loaded image
 *
 * This uses esdhc_start_image() to load an image from SD/MMC.  It is
 * assumed that the image is the currently running barebox image (This
 * information is used to calculate the length of the image). The
 * image is started afterwards.
 *
 * Return: If successful, this function does not return (if directly started)
 * or 0. A negative error code is returned when this function fails.
 */
int imx8m_esdhc_load_image(int instance, bool start)
{
	struct esdhc_soc_data data;
	struct fsl_esdhc_host host;
	int ret;

	ret = imx8m_esdhc_init(&host, &data, instance);
	if (ret)
		return ret;

	return esdhc_load_image(&host, MX8M_DDR_CSD1_BASE_ADDR,
				MX8MQ_ATF_BL33_BASE_ADDR, SZ_32K, SZ_1K,
				start);
}

/**
 * imx8mp_esdhc_load_image - Load and optionally start an image from USDHC controller
 * @instance: The USDHC controller instance (0..2)
 * @start: Whether to directly start the loaded image
 *
 * This uses esdhc_start_image() to load an image from SD/MMC.  It is
 * assumed that the image is the currently running barebox image (This
 * information is used to calculate the length of the image). The
 * image is started afterwards.
 *
 * Return: If successful, this function does not return (if directly started)
 * or 0. A negative error code is returned when this function fails.
 */
int imx8mp_esdhc_load_image(int instance, bool start)
{
	struct esdhc_soc_data data;
	struct fsl_esdhc_host host;
	int ret;

	ret = imx8m_esdhc_init(&host, &data, instance);
	if (ret)
		return ret;

	return esdhc_load_image(&host, MX8M_DDR_CSD1_BASE_ADDR,
				MX8MQ_ATF_BL33_BASE_ADDR, SZ_32K, 0, start);
}
#endif

#ifdef CONFIG_ARCH_LS1046

/*
 * The image on the SD card starts at 0x1000. We reserved 128KiB for the PBL,
 * so the 2nd stage image starts here:
 */
#define LS1046A_SD_IMAGE_OFFSET (SZ_4K + SZ_128K)

/**
 * ls1046a_esdhc_start_image - Load and start a 2nd stage from the ESDHC controller
 *
 * This loads and starts a 2nd stage barebox from an SD card and starts it. We
 * assume the image has been generated with scripts/pblimage.c which puts the
 * second stage to an offset of 128KiB in the image.
 *
 * Return: If successful, this function does not return. A negative error
 * code is returned when this function fails.
 */
int ls1046a_esdhc_start_image(unsigned long r0, unsigned long r1, unsigned long r2)
{
	int ret;
	uint32_t val;
	struct esdhc_soc_data data = {
		.flags = ESDHC_FLAG_BIGENDIAN,
	};
	struct fsl_esdhc_host host = {
		.regs = IOMEM(0x01560000),
		.socdata = &data,
	};
	unsigned long sdram = 0x80000000;
	void (*barebox)(unsigned long, unsigned long, unsigned long) =
		(void *)(sdram + LS1046A_SD_IMAGE_OFFSET);

	esdhc_populate_sdhci(&host);
	sdhci_write32(&host.sdhci, IMX_SDHCI_WML, 0);

	/*
	 * The ROM leaves us here with a clock frequency of around 400kHz. Speed
	 * this up a bit. FIXME: The resulting frequency has not yet been verified
	 * to work on all cards.
	 */
	val = sdhci_read32(&host.sdhci, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
	val &= ~0x0000fff0;
	val |= (8 << 8) | (3 << 4);
	sdhci_write32(&host.sdhci, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET, val);

	sdhci_write32(&host.sdhci, ESDHC_DMA_SYSCTL, ESDHC_SYSCTL_DMA_SNOOP);

	ret = esdhc_read_blocks(&host, (void *)sdram,
			ALIGN(barebox_image_size + LS1046A_SD_IMAGE_OFFSET, 512));
	if (ret) {
		pr_err("%s: reading blocks failed with: %d\n", __func__, ret);
		return ret;
	}

	sync_caches_for_execution();

	printf("Starting barebox\n");

	barebox(r0, r1, r2);

	return -EINVAL;
}
#endif

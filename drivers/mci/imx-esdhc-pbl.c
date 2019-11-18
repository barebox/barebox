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
#include <mach/imx8mq-regs.h>
#include <mach/imx-header.h>
#endif
#include "sdhci.h"
#include "imx-esdhc.h"

#define SECTOR_SIZE 512
#define SECTOR_WML  (SECTOR_SIZE / sizeof(u32))

struct esdhc {
	void __iomem *regs;
	bool is_mx6;
	bool is_be;
	bool wrap_wml;
};

static uint32_t esdhc_read32(struct esdhc *esdhc, int reg)
{
	if (esdhc->is_be)
		return in_be32(esdhc->regs + reg);
	else
		return readl(esdhc->regs + reg);
}

static void esdhc_write32(struct esdhc *esdhc, int reg, uint32_t val)
{
	if (esdhc->is_be)
		out_be32(esdhc->regs + reg, val);
	else
		writel(val, esdhc->regs + reg);
}

static void __udelay(int us)
{
	volatile int i;

	for (i = 0; i < us * 4; i++);
}

static u32 esdhc_xfertyp(struct mci_cmd *cmd, struct mci_data *data)
{
	u32 xfertyp = 0;
	u32 command = 0;

	if (data) {
		command |= SDHCI_DATA_PRESENT;
		xfertyp |= SDHCI_MULTIPLE_BLOCKS | SDHCI_BLOCK_COUNT_EN |
			   SDHCI_DATA_TO_HOST;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		command |= SDHCI_CMD_CRC_CHECK_EN;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		xfertyp |= SDHCI_CMD_INDEX_CHECK_EN;
	if (cmd->resp_type & MMC_RSP_136)
		command |= SDHCI_RESP_TYPE_136;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		command |= SDHCI_RESP_TYPE_48_BUSY;
	else if (cmd->resp_type & MMC_RSP_PRESENT)
		command |= SDHCI_RESP_TYPE_48;

	command |= SDHCI_CMD_INDEX(cmd->cmdidx);

	return command << 16 | xfertyp;
}

static int esdhc_do_data(struct esdhc *esdhc, struct mci_data *data)
{
	char *buffer;
	u32 databuf;
	u32 size;
	u32 irqstat;
	u32 present;

	buffer = data->dest;

	size = data->blocksize * data->blocks;
	irqstat = esdhc_read32(esdhc, SDHCI_INT_STATUS);

	while (size) {
		int i;
		int timeout = 1000000;

		while (1) {
			present = esdhc_read32(esdhc, SDHCI_PRESENT_STATE) & SDHCI_BUFFER_READ_ENABLE;
			if (present)
				break;
			if (!--timeout) {
				pr_err("read time out\n");
				return -ETIMEDOUT;
			}
		}

		for (i = 0; i < SECTOR_WML; i++) {
			databuf = esdhc_read32(esdhc, SDHCI_BUFFER);
			*((u32 *)buffer) = databuf;
			buffer += 4;
			size -= 4;
		}
	}

	return 0;
}

static int
esdhc_send_cmd(struct esdhc *esdhc, struct mci_cmd *cmd, struct mci_data *data)
{
	u32	xfertyp, mixctrl;
	u32	irqstat;
	int ret;
	int timeout;

	esdhc_write32(esdhc, SDHCI_INT_STATUS, -1);

	/* Wait at least 8 SD clock cycles before the next command */
	__udelay(1);

	if (data) {
		unsigned long dest = (unsigned long)data->dest;

		if (dest > 0xffffffff)
			return -EINVAL;

		/* Set up for a data transfer if we have one */
		esdhc_write32(esdhc, SDHCI_DMA_ADDRESS, (u32)dest);
		esdhc_write32(esdhc, SDHCI_BLOCK_SIZE__BLOCK_COUNT, data->blocks << 16 | SECTOR_SIZE);
	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	/* Send the command */
	esdhc_write32(esdhc, SDHCI_ARGUMENT, cmd->cmdarg);

	if (esdhc->is_mx6) {
		/* write lower-half of xfertyp to mixctrl */
		mixctrl = xfertyp & 0xFFFF;
		/* Keep the bits 22-25 of the register as is */
		mixctrl |= (esdhc_read32(esdhc, IMX_SDHCI_MIXCTRL) & (0xF << 22));
		esdhc_write32(esdhc, IMX_SDHCI_MIXCTRL, mixctrl);
	}

	esdhc_write32(esdhc, SDHCI_TRANSFER_MODE__COMMAND, xfertyp);

	/* Wait for the command to complete */
	timeout = 10000;
	while (!(esdhc_read32(esdhc, SDHCI_INT_STATUS) & SDHCI_INT_CMD_COMPLETE)) {
		__udelay(1);
		if (!timeout--)
			return -ETIMEDOUT;
	}

	irqstat = esdhc_read32(esdhc, SDHCI_INT_STATUS);
	esdhc_write32(esdhc, SDHCI_INT_STATUS, irqstat);

	if (irqstat & CMD_ERR)
		return -EIO;

	if (irqstat & SDHCI_INT_TIMEOUT)
		return -ETIMEDOUT;

	/* Copy the response to the response buffer */
	cmd->response[0] = esdhc_read32(esdhc, SDHCI_RESPONSE_0);

	/* Wait until all of the blocks are transferred */
	if (data) {
		ret = esdhc_do_data(esdhc, data);
		if (ret)
			return ret;
	}

	esdhc_write32(esdhc, SDHCI_INT_STATUS, -1);

	/* Wait for the bus to be idle */
	timeout = 10000;
	while (esdhc_read32(esdhc, SDHCI_PRESENT_STATE) & (SDHCI_CMD_INHIBIT_CMD |
			SDHCI_CMD_INHIBIT_DATA | SDHCI_DATA_LINE_ACTIVE)) {
		__udelay(1);
		if (!timeout--)
			return -ETIMEDOUT;
	}

	return 0;
}

static int esdhc_read_blocks(struct esdhc *esdhc, void *dst, size_t len)
{
	struct mci_cmd cmd;
	struct mci_data data;
	u32 val, wml;
	int ret;

	esdhc_write32(esdhc, SDHCI_INT_ENABLE,
		      SDHCI_INT_CMD_COMPLETE | SDHCI_INT_XFER_COMPLETE |
		      SDHCI_INT_CARD_INT | SDHCI_INT_TIMEOUT | SDHCI_INT_CRC |
		      SDHCI_INT_END_BIT | SDHCI_INT_INDEX | SDHCI_INT_DATA_TIMEOUT |
		      SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_END_BIT | SDHCI_INT_DMA);

	wml = FIELD_PREP(WML_WR_BRST_LEN, 16)         |
	      FIELD_PREP(WML_WR_WML_MASK, SECTOR_WML) |
	      FIELD_PREP(WML_RD_BRST_LEN, 16)         |
	      FIELD_PREP(WML_RD_WML_MASK, SECTOR_WML);
	/*
	 * Some SoCs intrpret 0 as MAX value so for those cases the
	 * above value translates to zero
	 */
	if (esdhc->wrap_wml)
		wml = 0;

	esdhc_write32(esdhc, IMX_SDHCI_WML, wml);

	val = esdhc_read32(esdhc, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
	val |= SYSCTL_HCKEN | SYSCTL_IPGEN;
	esdhc_write32(esdhc, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET, val);

	cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = len / SECTOR_SIZE;
	data.blocksize = SECTOR_SIZE;
	data.flags = MMC_DATA_READ;

	ret = esdhc_send_cmd(esdhc, &cmd, &data);
	if (ret) {
		pr_debug("send command failed with %d\n", ret);
		return ret;
	}

	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1b;

	esdhc_send_cmd(esdhc, &cmd, NULL);

	return 0;
}

#ifdef CONFIG_ARCH_IMX
static int esdhc_search_header(struct esdhc *esdhc,
			       struct imx_flash_header_v2 **header_pointer,
			       void *buffer, u32 *offset)
{
	int ret;
	int i, header_count = 1;
	void *buf = buffer;
	struct imx_flash_header_v2 *hdr;

	for (i = 0; i < header_count; i++) {
		ret = esdhc_read_blocks(esdhc, buf,
					*offset + SZ_1K + SECTOR_SIZE);
		if (ret)
			return ret;

		hdr = buf + *offset + SZ_1K;

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
esdhc_start_image(struct esdhc *esdhc, ptrdiff_t address, ptrdiff_t entry,
		  u32 offset)
{

	void *buf = (void *)address;
	struct imx_flash_header_v2 *hdr = NULL;
	int ret, len;
	void __noreturn (*bb)(void);
	unsigned int ofs;

	len = imx_image_size();
	len = ALIGN(len, SECTOR_SIZE);

	ret = esdhc_search_header(esdhc, &hdr, buf, &offset);
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

	ret = esdhc_read_blocks(esdhc, buf, offset + len);
	if (ret) {
		pr_err("Loading image failed with %d\n", ret);
		return ret;
	}

	pr_debug("Image loaded successfully\n");

	bb = buf + ofs;

	sync_caches_for_execution();

	bb();
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
	struct esdhc esdhc;

	switch (instance) {
	case 0:
		esdhc.regs = IOMEM(MX6_USDHC1_BASE_ADDR);
		break;
	case 1:
		esdhc.regs = IOMEM(MX6_USDHC2_BASE_ADDR);
		break;
	case 2:
		esdhc.regs = IOMEM(MX6_USDHC3_BASE_ADDR);
		break;
	case 3:
		esdhc.regs = IOMEM(MX6_USDHC4_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	esdhc.is_be = 0;
	esdhc.is_mx6 = 1;
	esdhc.wrap_wml = false;

	return esdhc_start_image(&esdhc, 0x10000000, 0x10000000, 0);
}

/**
 * imx8_esdhc_start_image - Load and start an image from USDHC controller
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
int imx8_esdhc_start_image(int instance)
{
	struct esdhc esdhc;

	switch (instance) {
	case 0:
		esdhc.regs = IOMEM(MX8MQ_USDHC1_BASE_ADDR);
		break;
	case 1:
		esdhc.regs = IOMEM(MX8MQ_USDHC2_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	esdhc.is_be = 0;
	esdhc.is_mx6 = 1;
	esdhc.wrap_wml = false;

	return esdhc_start_image(&esdhc, MX8MQ_DDR_CSD1_BASE_ADDR,
				 MX8MQ_ATF_BL33_BASE_ADDR, SZ_32K);
}

int imx8_esdhc_load_piggy(int instance)
{
	void *buf, *piggy;
	struct imx_flash_header_v2 *hdr = NULL;
	struct esdhc esdhc;
	int ret, len;
	int offset = SZ_32K;

	switch (instance) {
	case 0:
		esdhc.regs = IOMEM(MX8MQ_USDHC1_BASE_ADDR);
		break;
	case 1:
		esdhc.regs = IOMEM(MX8MQ_USDHC2_BASE_ADDR);
		break;
	default:
		return -EINVAL;
	}

	esdhc.is_be = 0;
	esdhc.is_mx6 = 1;
	esdhc.wrap_wml = false;

	/*
	 * We expect to be running at MX8MQ_ATF_BL33_BASE_ADDR where the atf
	 * has jumped to. Use a temporary buffer where we won't overwrite
	 * ourselves.
	 */
	buf = (void *)MX8MQ_ATF_BL33_BASE_ADDR + SZ_32M;

	ret = esdhc_search_header(&esdhc, &hdr, buf, &offset);
	if (ret)
		return ret;

	len = offset + hdr->boot_data.size + piggydata_size();
	len = ALIGN(len, SECTOR_SIZE);

	ret = esdhc_read_blocks(&esdhc, buf, len);

	/*
	 * Calculate location of the piggydata at the offset loaded into RAM
	 */
	piggy = buf + offset + hdr->boot_data.size;

	/*
	 * Copy the piggydata where the uncompressing code expects it
	 */
	memcpy(input_data, piggy, piggydata_size());

	return ret;
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
	struct esdhc esdhc = {
		.regs = IOMEM(0x01560000),
		.is_be = true,
		.wrap_wml = true,
	};
	unsigned long sdram = 0x80000000;
	void (*barebox)(unsigned long, unsigned long, unsigned long) =
		(void *)(sdram + LS1046A_SD_IMAGE_OFFSET);

	/*
	 * The ROM leaves us here with a clock frequency of around 400kHz. Speed
	 * this up a bit. FIXME: The resulting frequency has not yet been verified
	 * to work on all cards.
	 */
	val = esdhc_read32(&esdhc, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET);
	val &= ~0x0000fff0;
	val |= (8 << 8) | (3 << 4);
	esdhc_write32(&esdhc, SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET, val);

	esdhc_write32(&esdhc, ESDHC_DMA_SYSCTL, ESDHC_SYSCTL_DMA_SNOOP);

	ret = esdhc_read_blocks(&esdhc, (void *)sdram,
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

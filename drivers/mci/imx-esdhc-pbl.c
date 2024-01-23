// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "xload-esdhc: " fmt

#include <common.h>
#include <io.h>
#include <mci.h>
#include <linux/sizes.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <mach/imx/xload.h>
#ifdef CONFIG_ARCH_IMX
#include <mach/imx/atf.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/imx7-regs.h>
#include <mach/imx/imx8mq-regs.h>
#include <mach/imx/imx8mm-regs.h>
#include <mach/imx/imx-header.h>
#endif
#ifdef CONFIG_ARCH_LS1046
#include <mach/layerscape/xload.h>
#endif
#include "sdhci.h"
#include "imx-esdhc.h"

#define SECTOR_SIZE 512
#define SECTOR_WML  (SECTOR_SIZE / sizeof(u32))

#define esdhc_send_cmd	__esdhc_send_cmd

static u8 ext_csd[512] __aligned(64);

static int esdhc_send_ext_csd(struct fsl_esdhc_host *host)
{
	struct mci_cmd cmd;
	struct mci_data data;

	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = sizeof(ext_csd);
	data.flags = MMC_DATA_READ;

	return esdhc_send_cmd(host, &cmd, &data);
}

static bool __maybe_unused esdhc_bootpart_active(struct fsl_esdhc_host *host)
{
	unsigned bootpart;

	int ret = esdhc_send_ext_csd(host);
	if (ret)
		return false;

	bootpart = (ext_csd[EXT_CSD_PARTITION_CONFIG] >> 3) & 0x7;
	if (bootpart == 1 || bootpart == 2)
		return true;

	return false;
}

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
static int imx_read_blocks(void *dest, size_t len, void *priv)
{
	return esdhc_read_blocks(priv, dest, len);
}

static int
esdhc_load_image(struct fsl_esdhc_host *host, ptrdiff_t address,
		 ptrdiff_t entry, u32 offset, u32 ivt_offset, bool start)
{
	return imx_load_image(address, entry, offset, ivt_offset, start,
			      SECTOR_SIZE, imx_read_blocks, host);
}

static void imx_esdhc_init(struct fsl_esdhc_host *host,
			   struct esdhc_soc_data *data)
{
	u32 mixctrl;

	data->flags = ESDHC_FLAG_USDHC;
	host->socdata = data;
	esdhc_populate_sdhci(host);

	sdhci_write32(&host->sdhci, IMX_SDHCI_WML,
		      FIELD_PREP(WML_WR_BRST_LEN, 16)         |
		      FIELD_PREP(WML_WR_WML_MASK, SECTOR_WML) |
		      FIELD_PREP(WML_RD_BRST_LEN, 16)         |
		      FIELD_PREP(WML_RD_WML_MASK, SECTOR_WML));

	mixctrl = sdhci_read32(&host->sdhci, IMX_SDHCI_MIXCTRL);
	if (mixctrl & MIX_CTRL_DDREN)
		host->sdhci.timing = MMC_TIMING_MMC_DDR52;
}

static int imx8m_esdhc_init(struct fsl_esdhc_host *host,
			    struct esdhc_soc_data *data,
			    int instance)
{
	switch (instance) {
	case 0:
		host->sdhci.base = IOMEM(MX8M_USDHC1_BASE_ADDR);
		break;
	case 1:
		host->sdhci.base = IOMEM(MX8M_USDHC2_BASE_ADDR);
		break;
	case 2:
		/* Only exists on i.MX8MM, not on i.MX8MQ */
		host->sdhci.base = IOMEM(MX8MM_USDHC3_BASE_ADDR);
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
	struct fsl_esdhc_host host = { 0 };

	switch (instance) {
	case 0:
		host.sdhci.base = IOMEM(MX6_USDHC1_BASE_ADDR);
		break;
	case 1:
		host.sdhci.base = IOMEM(MX6_USDHC2_BASE_ADDR);
		break;
	case 2:
		host.sdhci.base = IOMEM(MX6_USDHC3_BASE_ADDR);
		break;
	case 3:
		host.sdhci.base = IOMEM(MX6_USDHC4_BASE_ADDR);
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
	struct fsl_esdhc_host host = { 0 };

	switch (instance) {
	case 0:
		host.sdhci.base = IOMEM(MX7_USDHC1_BASE_ADDR);
		break;
	case 1:
		host.sdhci.base = IOMEM(MX7_USDHC2_BASE_ADDR);
		break;
	case 2:
		host.sdhci.base = IOMEM(MX7_USDHC3_BASE_ADDR);
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
 * @bl33: Where to load the bl33 barebox image
 *
 * This uses esdhc_start_image() to load an image from SD/MMC.  It is
 * assumed that the image is the currently running barebox image (This
 * information is used to calculate the length of the image). The
 * image is not started afterwards.
 *
 * Return: If image successfully loaded, returns 0.
 * A negative error code is returned when this function fails.
 */
int imx8m_esdhc_load_image(int instance, void *bl33)
{
	struct esdhc_soc_data data;
	struct fsl_esdhc_host host = { 0 };
	int ret;

	ret = imx8m_esdhc_init(&host, &data, instance);
	if (ret)
		return ret;

	return esdhc_load_image(&host, MX8M_DDR_CSD1_BASE_ADDR,
				(ptrdiff_t)bl33, SZ_32K, SZ_1K,
				false);
}

/**
 * imx8mp_esdhc_load_image - Load and optionally start an image from USDHC controller
 * @instance: The USDHC controller instance (0..2)
 * @bl33: Where to load the bl33 barebox image
 *
 * This uses esdhc_start_image() to load an image from SD/MMC.  It is
 * assumed that the image is the currently running barebox image (This
 * information is used to calculate the length of the image). The
 * image is not started afterwards.
 *
 * Return: If image successfully loaded, returns 0.
 * A negative error code is returned when this function fails.
 */
int imx8mp_esdhc_load_image(int instance, void *bl33)
{
	struct esdhc_soc_data data;
	struct fsl_esdhc_host host = { 0 };
	u32 offset;
	int ret;

	ret = imx8m_esdhc_init(&host, &data, instance);
	if (ret)
		return ret;

	offset = esdhc_bootpart_active(&host)? 0 : SZ_32K;

	return esdhc_load_image(&host, MX8M_DDR_CSD1_BASE_ADDR,
				(ptrdiff_t)bl33, offset, 0, false);
}

int imx8mn_esdhc_load_image(int instance, void *bl33)
	__alias(imx8mp_esdhc_load_image);
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
		.flags = ESDHC_FLAG_MULTIBLK_NO_INT | ESDHC_FLAG_BIGENDIAN,
	};
	struct fsl_esdhc_host host = {
		.sdhci.base = IOMEM(0x01560000),
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

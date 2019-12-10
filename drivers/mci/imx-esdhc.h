/*
 * FSL SD/MMC Defines
 *-------------------------------------------------------------------
 *
 * Copyright 2007-2008,2010 Freescale Semiconductor, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 *
 *-------------------------------------------------------------------
 *
 */

#ifndef  __FSL_ESDHC_H__
#define	__FSL_ESDHC_H__

#include <dma.h>
#include <errno.h>
#include <asm/byteorder.h>
#include <linux/bitfield.h>

#define SYSCTL_INITA		0x08000000
#define SYSCTL_TIMEOUT_MASK	0x000f0000
#define SYSCTL_CLOCK_MASK	0x0000fff0
#define SYSCTL_RSTA		0x01000000
#define SYSCTL_CKEN		0x00000008
#define SYSCTL_PEREN		0x00000004
#define SYSCTL_HCKEN		0x00000002
#define SYSCTL_IPGEN		0x00000001

#define CMD_ERR		(SDHCI_INT_INDEX | SDHCI_INT_END_BIT | SDHCI_INT_CRC)
#define DATA_ERR	(SDHCI_INT_DATA_END_BIT | SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT)

#define PROCTL_INIT		0x00000020
#define PROCTL_DTW_4		0x00000002
#define PROCTL_DTW_8		0x00000004

#define WML_WRITE	0x00010000
#define WML_RD_WML_MASK	0xff
#define WML_WR_BRST_LEN	GENMASK(28, 24)
#define WML_WR_WML_MASK	0xff0000
#define WML_RD_BRST_LEN	GENMASK(12, 8)

#define BLKATTR_CNT(x)	((x & 0xffff) << 16)
#define BLKATTR_SIZE(x)	(x & 0x1fff)
#define MAX_BLK_CNT	0x7fff	/* so malloc will have enough room with 32M */

#define ESDHC_HOSTCAPBLT_VS18	0x04000000
#define ESDHC_HOSTCAPBLT_VS30	0x02000000
#define ESDHC_HOSTCAPBLT_VS33	0x01000000
#define ESDHC_HOSTCAPBLT_SRS	0x00800000
#define ESDHC_HOSTCAPBLT_DMAS	0x00400000
#define ESDHC_HOSTCAPBLT_HSS	0x00200000

#define PIO_TIMEOUT		100000

#define IMX_SDHCI_WML		0x44
#define IMX_SDHCI_MIXCTRL	0x48
#define IMX_SDHCI_DLL_CTRL	0x60
#define IMX_SDHCI_MIX_CTRL_FBCLK_SEL	BIT(25)

#define ESDHC_DMA_SYSCTL	0x40c /* Layerscape specific */
#define ESDHC_SYSCTL_DMA_SNOOP 	BIT(6)


/*
 * The CMDTYPE of the CMD register (offset 0xE) should be set to
 * "11" when the STOP CMD12 is issued on imx53 to abort one
 * open ended multi-blk IO. Otherwise the TC INT wouldn't
 * be generated.
 * In exact block transfer, the controller doesn't complete the
 * operations automatically as required at the end of the
 * transfer and remains on hold if the abort command is not sent.
 * As a result, the TC flag is not asserted and SW  received timeout
 * exeception. Bit1 of Vendor Spec registor is used to fix it.
 */
#define ESDHC_FLAG_MULTIBLK_NO_INT	BIT(1)
/*
 * The flag enables the workaround for ESDHC errata ENGcm07207 which
 * affects i.MX25 and i.MX35.
 */
#define ESDHC_FLAG_ENGCM07207		BIT(2)
/*
 * The flag tells that the ESDHC controller is an USDHC block that is
 * integrated on the i.MX6 series.
 */
#define ESDHC_FLAG_USDHC		BIT(3)
/* The IP supports manual tuning process */
#define ESDHC_FLAG_MAN_TUNING		BIT(4)
/* The IP supports standard tuning process */
#define ESDHC_FLAG_STD_TUNING		BIT(5)
/* The IP has SDHCI_CAPABILITIES_1 register */
#define ESDHC_FLAG_HAVE_CAP1		BIT(6)

/*
 * The IP has errata ERR004536
 * uSDHC: ADMA Length Mismatch Error occurs if the AHB read access is slow,
 * when reading data from the card
 */
#define ESDHC_FLAG_ERR004536		BIT(7)
/* The IP supports HS200 mode */
#define ESDHC_FLAG_HS200		BIT(8)
/* The IP supports HS400 mode */
#define ESDHC_FLAG_HS400		BIT(9)
/* Need to access registers in bigendian mode */
#define ESDHC_FLAG_BIGENDIAN		BIT(10)
/* Enable cache snooping */
#define ESDHC_FLAG_CACHE_SNOOPING	BIT(11)

struct esdhc_soc_data {
	u32 flags;
	const char *clkidx;
};

struct fsl_esdhc_host {
	struct mci_host		mci;
	struct clk		*clk;
	struct device_d		*dev;
	void __iomem		*regs;
	const struct esdhc_soc_data *socdata;
	struct sdhci	sdhci;
};

static inline int esdhc_is_usdhc(struct fsl_esdhc_host *data)
{
	return !!(data->socdata->flags & ESDHC_FLAG_USDHC);
}

static inline struct fsl_esdhc_host *sdhci_to_esdhc(struct sdhci *sdhci)
{
	return container_of(sdhci, struct fsl_esdhc_host, sdhci);
}

static inline void
esdhc_clrsetbits32(struct fsl_esdhc_host *host, unsigned int reg,
		   u32 clear, u32 set)
{
	u32 val;

	val = sdhci_read32(&host->sdhci, reg);
	val &= ~clear;
	val |= set;
	sdhci_write32(&host->sdhci, reg, val);
}

static inline void
esdhc_clrbits32(struct fsl_esdhc_host *host, unsigned int reg,
		u32 clear)
{
	esdhc_clrsetbits32(host, reg, clear, 0);
}

static inline void
esdhc_setbits32(struct fsl_esdhc_host *host, unsigned int reg,
		u32 set)
{
	esdhc_clrsetbits32(host, reg, 0, set);
}

void esdhc_populate_sdhci(struct fsl_esdhc_host *host);
int esdhc_poll(struct fsl_esdhc_host *host, unsigned int off,
	       unsigned int mask, unsigned int val,
	       uint64_t timeout);
int __esdhc_send_cmd(struct fsl_esdhc_host *host, struct mci_cmd *cmd,
		     struct mci_data *data);

#endif  /* __FSL_ESDHC_H__ */

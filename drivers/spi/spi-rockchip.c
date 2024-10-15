// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014, Fuzhou Rockchip Electronics Co., Ltd
 * Author: Addy Ke <addy.ke@rock-chips.com>
 */

#include <common.h>
#include <linux/clk.h>
#include <driver.h>
#include <errno.h>
#include <spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/bitops.h>
#include <clock.h>
#include <gpio.h>
#include <of_gpio.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>

#define DRIVER_NAME "rockchip-spi"

#define ROCKCHIP_SPI_CLR_BITS(reg, bits) \
		writel_relaxed(readl_relaxed(reg) & ~(bits), reg)
#define ROCKCHIP_SPI_SET_BITS(reg, bits) \
		writel_relaxed(readl_relaxed(reg) | (bits), reg)

/* SPI register offsets */
#define ROCKCHIP_SPI_CTRLR0			0x0000
#define ROCKCHIP_SPI_CTRLR1			0x0004
#define ROCKCHIP_SPI_SSIENR			0x0008
#define ROCKCHIP_SPI_SER			0x000c
#define ROCKCHIP_SPI_BAUDR			0x0010
#define ROCKCHIP_SPI_TXFTLR			0x0014
#define ROCKCHIP_SPI_RXFTLR			0x0018
#define ROCKCHIP_SPI_TXFLR			0x001c
#define ROCKCHIP_SPI_RXFLR			0x0020
#define ROCKCHIP_SPI_SR				0x0024
#define ROCKCHIP_SPI_IPR			0x0028
#define ROCKCHIP_SPI_IMR			0x002c
#define ROCKCHIP_SPI_ISR			0x0030
#define ROCKCHIP_SPI_RISR			0x0034
#define ROCKCHIP_SPI_ICR			0x0038
#define ROCKCHIP_SPI_DMACR			0x003c
#define ROCKCHIP_SPI_DMATDLR			0x0040
#define ROCKCHIP_SPI_DMARDLR			0x0044
#define ROCKCHIP_SPI_VERSION			0x0048
#define ROCKCHIP_SPI_TXDR			0x0400
#define ROCKCHIP_SPI_RXDR			0x0800

/* Bit fields in CTRLR0 */
#define CR0_DFS_OFFSET				0
#define CR0_DFS_4BIT				0x0
#define CR0_DFS_8BIT				0x1
#define CR0_DFS_16BIT				0x2

#define CR0_CFS_OFFSET				2

#define CR0_SCPH_OFFSET				6

#define CR0_SCPOL_OFFSET			7

#define CR0_CSM_OFFSET				8
#define CR0_CSM_KEEP				0x0
/* ss_n be high for half sclk_out cycles */
#define CR0_CSM_HALF				0X1
/* ss_n be high for one sclk_out cycle */
#define CR0_CSM_ONE					0x2

/* ss_n to sclk_out delay */
#define CR0_SSD_OFFSET				10
/*
 * The period between ss_n active and
 * sclk_out active is half sclk_out cycles
 */
#define CR0_SSD_HALF				0x0
/*
 * The period between ss_n active and
 * sclk_out active is one sclk_out cycle
 */
#define CR0_SSD_ONE					0x1

#define CR0_EM_OFFSET				11
#define CR0_EM_LITTLE				0x0
#define CR0_EM_BIG					0x1

#define CR0_FBM_OFFSET				12
#define CR0_FBM_MSB					0x0
#define CR0_FBM_LSB					0x1

#define CR0_BHT_OFFSET				13
#define CR0_BHT_16BIT				0x0
#define CR0_BHT_8BIT				0x1

#define CR0_RSD_OFFSET				14
#define CR0_RSD_MAX				0x3

#define CR0_FRF_OFFSET				16
#define CR0_FRF_SPI					0x0
#define CR0_FRF_SSP					0x1
#define CR0_FRF_MICROWIRE			0x2

#define CR0_XFM_OFFSET				18
#define CR0_XFM_MASK				(0x03 << SPI_XFM_OFFSET)
#define CR0_XFM_TR					0x0
#define CR0_XFM_TO					0x1
#define CR0_XFM_RO					0x2

#define CR0_OPM_OFFSET				20
#define CR0_OPM_HOST				0x0
#define CR0_OPM_TARGET				0x1

#define CR0_SOI_OFFSET				23

#define CR0_MTM_OFFSET				0x21

/* Bit fields in SER, 2bit */
#define SER_MASK					0x3

/* Bit fields in BAUDR */
#define BAUDR_SCKDV_MIN				2
#define BAUDR_SCKDV_MAX				65534

/* Bit fields in SR, 6bit */
#define SR_MASK						0x3f
#define SR_BUSY						(1 << 0)
#define SR_TF_FULL					(1 << 1)
#define SR_TF_EMPTY					(1 << 2)
#define SR_RF_EMPTY					(1 << 3)
#define SR_RF_FULL					(1 << 4)
#define SR_TARGET_TX_BUSY				(1 << 5)

/* Bit fields in ISR, IMR, ISR, RISR, 5bit */
#define INT_MASK					0x1f
#define INT_TF_EMPTY				(1 << 0)
#define INT_TF_OVERFLOW				(1 << 1)
#define INT_RF_UNDERFLOW			(1 << 2)
#define INT_RF_OVERFLOW				(1 << 3)
#define INT_RF_FULL				(1 << 4)
#define INT_CS_INACTIVE				(1 << 6)

/* Bit fields in ICR, 4bit */
#define ICR_MASK					0x0f
#define ICR_ALL						(1 << 0)
#define ICR_RF_UNDERFLOW			(1 << 1)
#define ICR_RF_OVERFLOW				(1 << 2)
#define ICR_TF_OVERFLOW				(1 << 3)

/* Bit fields in DMACR */
#define RF_DMA_EN					(1 << 0)
#define TF_DMA_EN					(1 << 1)

/* Driver state flags */
#define RXDMA					(1 << 0)
#define TXDMA					(1 << 1)

/* sclk_out: spi host internal logic in rk3x can support 50Mhz */
#define MAX_SCLK_OUT				50000000U

/*
 * SPI_CTRLR1 is 16-bits, so we should support lengths of 0xffff + 1. However,
 * the controller seems to hang when given 0x10000, so stick with this for now.
 */
#define ROCKCHIP_SPI_MAX_TRANLEN		0xffff

#define ROCKCHIP_SPI_MAX_NATIVE_CS_NUM		2
#define ROCKCHIP_SPI_VER2_TYPE1			0x05EC0002
#define ROCKCHIP_SPI_VER2_TYPE2			0x00110002

#define ROCKCHIP_AUTOSUSPEND_TIMEOUT		2000

struct rockchip_spi {
	struct spi_controller ctlr;
	struct device *dev;

	struct clk *spiclk;
	struct clk *apb_pclk;

	void __iomem *regs;

	/*depth of the FIFO buffer */
	u32 fifo_len;
	/* frequency of spiclk */
	u32 freq;

	u8 rsd;

	bool cs_high_supported; /* native CS supports active-high polarity */

	struct spi_transfer *xfer; /* Store xfer temporarily */
};

static inline void spi_enable_chip(struct rockchip_spi *rs, bool enable)
{
	writel_relaxed((enable ? 1U : 0U), rs->regs + ROCKCHIP_SPI_SSIENR);
}

static inline void wait_for_tx_idle(struct rockchip_spi *rs, bool target_mode)
{
	u64 start = get_time_ns();

	do {
		if (target_mode) {
			if (!(readl_relaxed(rs->regs + ROCKCHIP_SPI_SR) & SR_TARGET_TX_BUSY) &&
			    !((readl_relaxed(rs->regs + ROCKCHIP_SPI_SR) & SR_BUSY)))
				return;
		} else {
			if (!(readl_relaxed(rs->regs + ROCKCHIP_SPI_SR) & SR_BUSY))
				return;
		}
	} while (!is_timeout(start, 5 * MSECOND));

	dev_warn(rs->dev, "spi controller is in busy state!\n");
}

static u32 get_fifo_len(struct rockchip_spi *rs)
{
	u32 ver;

	ver = readl_relaxed(rs->regs + ROCKCHIP_SPI_VERSION);

	switch (ver) {
	case ROCKCHIP_SPI_VER2_TYPE1:
	case ROCKCHIP_SPI_VER2_TYPE2:
		return 64;
	default:
		return 32;
	}
}

static inline struct gpio_desc *spi_get_csgpiod(const struct spi_device *spi, u8 idx)
{
	return NULL;
}

static void rockchip_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct spi_controller *ctlr = spi->controller;
	struct rockchip_spi *rs = spi_controller_get_devdata(ctlr);
	bool cs_asserted = spi->mode & SPI_CS_HIGH ? !enable : enable;

	if (cs_asserted) {
		if (spi_get_csgpiod(spi, 0))
			ROCKCHIP_SPI_SET_BITS(rs->regs + ROCKCHIP_SPI_SER, 1);
		else
			ROCKCHIP_SPI_SET_BITS(rs->regs + ROCKCHIP_SPI_SER,
					      BIT(spi_get_chipselect(spi, 0)));
	} else {
		if (spi_get_csgpiod(spi, 0))
			ROCKCHIP_SPI_CLR_BITS(rs->regs + ROCKCHIP_SPI_SER, 1);
		else
			ROCKCHIP_SPI_CLR_BITS(rs->regs + ROCKCHIP_SPI_SER,
					      BIT(spi_get_chipselect(spi, 0)));
	}
}

static int rockchip_spi_pio(struct rockchip_spi *rs, struct spi_transfer *xfer)
{
	int bytes_per_word = xfer->bits_per_word <= 8 ? 1 : 2;
	const void *txbuf = xfer->tx_buf;
	void *rxbuf = xfer->rx_buf;
	int rxwords, txwords;

	txwords = rxwords = xfer->len / bytes_per_word;

	while (1) {
		unsigned int fifowords = readl_relaxed(rs->regs + ROCKCHIP_SPI_TXFLR);
		unsigned int tx_free = rs->fifo_len - fifowords;

		if (tx_free && txwords) {
			u32 txw;

			if (!txbuf)
				txw = 0;
			else if (bytes_per_word == 1)
				txw = *(u8 *)txbuf;
			else
				txw = *(u16 *)txbuf;

			writel_relaxed(txw, rs->regs + ROCKCHIP_SPI_TXDR);

			if (txbuf)
				txbuf += bytes_per_word;

			txwords--;
		}

		fifowords = readl_relaxed(rs->regs + ROCKCHIP_SPI_RXFLR);

		if (fifowords) {
			u32 rxw = readl_relaxed(rs->regs + ROCKCHIP_SPI_RXDR);

			if (rxbuf) {
				if (bytes_per_word == 1)
					*(u8 *)rxbuf = (u8)rxw;
				else
					*(u16 *)rxbuf = (u16)rxw;
				rxbuf += bytes_per_word;
			}
			
			rxwords--;
		}

		if (!rxwords)
			return 0;
	}
}

static int rockchip_spi_config(struct rockchip_spi *rs,
		struct spi_device *spi, struct spi_transfer *xfer)
{
	u32 cr0 = CR0_FRF_SPI  << CR0_FRF_OFFSET
		| CR0_BHT_8BIT << CR0_BHT_OFFSET
		| CR0_SSD_ONE  << CR0_SSD_OFFSET
		| CR0_EM_BIG   << CR0_EM_OFFSET;

	cr0 |= rs->rsd << CR0_RSD_OFFSET;
	cr0 |= (spi->mode & 0x3U) << CR0_SCPH_OFFSET;
	if (spi->mode & SPI_LSB_FIRST)
		cr0 |= CR0_FBM_LSB << CR0_FBM_OFFSET;
	if (spi->mode & SPI_CS_HIGH)
		cr0 |= BIT(spi_get_chipselect(spi, 0)) << CR0_SOI_OFFSET;

	if (xfer->rx_buf && xfer->tx_buf)
		cr0 |= CR0_XFM_TR << CR0_XFM_OFFSET;
	else if (xfer->rx_buf)
		cr0 |= CR0_XFM_RO << CR0_XFM_OFFSET;

	cr0 |= CR0_XFM_TR << CR0_XFM_OFFSET;

	switch (xfer->bits_per_word) {
	case 4:
		cr0 |= CR0_DFS_4BIT << CR0_DFS_OFFSET;
		break;
	case 8:
		cr0 |= CR0_DFS_8BIT << CR0_DFS_OFFSET;
		break;
	case 16:
		cr0 |= CR0_DFS_16BIT << CR0_DFS_OFFSET;
		break;
	default:
		/* we only whitelist 4, 8 and 16 bit words in
		 * ctlr->bits_per_word_mask, so this shouldn't
		 * happen
		 */
		dev_err(rs->dev, "unknown bits per word: %d\n",
			xfer->bits_per_word);
		return -EINVAL;
	}

	writel_relaxed(cr0, rs->regs + ROCKCHIP_SPI_CTRLR0);

	/* the hardware only supports an even clock divisor, so
	 * round divisor = spiclk / speed up to nearest even number
	 * so that the resulting speed is <= the requested speed
	 */
	writel_relaxed(2 * DIV_ROUND_UP(rs->freq, 2 * xfer->speed_hz),
			rs->regs + ROCKCHIP_SPI_BAUDR);

	return 0;
}

static size_t rockchip_spi_max_transfer_size(struct spi_device *spi)
{
	return ROCKCHIP_SPI_MAX_TRANLEN;
}

static int rockchip_spi_transfer_one(
		struct rockchip_spi *rs,
		struct spi_device *spi,
		struct spi_transfer *xfer)
{
	int ret;

	/* Zero length transfers won't trigger an interrupt on completion */
	if (!xfer->len)
		return 1;

	if (!xfer->tx_buf && !xfer->rx_buf) {
		dev_err(rs->dev, "No buffer for transfer\n");
		return -EINVAL;
	}

	if (xfer->len > ROCKCHIP_SPI_MAX_TRANLEN) {
		dev_err(rs->dev, "Transfer is too long (%d)\n", xfer->len);
		return -EINVAL;
	}

	ret = rockchip_spi_config(rs, spi, xfer);
	if (ret)
		return ret;

	spi_enable_chip(rs, true);
	rockchip_spi_pio(rs, xfer);
	spi_enable_chip(rs, false);

	return 0;
}

static int rockchip_spi_transfer(struct spi_device *spi_dev, struct spi_message *msg)
{
	struct rockchip_spi *rs = spi_controller_get_devdata(spi_dev->controller);
	struct spi_transfer *t;
	int ret = 0;

	if (list_empty(&msg->transfers))
		return 0;

	rockchip_spi_set_cs(spi_dev, true);

	msg->actual_length = 0;

	list_for_each_entry(t, &msg->transfers, transfer_list) {
		dev_dbg(rs->dev, "  xfer %p: len %u tx %p rx %p\n",
			t, t->len, t->tx_buf, t->rx_buf);

		ret = rockchip_spi_transfer_one(rs, spi_dev, t);
		if (ret < 0)
			goto out;
		msg->actual_length += t->len;
	}

out:
	rockchip_spi_set_cs(spi_dev, false);
	return ret;
}

static int rockchip_spi_setup(struct spi_device *spi)
{
	struct rockchip_spi *rs = spi_controller_get_devdata(spi->controller);
	u32 cr0;

	if (!spi_get_csgpiod(spi, 0) && (spi->mode & SPI_CS_HIGH) && !rs->cs_high_supported) {
		dev_warn(&spi->dev, "setup: non GPIO CS can't be active-high\n");
		return -EINVAL;
	}

	cr0 = readl_relaxed(rs->regs + ROCKCHIP_SPI_CTRLR0);

	cr0 &= ~(0x3 << CR0_SCPH_OFFSET);
	cr0 |= ((spi->mode & 0x3) << CR0_SCPH_OFFSET);
	if (spi->mode & SPI_CS_HIGH && spi_get_chipselect(spi, 0) <= 1)
		cr0 |= BIT(spi_get_chipselect(spi, 0)) << CR0_SOI_OFFSET;
	else if (spi_get_chipselect(spi, 0) <= 1)
		cr0 &= ~(BIT(spi_get_chipselect(spi, 0)) << CR0_SOI_OFFSET);

	writel_relaxed(cr0, rs->regs + ROCKCHIP_SPI_CTRLR0);

	return 0;
}

static int rockchip_spi_probe(struct device *dev)
{
	int ret;
	struct rockchip_spi *rs;
	struct spi_controller *ctlr;
	struct device_node *np = dev->of_node;
	u32 rsd_nsecs, num_cs;
	bool target_mode;
	struct resource *iores;

	target_mode = of_property_read_bool(np, "spi-slave");
	if (target_mode)
		return 0;

	rs = xzalloc(sizeof(*rs));
	ctlr = &rs->ctlr;

	/* Get basic io resource and map it */
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	rs->regs = IOMEM(iores->start);

	rs->apb_pclk = clk_get_enabled(dev, "apb_pclk");
	if (IS_ERR(rs->apb_pclk)) {
		dev_err(dev, "Failed to get apb_pclk\n");
		ret = PTR_ERR(rs->apb_pclk);
		goto err_put_ctlr;
	}

	rs->spiclk = clk_get_enabled(dev, "spiclk");
	if (IS_ERR(rs->spiclk)) {
		dev_err(dev, "Failed to get spi_pclk\n");
		ret = PTR_ERR(rs->spiclk);
		goto err_put_ctlr;
	}

	spi_enable_chip(rs, false);

	rs->dev = dev;
	rs->freq = clk_get_rate(rs->spiclk);

	if (!of_property_read_u32(np, "rx-sample-delay-ns",
				  &rsd_nsecs)) {
		/* rx sample delay is expressed in parent clock cycles (max 3) */
		u32 rsd = DIV_ROUND_CLOSEST(rsd_nsecs * (rs->freq >> 8),
				1000000000 >> 8);
		if (!rsd) {
			dev_warn(rs->dev, "%u Hz are too slow to express %u ns delay\n",
					rs->freq, rsd_nsecs);
		} else if (rsd > CR0_RSD_MAX) {
			rsd = CR0_RSD_MAX;
			dev_warn(rs->dev, "%u Hz are too fast to express %u ns delay, clamping at %u ns\n",
					rs->freq, rsd_nsecs,
					CR0_RSD_MAX * 1000000000U / rs->freq);
		}
		rs->rsd = rsd;
	}

	rs->fifo_len = get_fifo_len(rs);
	if (!rs->fifo_len) {
		dev_err(dev, "Failed to get fifo length\n");
		ret = -EINVAL;
		goto err_put_ctlr;
	}

//	ctlr->bus_num = pdev->id;
//	ctlr->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LOOP | SPI_LSB_FIRST;

	/*
	 * rk spi0 has two native cs, spi1..5 one cs only
	 * if num-cs is missing in the dts, default to 1
	 */
	num_cs = 1;
	of_property_read_u32(np, "num-cs", &num_cs);
	ctlr->num_chipselect = num_cs;

	ctlr->dev = dev;
	ctlr->bits_per_word_mask = SPI_BPW_MASK(16) | SPI_BPW_MASK(8) | SPI_BPW_MASK(4);
	ctlr->max_speed_hz = min(rs->freq / BAUDR_SCKDV_MIN, MAX_SCLK_OUT);

	ctlr->setup = rockchip_spi_setup;
	ctlr->transfer = rockchip_spi_transfer;
	ctlr->max_transfer_size = rockchip_spi_max_transfer_size;

	switch (readl_relaxed(rs->regs + ROCKCHIP_SPI_VERSION)) {
	case ROCKCHIP_SPI_VER2_TYPE2:
		rs->cs_high_supported = true;
		break;
	}

	spi_controller_set_devdata(ctlr, rs);

	ret = spi_register_controller(ctlr);
	if (ret < 0) {
		dev_err(dev, "Failed to register controller\n");
		goto err_free_dma_rx;
	}

	return 0;

err_free_dma_rx:
err_put_ctlr:

	return ret;
}

static const struct of_device_id rockchip_spi_dt_match[] = {
	{ .compatible = "rockchip,px30-spi", },
	{ .compatible = "rockchip,rk3036-spi", },
	{ .compatible = "rockchip,rk3066-spi", },
	{ .compatible = "rockchip,rk3188-spi", },
	{ .compatible = "rockchip,rk3228-spi", },
	{ .compatible = "rockchip,rk3288-spi", },
	{ .compatible = "rockchip,rk3308-spi", },
	{ .compatible = "rockchip,rk3328-spi", },
	{ .compatible = "rockchip,rk3368-spi", },
	{ .compatible = "rockchip,rk3399-spi", },
	{ .compatible = "rockchip,rv1108-spi", },
	{ .compatible = "rockchip,rv1126-spi", },
	{ },
};
MODULE_DEVICE_TABLE(of, rockchip_spi_dt_match);

static struct driver rockchip_spi_driver = {
        .name  = DRIVER_NAME,
        .probe = rockchip_spi_probe,
        .of_compatible = rockchip_spi_dt_match,
};
coredevice_platform_driver(rockchip_spi_driver);

MODULE_AUTHOR("Addy Ke <addy.ke@rock-chips.com>");
MODULE_DESCRIPTION("ROCKCHIP SPI Controller Driver");
MODULE_LICENSE("GPL v2");

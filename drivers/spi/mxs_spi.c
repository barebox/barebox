/*
 * Freescale i.MX28 SPI driver
 *
 * Copyright (C) 2013 Michael Grzeschik <mgr@pengutronix.de>
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <spi/spi.h>
#include <clock.h>
#include <errno.h>
#include <io.h>
#include <stmp-device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <mach/ssp.h>

#define	MXS_SPI_MAX_TIMEOUT		(10 * MSECOND)

#define	SPI_XFER_BEGIN	0x01 /* Assert CS before transfer */
#define	SPI_XFER_END	0x02 /* Deassert CS after transfer */

struct mxs_spi {
	struct spi_master	master;
	uint32_t		max_khz;
	uint32_t		mode;
	struct clk		*clk;
	void __iomem		*regs;
};

static inline struct mxs_spi *to_mxs(struct spi_master *master)
{
	return container_of(master, struct mxs_spi, master);
}

/*
 * Set SSP/MMC bus frequency, in kHz
 */
static void imx_set_ssp_busclock(struct spi_master *master, uint32_t freq)
{
	struct mxs_spi *mxs = to_mxs(master);
	const uint32_t sspclk = clk_get_rate(mxs->clk);
	uint32_t val;
	uint32_t divide, rate, tgtclk;

	/*
	 * SSP bit rate = SSPCLK / (CLOCK_DIVIDE * (1 + CLOCK_RATE)),
	 * CLOCK_DIVIDE has to be an even value from 2 to 254, and
	 * CLOCK_RATE could be any integer from 0 to 255.
	 */
	for (divide = 2; divide < 254; divide += 2) {
		rate = sspclk / freq / divide;
		if (rate <= 256)
			break;
	}

	tgtclk = sspclk / divide / rate;
	while (tgtclk > freq) {
		rate++;
		tgtclk = sspclk / divide / rate;
	}
	if (rate > 256)
		rate = 256;

	/* Always set timeout the maximum */
	val = SSP_TIMING_TIMEOUT_MASK |
		SSP_TIMING_CLOCK_DIVIDE(divide) |
		SSP_TIMING_CLOCK_RATE(rate - 1);
	writel(val, mxs->regs + HW_SSP_TIMING);

	dev_dbg(master->dev, "SPI%d: Set freq rate to %d KHz (requested %d KHz)\n",
		master->bus_num, tgtclk, freq);
}

static int mxs_spi_setup(struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	struct mxs_spi *mxs = to_mxs(master);
	uint32_t val = 0;

	/* MXS SPI: 4 ports and 3 chip selects maximum */
	if (master->bus_num > 3 || spi->chip_select > 2) {
		dev_err(master->dev, "mxs_spi: invalid bus %d / chip select %d\n",
			 master->bus_num, spi->chip_select);
		return -EINVAL;
	}

	stmp_reset_block(mxs->regs + HW_SSP_CTRL0, 0);

	val |= SSP_CTRL0_SSP_ASSERT_OUT(spi->chip_select);
	val |= SSP_CTRL0_BUS_WIDTH(0);
	writel(val, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

	val = SSP_CTRL1_SSP_MODE(0) | SSP_CTRL1_WORD_LENGTH(7);
	val |= (mxs->mode & SPI_CPOL) ? SSP_CTRL1_POLARITY : 0;
	val |= (mxs->mode & SPI_CPHA) ? SSP_CTRL1_PHASE : 0;
	writel(val, mxs->regs + HW_SSP_CTRL1);

	writel(0x0, mxs->regs + HW_SSP_CMD0);
	writel(0x0, mxs->regs + HW_SSP_CMD1);

	imx_set_ssp_busclock(master, spi->max_speed_hz);

	return 0;
}

static void mxs_spi_start_xfer(struct mxs_spi *mxs)
{
	writel(SSP_CTRL0_LOCK_CS, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
	writel(SSP_CTRL0_IGNORE_CRC, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
}

static void mxs_spi_end_xfer(struct mxs_spi *mxs)
{
	writel(SSP_CTRL0_LOCK_CS, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
	writel(SSP_CTRL0_IGNORE_CRC, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
}

static void mxs_spi_set_cs(struct spi_device *spi)
{
	struct mxs_spi *mxs = to_mxs(spi->master);
	const uint32_t mask = SSP_CTRL0_WAIT_FOR_CMD | SSP_CTRL0_WAIT_FOR_IRQ;
	uint32_t select = SSP_CTRL0_SSP_ASSERT_OUT(spi->chip_select);

	writel(mask, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
	writel(select, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);
}

static int mxs_spi_xfer_pio(struct spi_device *spi,
			char *data, int length, int write, unsigned long flags)
{
	struct mxs_spi *mxs = to_mxs(spi->master);
	struct spi_master *master = spi->master;

	if (flags & SPI_XFER_BEGIN)
		mxs_spi_start_xfer(mxs);

	mxs_spi_set_cs(spi);

	while (length--) {
		if ((flags & SPI_XFER_END) && !length)
			mxs_spi_end_xfer(mxs);

		/* We transfer 1 byte */
		writel(1, mxs->regs + HW_SSP_XFER_COUNT);

		if (write)
			writel(SSP_CTRL0_READ, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_CLR);
		else
			writel(SSP_CTRL0_READ, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

		writel(SSP_CTRL0_RUN, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

		if (wait_on_timeout(MXS_SPI_MAX_TIMEOUT,
				(readl(mxs->regs + HW_SSP_CTRL0) & SSP_CTRL0_RUN) == SSP_CTRL0_RUN)) {
			dev_err(master->dev, "MXS SPI: Timeout waiting for start\n");
			return -ETIMEDOUT;
		}

		if (write)
			writel(*data++, mxs->regs + HW_SSP_DATA);

		writel(SSP_CTRL0_DATA_XFER, mxs->regs + HW_SSP_CTRL0 + STMP_OFFSET_REG_SET);

		if (!write) {
			if (wait_on_timeout(MXS_SPI_MAX_TIMEOUT,
				!(readl(mxs->regs + HW_SSP_STATUS) & SSP_STATUS_FIFO_EMPTY))) {
				dev_err(master->dev, "MXS SPI: Timeout waiting for data\n");
				return -ETIMEDOUT;
			}

			*data++ = readl(mxs->regs + HW_SSP_DATA) & 0xff;
		}

		if (wait_on_timeout(MXS_SPI_MAX_TIMEOUT,
			!(readl(mxs->regs + HW_SSP_CTRL0) & SSP_CTRL0_RUN))) {
			dev_err(master->dev, "MXS SPI: Timeout waiting for finish\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int mxs_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct mxs_spi *mxs = to_mxs(spi->master);
	struct spi_master *master = spi->master;
	struct spi_transfer *t = NULL;
	char dummy;
	unsigned long flags = 0;
	int write = 0;
	char *data = NULL;
	int ret;
	mesg->actual_length = 0;

	list_for_each_entry(t, &mesg->transfers, transfer_list) {
		flags = 0;

		if (t->tx_buf) {
			data = (char *) t->tx_buf;
			write = 1;
		} else if (t->rx_buf) {
			data = (char *) t->rx_buf;
			write = 0;
		} else if (t->rx_buf && t->tx_buf) {
			dev_err(master->dev, "Cannot send and receive simultaneously\n");
			return -EIO;
		} else if (!t->rx_buf && !t->tx_buf) {
			dev_err(master->dev, "No Data\n");
			return -EIO;
		}

		if (&t->transfer_list == mesg->transfers.next)
			flags |= SPI_XFER_BEGIN;

		if (&t->transfer_list == mesg->transfers.prev)
			flags |= SPI_XFER_END;

		if (t->len == 0) {
			if (flags == SPI_XFER_END) {
				t->len = 1;
				t->rx_buf = (void *) &dummy;
			} else {
				return 0;
			}
		}

		writel(SSP_CTRL1_DMA_ENABLE, mxs->regs + HW_SSP_CTRL1 + STMP_OFFSET_REG_CLR);
		ret = mxs_spi_xfer_pio(spi, data, t->len, write, flags);
		if (ret < 0)
			return ret;
		mesg->actual_length += t->len;
	}

	return 0;
}

static int mxs_spi_probe(struct device_d *dev)
{
	struct spi_master *master;
	struct mxs_spi *mxs;

	mxs = xzalloc(sizeof(*mxs));

	master = &mxs->master;
	master->dev = dev;

	master->bus_num = dev->id;
	master->setup = mxs_spi_setup;
	master->transfer = mxs_spi_transfer;
	master->num_chipselect = 3;
	mxs->mode = SPI_CPOL | SPI_CPHA;

	mxs->regs = dev_request_mem_region(dev, 0);
	mxs->clk = clk_get(dev, NULL);
	if (IS_ERR(mxs->clk))
		return PTR_ERR(mxs->clk);
	clk_enable(mxs->clk);

	spi_register_master(master);

	return 0;
}

static struct driver_d mxs_spi_driver = {
	.name  = "mxs_spi",
	.probe = mxs_spi_probe,
};
device_platform_driver(mxs_spi_driver);

MODULE_AUTHOR("Denx Software Engeneering and Michael Grzeschik");
MODULE_DESCRIPTION("MXS SPI driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for Broadcom BCM2835 SPI Controllers
 *
 * Copyright (C) 2012 Chris Boot
 * Copyright (C) 2013 Stephen Warren
 * Copyright (C) 2015 Martin Sperl
 *
 * This driver is inspired by:
 * spi-ath79.c, Copyright (C) 2009-2011 Gabor Juhos <juhosg@openwrt.org>
 * spi-atmel.c, Copyright (C) 2006 Atmel Corporation
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <spi/spi.h>

/* SPI register offsets */
#define BCM2835_SPI_CS			0x00
#define BCM2835_SPI_FIFO		0x04
#define BCM2835_SPI_CLK			0x08
#define BCM2835_SPI_DLEN		0x0c

/* Bitfields in CS */
#define BCM2835_SPI_CS_TXD		0x00040000
#define BCM2835_SPI_CS_RXD		0x00020000
#define BCM2835_SPI_CS_DONE		0x00010000
#define BCM2835_SPI_CS_REN		0x00001000
#define BCM2835_SPI_CS_ADCS		0x00000800
#define BCM2835_SPI_CS_INTR		0x00000400
#define BCM2835_SPI_CS_INTD		0x00000200
#define BCM2835_SPI_CS_DMAEN		0x00000100
#define BCM2835_SPI_CS_TA		0x00000080
#define BCM2835_SPI_CS_CLEAR_RX		0x00000020
#define BCM2835_SPI_CS_CLEAR_TX		0x00000010
#define BCM2835_SPI_CS_CPOL		0x00000008
#define BCM2835_SPI_CS_CPHA		0x00000004
#define BCM2835_SPI_CS_CS_10		0x00000002
#define BCM2835_SPI_CS_CS_01		0x00000001

#define BCM2835_SPI_FIFO_SIZE		64
#define BCM2835_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

#define DRV_NAME	"spi-bcm2835"

/**
 * struct bcm2835_spi - BCM2835 SPI controller
 * @regs: base address of register map
 * @clk: core clock, divided to calculate serial clock
 * @clk_hz: core clock cached speed
 * @tfr: SPI transfer currently processed
 * @ctlr: SPI controller reverse lookup
 * @tx_buf: pointer whence next transmitted byte is read
 * @rx_buf: pointer where next received byte is written
 * @tx_len: remaining bytes to transmit
 * @rx_len: remaining bytes to receive
 */
struct bcm2835_spi {
	void __iomem *regs;
	struct clk *clk;
	unsigned long clk_hz;
	struct spi_transfer *tfr;
	struct spi_controller ctlr;
	const u8 *tx_buf;
	u8 *rx_buf;
	int tx_len;
	int rx_len;
};

/**
 * struct bcm2835_spidev - BCM2835 SPI target
 * @prepare_cs: precalculated CS register value for ->prepare_message()
 *	(uses target-specific clock polarity and phase settings)
 */
struct bcm2835_spidev {
	u32 prepare_cs;
};

static inline u32 bcm2835_rd(struct bcm2835_spi *bs, unsigned int reg)
{
	return readl(bs->regs + reg);
}

static inline void bcm2835_wr(struct bcm2835_spi *bs, unsigned int reg, u32 val)
{
	writel(val, bs->regs + reg);
}

static inline void bcm2835_rd_fifo(struct bcm2835_spi *bs)
{
	u8 byte;

	while ((bs->rx_len) &&
	       (bcm2835_rd(bs, BCM2835_SPI_CS) & BCM2835_SPI_CS_RXD)) {
		byte = bcm2835_rd(bs, BCM2835_SPI_FIFO);
		if (bs->rx_buf)
			*bs->rx_buf++ = byte;
		bs->rx_len--;
	}
}

static inline void bcm2835_wr_fifo(struct bcm2835_spi *bs)
{
	u8 byte;

	while ((bs->tx_len) &&
	       (bcm2835_rd(bs, BCM2835_SPI_CS) & BCM2835_SPI_CS_TXD)) {
		byte = bs->tx_buf ? *bs->tx_buf++ : 0;
		bcm2835_wr(bs, BCM2835_SPI_FIFO, byte);
		bs->tx_len--;
	}
}

/**
 * bcm2835_rd_fifo_count() - blindly read exactly @count bytes from RX FIFO
 * @bs: BCM2835 SPI controller
 * @count: bytes to read from RX FIFO
 *
 * The caller must ensure that @bs->rx_len is greater than or equal to @count,
 * that the RX FIFO contains at least @count bytes and that the DMA Enable flag
 * in the CS register is set (such that a read from the FIFO register receives
 * 32-bit instead of just 8-bit).  Moreover @bs->rx_buf must not be %NULL.
 */
static inline void bcm2835_rd_fifo_count(struct bcm2835_spi *bs, int count)
{
	u32 val;
	int len;

	bs->rx_len -= count;

	do {
		val = bcm2835_rd(bs, BCM2835_SPI_FIFO);
		len = min(count, 4);
		memcpy(bs->rx_buf, &val, len);
		bs->rx_buf += len;
		count -= 4;
	} while (count > 0);
}

/**
 * bcm2835_wr_fifo_blind() - blindly write up to @count bytes to TX FIFO
 * @bs: BCM2835 SPI controller
 * @count: bytes available for writing in TX FIFO
 */
static inline void bcm2835_wr_fifo_blind(struct bcm2835_spi *bs, int count)
{
	u8 val;

	count = min(count, bs->tx_len);
	bs->tx_len -= count;

	do {
		val = bs->tx_buf ? *bs->tx_buf++ : 0;
		bcm2835_wr(bs, BCM2835_SPI_FIFO, val);
	} while (--count);
}

static void bcm2835_spi_reset_hw(struct bcm2835_spi *bs)
{
	u32 cs = bcm2835_rd(bs, BCM2835_SPI_CS);

	/* Disable SPI interrupts and transfer */
	cs &= ~(BCM2835_SPI_CS_INTR |
		BCM2835_SPI_CS_INTD |
		BCM2835_SPI_CS_DMAEN |
		BCM2835_SPI_CS_TA);
	/*
	 * Transmission sometimes breaks unless the DONE bit is written at the
	 * end of every transfer.  The spec says it's a RO bit.  Either the
	 * spec is wrong and the bit is actually of type RW1C, or it's a
	 * hardware erratum.
	 */
	cs |= BCM2835_SPI_CS_DONE;
	/* and reset RX/TX FIFOS */
	cs |= BCM2835_SPI_CS_CLEAR_RX | BCM2835_SPI_CS_CLEAR_TX;

	/* and reset the SPI_HW */
	bcm2835_wr(bs, BCM2835_SPI_CS, cs);
	/* as well as DLEN */
	bcm2835_wr(bs, BCM2835_SPI_DLEN, 0);
}

static int bcm2835_spi_transfer_one_poll(struct spi_controller *ctlr,
					 struct spi_device *spi,
					 struct spi_transfer *tfr,
					 u32 cs)
{
	struct bcm2835_spi *bs = spi_controller_get_devdata(ctlr);

	/* enable HW block without interrupts */
	bcm2835_wr(bs, BCM2835_SPI_CS, cs | BCM2835_SPI_CS_TA);

	bcm2835_wr_fifo_blind(bs, BCM2835_SPI_FIFO_SIZE);

	/* loop until finished the transfer */
	while (bs->rx_len) {
		/* fill in tx fifo with remaining data */
		bcm2835_wr_fifo(bs);

		/* read from fifo as much as possible */
		bcm2835_rd_fifo(bs);
	}

	/* Transfer complete - reset SPI HW */
	bcm2835_spi_reset_hw(bs);
	/* and return without waiting for completion */
	return 0;
}

static int bcm2835_spi_transfer_one(struct spi_controller *ctlr,
				    struct spi_device *spi,
				    struct spi_transfer *tfr)
{
	struct bcm2835_spi *bs = spi_controller_get_devdata(ctlr);
	struct bcm2835_spidev *target = spi_get_ctldata(spi);
	unsigned long spi_hz, cdiv;
	u32 cs = target->prepare_cs;

	/* set clock */
	spi_hz = tfr->speed_hz;

	if (spi_hz >= bs->clk_hz / 2) {
		cdiv = 2; /* clk_hz/2 is the fastest we can go */
	} else if (spi_hz) {
		/* CDIV must be a multiple of two */
		cdiv = DIV_ROUND_UP(bs->clk_hz, spi_hz);
		cdiv += (cdiv % 2);

		if (cdiv >= 65536)
			cdiv = 0; /* 0 is the slowest we can go */
	} else {
		cdiv = 0; /* 0 is the slowest we can go */
	}
	tfr->effective_speed_hz = cdiv ? (bs->clk_hz / cdiv) : (bs->clk_hz / 65536);
	bcm2835_wr(bs, BCM2835_SPI_CLK, cdiv);

	/* handle all the 3-wire mode */
	if (spi->mode & SPI_3WIRE && tfr->rx_buf)
		cs |= BCM2835_SPI_CS_REN;

	/* set transmit buffers and length */
	bs->tx_buf = tfr->tx_buf;
	bs->rx_buf = tfr->rx_buf;
	bs->tx_len = tfr->len;
	bs->rx_len = tfr->len;

	return bcm2835_spi_transfer_one_poll(ctlr, spi, tfr, cs);
}

static int bcm2835_spi_prepare_message(struct spi_controller *ctlr,
				       struct spi_message *msg)
{
	struct spi_device *spi = msg->spi;
	struct bcm2835_spi *bs = spi_controller_get_devdata(ctlr);
	struct bcm2835_spidev *target = spi_get_ctldata(spi);

	/*
	 * Set up clock polarity before spi_transfer_one_message() asserts
	 * chip select to avoid a gratuitous clock signal edge.
	 */
	bcm2835_wr(bs, BCM2835_SPI_CS, target->prepare_cs);

	return 0;
}

static void bcm2835_spi_handle_err(struct spi_controller *ctlr,
				   struct spi_message *msg)
{
	struct bcm2835_spi *bs = spi_controller_get_devdata(ctlr);

	/* and reset */
	bcm2835_spi_reset_hw(bs);
}


static void bcm2835_spi_cleanup(struct spi_device *spi)
{
	struct bcm2835_spidev *target = spi_get_ctldata(spi);

	kfree(target);
}

static size_t bcm2835_spi_max_transfer_size(struct spi_device *spi)
{
	return SIZE_MAX;
}

static int bcm2835_spi_setup(struct spi_device *spi)
{
	struct bcm2835_spidev *target = spi_get_ctldata(spi);
	u32 cs;

	if (!target) {
		target = kzalloc(sizeof(*target), GFP_KERNEL);
		if (!target)
			return -ENOMEM;

		spi_set_ctldata(spi, target);
	}

	/*
	 * Precalculate SPI target's CS register value for ->prepare_message():
	 * The driver always uses software-controlled GPIO chip select, hence
	 * set the hardware-controlled native chip select to an invalid value
	 * to prevent it from interfering.
	 */
	cs = BCM2835_SPI_CS_CS_10 | BCM2835_SPI_CS_CS_01;
	if (spi->mode & SPI_CPOL)
		cs |= BCM2835_SPI_CS_CPOL;
	if (spi->mode & SPI_CPHA)
		cs |= BCM2835_SPI_CS_CPHA;
	target->prepare_cs = cs;

	/*
	 * sanity checking the native-chipselects
	 */
	if (spi->mode & SPI_NO_CS)
		return 0;

	if (!spi->cs_gpiod) {
		dev_err(&spi->dev, "Driver supports only cs-gpios at the moment\n");
		return -EINVAL;
	}

	return 0;
}

static int bcm2835_spi_probe(struct device *dev)
{
	struct spi_controller *ctlr;
	struct resource *iores;
	struct bcm2835_spi *bs;
	int err;

	bs = xzalloc(sizeof(*bs));

	ctlr = &bs->ctlr;

	/* ctlr->mode_bits = BCM2835_SPI_MODE_BITS; */
	ctlr->bits_per_word_mask = SPI_BPW_MASK(8);
	ctlr->num_chipselect = 3;
	ctlr->use_gpio_descriptors = true;
	ctlr->max_transfer_size = bcm2835_spi_max_transfer_size;
	ctlr->setup = bcm2835_spi_setup;
	ctlr->cleanup = bcm2835_spi_cleanup;
	ctlr->transfer_one = bcm2835_spi_transfer_one;
	ctlr->handle_err = bcm2835_spi_handle_err;
	ctlr->prepare_message = bcm2835_spi_prepare_message;
	ctlr->dev = dev;

	spi_controller_set_devdata(ctlr, bs);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return dev_err_probe(dev, PTR_ERR(iores),
				     "failed to get io-resource\n");
	bs->regs = IOMEM(iores->start);

	bs->clk = clk_get_enabled(dev, NULL);
	if (IS_ERR(bs->clk))
		return dev_err_probe(dev, PTR_ERR(bs->clk),
				     "could not get clk\n");

	ctlr->max_speed_hz = clk_get_rate(bs->clk) / 2;

	bs->clk_hz = clk_get_rate(bs->clk);

	/* initialise the hardware with the default polarities */
	bcm2835_wr(bs, BCM2835_SPI_CS,
		   BCM2835_SPI_CS_CLEAR_RX | BCM2835_SPI_CS_CLEAR_TX);

	err = spi_register_controller(ctlr);
	if (err)
		return dev_err_probe(dev, err,
				     "could not register SPI controller\n");

	return 0;
}

static const struct of_device_id bcm2835_spi_match[] = {
	{ .compatible = "brcm,bcm2835-spi", },
	{}
};
MODULE_DEVICE_TABLE(of, bcm2835_spi_match);

static struct driver bcm2835_spi_driver = {
	.name		= DRV_NAME,
	.of_compatible	= DRV_OF_COMPAT(bcm2835_spi_match),
	.probe		= bcm2835_spi_probe,
};
coredevice_platform_driver(bcm2835_spi_driver);

MODULE_DESCRIPTION("SPI controller driver for Broadcom BCM2835");
MODULE_AUTHOR("Chris Boot <bootc@bootc.net>");
MODULE_LICENSE("GPL");

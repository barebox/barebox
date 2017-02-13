/*
 * Copyright (c) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * Based on drivers/spi/spi-fsl-dspi.c from Linux kernel
 *
 * Copyright 2013 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <linux/math64.h>
#include <xfuncs.h>
#include <io.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <gpio.h>
#include <of_gpio.h>
#include <of_device.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <clock.h>

#define SPI_MCR		0x00
#define SPI_MCR_MASTER		(1 << 31)
#define SPI_MCR_PCSIS		(0x3F << 16)
#define SPI_MCR_CLR_TXF	(1 << 11)
#define SPI_MCR_CLR_RXF	(1 << 10)
#define SPI_MCR_DIS_TXF		BIT(13)
#define SPI_MCR_DIS_RXF		BIT(12)

#define SPI_CTAR(x)		(0x0c + (((x) & 0x3) * 4))
#define SPI_CTAR_FMSZ(x)	(((x) & 0x0000000f) << 27)
#define SPI_CTAR_CPOL(x)	((x) << 26)
#define SPI_CTAR_CPHA(x)	((x) << 25)
#define SPI_CTAR_LSBFE(x)	((x) << 24)
#define SPI_CTAR_PCSSCK(x)	(((x) & 0x00000003) << 22)
#define SPI_CTAR_PASC(x)	(((x) & 0x00000003) << 20)
#define SPI_CTAR_PDT(x)	(((x) & 0x00000003) << 18)
#define SPI_CTAR_PBR(x)	(((x) & 0x00000003) << 16)
#define SPI_CTAR_CSSCK(x)	(((x) & 0x0000000f) << 12)
#define SPI_CTAR_ASC(x)	(((x) & 0x0000000f) << 8)
#define SPI_CTAR_DT(x)		(((x) & 0x0000000f) << 4)
#define SPI_CTAR_BR(x)		((x) & 0x0000000f)
#define SPI_CTAR_SCALE_BITS	0xf

#define SPI_SR			0x2c
#define SPI_SR_EOQF		0x10000000
#define SPI_SR_TCFQF		0x80000000
#define SPI_SR_RFDF		BIT(17)

#define SPI_PUSHR		0x34
#define SPI_PUSHR_CONT		(1 << 31)
#define SPI_PUSHR_CTAS(x)	(((x) & 0x00000003) << 28)
#define SPI_PUSHR_EOQ		(1 << 27)
#define SPI_PUSHR_CTCNT	(1 << 26)
#define SPI_PUSHR_PCS(x)	(((1 << x) & 0x0000003f) << 16)
#define SPI_PUSHR_TXDATA(x)	((x) & 0x0000ffff)

#define SPI_POPR		0x38
#define SPI_POPR_RXDATA(x)	((x) & 0x0000ffff)


enum dspi_trans_mode {
	DSPI_EOQ_MODE = 0,
	DSPI_TCFQ_MODE,
};

struct fsl_dspi {
	struct spi_master	master;
	int			*cs_array;
	void __iomem		*base;
	struct clk		*clk;
	u32 			cs_sck_delay;
	u32			sck_cs_delay;

	const struct fsl_dspi_devtype_data *devtype_data;
};

struct fsl_dspi_devtype_data {
	enum dspi_trans_mode trans_mode;
	u8 max_clock_factor;
};

static struct fsl_dspi *to_dspi(struct spi_master *master)
{
	return container_of(master, struct fsl_dspi, master);
}

static void hz_to_spi_baud(struct device_d *dev,
			   char *pbr, char *br, int speed_hz,
			   unsigned long clkrate)
{
	/* Valid baud rate pre-scaler values */
	int pbr_tbl[4] = {2, 3, 5, 7};
	int brs[16] = {	2,	4,	6,	8,
		16,	32,	64,	128,
		256,	512,	1024,	2048,
		4096,	8192,	16384,	32768 };
	int scale_needed, scale, minscale = INT_MAX;
	int i, j;

	scale_needed = clkrate / speed_hz;
	if (clkrate % speed_hz)
		scale_needed++;

	for (i = 0; i < ARRAY_SIZE(brs); i++)
		for (j = 0; j < ARRAY_SIZE(pbr_tbl); j++) {
			scale = brs[i] * pbr_tbl[j];
			if (scale >= scale_needed) {
				if (scale < minscale) {
					minscale = scale;
					*br = i;
					*pbr = j;
				}
				break;
			}
		}

	if (minscale == INT_MAX) {
		dev_warn(dev, "Can not find valid baud rate,speed_hz is %d,"
			 "clkrate is %ld, we use the max prescaler value.\n",
			speed_hz, clkrate);
		*pbr = ARRAY_SIZE(pbr_tbl) - 1;
		*br =  ARRAY_SIZE(brs) - 1;
	}
}

static void ns_delay_scale(struct device_d *dev,
			   char *psc, char *sc, int delay_ns,
			   unsigned long clkrate)
{
	int pscale_tbl[4] = {1, 3, 5, 7};
	int scale_needed, scale, minscale = INT_MAX;
	int i, j;
	u32 remainder;

	scale_needed = div_u64_rem((u64)delay_ns * clkrate, NSEC_PER_SEC,
			&remainder);
	if (remainder)
		scale_needed++;

	for (i = 0; i < ARRAY_SIZE(pscale_tbl); i++)
		for (j = 0; j <= SPI_CTAR_SCALE_BITS; j++) {
			scale = pscale_tbl[i] * (2 << j);
			if (scale >= scale_needed) {
				if (scale < minscale) {
					minscale = scale;
					*psc = i;
					*sc = j;
				}
				break;
			}
		}

	if (minscale == INT_MAX) {
		dev_warn(dev, "Cannot find correct scale values for %dns "
			 "delay at clkrate %ld, using max prescaler value",
			 delay_ns, clkrate);
		*psc = ARRAY_SIZE(pscale_tbl) - 1;
		*sc = SPI_CTAR_SCALE_BITS;
	}
}

static u32 dspi_xchg_single(struct fsl_dspi *dspi, u32 in)
{
	const uint32_t sr_ready = (SPI_SR_EOQF | SPI_SR_RFDF);

	writel(in, dspi->base + SPI_PUSHR);

	while ((readl(dspi->base + SPI_SR) & sr_ready) != sr_ready)
		;

	writel(sr_ready, dspi->base + SPI_SR);

	return readl(dspi->base + SPI_POPR);
}

static const void *dspi_load_and_advance(const void *buffer,
					 size_t word_size, uint32_t *word)
{
	if (!buffer) {
		*word = 0;
		return NULL;
	}

	switch (word_size) {
	case 2:
		*word = *(const uint16_t *)buffer;
		break;
	case 1:
		*word = *(const uint8_t *)buffer;
		break;
	}

	return buffer + word_size;
}

static void *dspi_store_and_advance(void *buffer,
				    size_t word_size, uint32_t word)
{
	if (!buffer)
		return NULL;

	switch (word_size) {
	case 2:
		*(uint16_t *)buffer = word;
		break;
	case 1:
		*(uint8_t *)buffer  = word;
		break;
	}

	return buffer + word_size;
}


static void dspi_do_transfer(struct spi_device *spi,
			     struct spi_transfer *transfer,
			     bool last)
{
	size_t i;
	void *rx;
	const void *tx;
	unsigned int flags;
	struct fsl_dspi *dspi = to_dspi(spi->master);
	const size_t word_size = spi->bits_per_word <= 8 ? 1 : 2;
	const size_t count = transfer->len / word_size;

	flags = SPI_PUSHR_PCS(spi->chip_select)
		| SPI_PUSHR_CTAS(spi->chip_select)
		| SPI_PUSHR_EOQ
		| SPI_PUSHR_CONT;

	tx = transfer->tx_buf;
	rx = transfer->rx_buf;

	for (i = 0; i < count; i++) {
		uint32_t in, out;

		if (i == count - 1 &&
		    (last || transfer->cs_change))
			flags &= ~SPI_PUSHR_CONT;

		tx = dspi_load_and_advance(tx, word_size, &out);
		in = dspi_xchg_single(dspi, flags | out);
		rx = dspi_store_and_advance(rx, word_size, in);
	}
}

static int dspi_transfer(struct spi_device *spi,
			 struct spi_message *message)
{
	struct spi_transfer *transfer;

	message->actual_length = 0;

	list_for_each_entry(transfer, &message->transfers, transfer_list) {

		if (spi->bits_per_word > 8 &&
		    transfer->len % 2)
			dev_err(spi->master->dev,
				"Transfer doesn't contain exact number of SPI "
				"words. Last partial word will be truncated");

		dspi_do_transfer(spi, transfer,
				 list_is_last(&transfer->transfer_list,
					      &message->transfers));
		message->actual_length += transfer->len;
	}

	return 0;
}

static int dspi_setup(struct spi_device *spi)
{
	struct fsl_dspi *dspi = to_dspi(spi->master);
	unsigned char br = 0, pbr = 0, pcssck = 0, cssck = 0;
	unsigned char pasc = 0, asc = 0, fmsz = 0;
	unsigned long clkrate;

	if (4 <= spi->bits_per_word && spi->bits_per_word <= 16)
		fmsz = spi->bits_per_word - 1;
	else
		return -ENODEV;

	clkrate = clk_get_rate(dspi->clk);
	hz_to_spi_baud(spi->master->dev, &pbr, &br, spi->max_speed_hz, clkrate);

	/* Set PCS to SCK delay scale values */
	ns_delay_scale(spi->master->dev, &pcssck, &cssck, dspi->cs_sck_delay, clkrate);

	/* Set After SCK delay scale values */
	ns_delay_scale(spi->master->dev, &pasc, &asc, dspi->sck_cs_delay, clkrate);

	writel(SPI_CTAR_FMSZ(fmsz)
	       | SPI_CTAR_CPOL(spi->mode & SPI_CPOL ? 1 : 0)
	       | SPI_CTAR_CPHA(spi->mode & SPI_CPHA ? 1 : 0)
	       | SPI_CTAR_LSBFE(spi->mode & SPI_LSB_FIRST ? 1 : 0)
	       | SPI_CTAR_PCSSCK(pcssck)
	       | SPI_CTAR_CSSCK(cssck)
	       | SPI_CTAR_PASC(pasc)
	       | SPI_CTAR_ASC(asc)
	       | SPI_CTAR_PBR(pbr)
	       | SPI_CTAR_BR(br),
	       dspi->base + SPI_CTAR(spi->chip_select));

	return 0;
}

static int dspi_probe(struct device_d *dev)
{
	struct resource *io;
	struct fsl_dspi *dspi;
	struct spi_master *master;
	struct device_node *np = dev->device_node;

	int ret = 0;
	uint32_t bus_num = 0;
	uint32_t cs_num;

	dspi = xzalloc(sizeof(*dspi));

	dspi->devtype_data = of_device_get_match_data(dev);
	if (!dspi->devtype_data) {
		dev_err(dev, "can't get devtype_data\n");
		ret = -EFAULT;
		goto free_memory;
	}

	master = &dspi->master;
	master->dev = dev;
	master->setup = dspi_setup;
	master->transfer = dspi_transfer;

	ret = of_property_read_u32(np, "spi-num-chipselects", &cs_num);
	if (ret < 0) {
		dev_err(dev, "can't get spi-num-chipselects\n");
		goto free_memory;
	}
	master->num_chipselect = cs_num;

	if (!of_property_read_u32(np, "bus-num", &bus_num))
		master->bus_num = bus_num;
	else
		master->bus_num = dev->id;

	of_property_read_u32(dev->device_node, "fsl,spi-cs-sck-delay",
			     &dspi->cs_sck_delay);
	of_property_read_u32(dev->device_node, "fsl,spi-sck-cs-delay",
			     &dspi->sck_cs_delay);

	io = dev_request_mem_resource(dev, 0);
	if (IS_ERR(io)) {
		ret = PTR_ERR(io);
		goto free_memory;
	}
	dspi->base = IOMEM(io->start);

	dspi->clk = clk_get(dev, "dspi");
	if (IS_ERR(dspi->clk)) {
		ret = PTR_ERR(dspi->clk);
		goto free_memory;
	}

	ret = clk_enable(dspi->clk);
	if (ret)
		goto free_memory;

	writel(SPI_MCR_MASTER  |
	       SPI_MCR_PCSIS   |
	       SPI_MCR_CLR_TXF |
	       SPI_MCR_CLR_RXF |
	       SPI_MCR_DIS_TXF |
	       SPI_MCR_DIS_RXF, dspi->base + SPI_MCR);

	ret = spi_register_master(master);
	if (!ret)
		return 0;

	dev_err(dev, "Problem registering DSPI master\n");
	clk_disable(dspi->clk);
free_memory:
	free(dspi);
	return ret;
}

static const struct fsl_dspi_devtype_data vf610_data = {
	.trans_mode = DSPI_EOQ_MODE,
	.max_clock_factor = 2,
};

static const struct of_device_id dspi_dt_ids[] = {
	{ .compatible = "fsl,vf610-dspi", .data = (void *)&vf610_data, },
	{ /* sentinel */ }
};

static struct driver_d dspi_spi_driver = {
	.name  = "fsl-dspi",
	.probe = dspi_probe,
	.of_compatible = DRV_OF_COMPAT(dspi_dt_ids),
};
device_platform_driver(dspi_spi_driver);

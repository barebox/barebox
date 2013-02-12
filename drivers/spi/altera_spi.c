/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <io.h>
#include <asm/spi.h>
#include <asm/nios2-io.h>
#include <clock.h>

static void altera_spi_cs_inactive(struct spi_device *spi);

static int altera_spi_setup(struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	struct device_d spi_dev = spi->dev;
	struct altera_spi *altera_spi = container_of(master, struct altera_spi, master);

	if (spi->bits_per_word != altera_spi->databits) {
		dev_err(master->dev, " master doesn't support %d bits per word requested by %s\n",
			spi->bits_per_word, spi_dev.name);
		return -1;
	}

	if ((spi->mode & (SPI_CPHA | SPI_CPOL)) != altera_spi->mode) {
		dev_err(master->dev, " master doesn't support SPI_MODE%d requested by %s\n",
			spi->mode & (SPI_CPHA | SPI_CPOL), spi_dev.name);
		return -1;
	}

	if (spi->max_speed_hz < altera_spi->speed) {
		dev_err(master->dev, " frequency is too high for %s\n", spi_dev.name);
		return -1;
	}

	altera_spi_cs_inactive(spi);

	dev_dbg(master->dev, " mode 0x%08x, bits_per_word: %d, speed: %d\n",
		spi->mode, spi->bits_per_word, altera_spi->speed);

	return 0;
}


static unsigned int altera_spi_xchg_single(struct altera_spi *altera_spi, unsigned int data)
{
	struct nios_spi *nios_spi = altera_spi->regs;

	while (!(readl(&nios_spi->status) & NIOS_SPI_TRDY));
	writel(data, &nios_spi->txdata);

	while (!(readl(&nios_spi->status) & NIOS_SPI_RRDY));

	return readl(&nios_spi->rxdata);
}

/*
 * When using SPI_CS_HIGH devices, only one device is allowed to be
 * connected to the Altera SPI master. This limitation is due to the
 * emulation of an active high CS by writing 0 to the slaveselect register
 * (this produce a '1' to all CS pins).
 */

static void altera_spi_cs_active(struct spi_device *spi)
{
	struct altera_spi *altera_spi = container_of(spi->master, struct altera_spi, master);
	struct nios_spi *nios_spi = altera_spi->regs;
	uint32_t tmp;

	if (spi->mode & SPI_CS_HIGH) {
		tmp = readw(&nios_spi->control);
		writew(tmp & ~NIOS_SPI_SSO, &nios_spi->control);
		writel(0, &nios_spi->slaveselect);
	} else {
		writel(1 << spi->chip_select, &nios_spi->slaveselect);
		tmp = readl(&nios_spi->control);
		writel(tmp | NIOS_SPI_SSO, &nios_spi->control);
	}
}

static void altera_spi_cs_inactive(struct spi_device *spi)
{
	struct altera_spi *altera_spi = container_of(spi->master, struct altera_spi, master);
	struct nios_spi *nios_spi = altera_spi->regs;
	uint32_t tmp;

	if (spi->mode & SPI_CS_HIGH) {
		writel(1 << spi->chip_select, &nios_spi->slaveselect);
		tmp = readl(&nios_spi->control);
		writel(tmp | NIOS_SPI_SSO, &nios_spi->control);
	} else {
		tmp = readw(&nios_spi->control);
		writew(tmp & ~NIOS_SPI_SSO, &nios_spi->control);
	}
}

static unsigned altera_spi_do_xfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct altera_spi *altera_spi = container_of(spi->master, struct altera_spi, master);
	int word_len = spi->bits_per_word;
	unsigned retval = 0;
	u32 txval;
	u32 rxval;

	word_len = spi->bits_per_word;

	if (word_len <= 8) {
		const u8 *txbuf = t->tx_buf;
		u8 *rxbuf = t->rx_buf;
		int i = 0;

		while (i < t->len) {
			txval = txbuf ? txbuf[i] : 0;
			rxval = altera_spi_xchg_single(altera_spi, txval);
			if (rxbuf)
				rxbuf[i] = rxval;
			i++;
			retval++;
		}
	} else if (word_len <= 16) {
		const u16 *txbuf = t->tx_buf;
		u16 *rxbuf = t->rx_buf;
		int i = 0;

		while (i < t->len >> 1) {
			txval = txbuf ? txbuf[i] : 0;
			rxval = altera_spi_xchg_single(altera_spi, txval);
			if (rxbuf)
				rxbuf[i] = rxval;
			i++;
			retval += 2;
		}
	} else if (word_len <= 32) {
		const u32 *txbuf = t->tx_buf;
		u32 *rxbuf = t->rx_buf;
		int i = 0;

		while (i < t->len >> 2) {
			txval = txbuf ? txbuf[i] : 0;
			rxval = altera_spi_xchg_single(altera_spi, txval);
			if (rxbuf)
				rxbuf[i] = rxval;
			i++;
			retval += 4;
		}
	}

	return retval;
}

static int altera_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct altera_spi *altera_spi = container_of(spi->master, struct altera_spi, master);
	struct nios_spi *nios_spi = altera_spi->regs;
	struct spi_transfer *t;
	unsigned int cs_change;
	const int nsecs = 50;

	altera_spi_cs_active(spi);

	cs_change = 0;

	mesg->actual_length = 0;

	list_for_each_entry(t, &mesg->transfers, transfer_list) {

		if (cs_change) {
			ndelay(nsecs);
			altera_spi_cs_inactive(spi);
			ndelay(nsecs);
			altera_spi_cs_active(spi);
		}

		cs_change = t->cs_change;

		mesg->actual_length += altera_spi_do_xfer(spi, t);

		if (cs_change) {
			altera_spi_cs_active(spi);
		}
	}

	/* Wait the end of any pending transfer */
	while ((readl(&nios_spi->status) & NIOS_SPI_TMT) == 0);

	if (!cs_change)
		altera_spi_cs_inactive(spi);

	return 0;
}

static int altera_spi_probe(struct device_d *dev)
{
	struct spi_master *master;
	struct altera_spi *altera_spi;
	struct spi_altera_master *pdata = dev->platform_data;
	struct nios_spi *nios_spi;

	altera_spi = xzalloc(sizeof(*altera_spi));

	master = &altera_spi->master;
	master->dev = dev;

	master->setup = altera_spi_setup;
	master->transfer = altera_spi_transfer;
	master->num_chipselect = pdata->num_chipselect;
	master->bus_num = pdata->bus_num;

	altera_spi->regs = dev_request_mem_region(dev, 0);
	altera_spi->databits = pdata->databits;
	altera_spi->speed = pdata->speed;
	altera_spi->mode = pdata->spi_mode;

	nios_spi = altera_spi->regs;
	writel(0, &nios_spi->slaveselect);
	writel(0, &nios_spi->control);

	spi_register_master(master);

	return 0;
}

static struct driver_d altera_spi_driver = {
	.name  = "altera_spi",
	.probe = altera_spi_probe,
};
device_platform_driver(altera_spi_driver);

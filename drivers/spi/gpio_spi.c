// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SPI master driver using generic bitbanged GPIO
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux driver
 *   Copyright (C) 2006,2008 David Brownell
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <gpio.h>
#include <linux/gpio/consumer.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of_gpio.h>
#include <spi/spi.h>

struct gpio_spi {
	struct spi_master master;
	struct gpio_desc *sck;
	struct gpio_desc *mosi;
	struct gpio_desc *miso;
	struct gpio_descs *cs;
};

#define priv_from_spi_device(s)	container_of(s->master, struct gpio_spi, master)

static inline void setsck(const struct spi_device *spi, int is_on)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);

	gpiod_set_value(priv->sck, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);

	if (!priv->mosi)
		return;

	gpiod_set_value(priv->mosi, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);

	if (!priv->miso)
		return 1;

	return !!gpiod_get_value(priv->miso);
}

static inline void spidelay(unsigned int nsecs)
{
	ndelay(nsecs);
}

#include "spi-bitbang-txrx.h"

static int gpio_spi_set_cs(struct spi_device *spi, bool en)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);

	/*
	 * Use the raw variant here. Devices using active high chip select
	 * either have the spi-cs-high property in the device tree or set
	 * SPI_CS_HIGH in their driver.
	 */
	gpiod_set_raw_value(priv->cs->desc[spi->chip_select],
			    (spi->mode & SPI_CS_HIGH) ? en : !en);

	return 0;
}

static inline u32 gpio_spi_txrx_word(struct spi_device *spi, unsigned nsecs,
				     u32 word, u8 bits)
{
	int cpol = !!(spi->mode & SPI_CPOL);
	if (spi->mode & SPI_CPHA)
		return bitbang_txrx_be_cpha1(spi, nsecs, cpol, word, bits);
	else
		return bitbang_txrx_be_cpha0(spi, nsecs, cpol, word, bits);
}

static int gpio_spi_transfer_one_u8(struct spi_device *spi, struct spi_transfer *t)
{
	bool read = (t->rx_buf) ? true : false;
	u32 word = 0;
	int n;

	for (n = 0; n < t->len; n++) {
		if (!read)
			word = ((const u8 *)t->tx_buf)[n];
		word = gpio_spi_txrx_word(spi, 0, word, spi->bits_per_word);
		if (read)
			((u8 *)t->rx_buf)[n] = word;
	}

	return 0;
}

static int gpio_spi_transfer_one_u16(struct spi_device *spi, struct spi_transfer *t)
{
	bool read = (t->rx_buf) ? true : false;
	u32 word = 0;
	int n;

	for (n = 0; n < t->len / 2; n++) {
		if (!read)
			word = ((const u16 *)t->tx_buf)[n];
		word = gpio_spi_txrx_word(spi, 0, word, spi->bits_per_word);
		if (read)
			((u16 *)t->rx_buf)[n] = word;
	}

	return 0;
}

static int gpio_spi_transfer_one_u32(struct spi_device *spi, struct spi_transfer *t)
{
	bool read = (t->rx_buf) ? true : false;
	u32 word = 0;
	int n;

	for (n = 0; n < t->len / 4; n++) {
		if (!read)
			word = ((const u32 *)t->tx_buf)[n];
		word = gpio_spi_txrx_word(spi, 0, word, spi->bits_per_word);
		if (read)
			((u32 *)t->rx_buf)[n] = word;
	}

	return 0;
}

static int gpio_spi_transfer_one(struct spi_device *spi, struct spi_transfer *t)
{
	if (spi->bits_per_word <= 8)
		return gpio_spi_transfer_one_u8(spi, t);
	if (spi->bits_per_word <= 16)
		return gpio_spi_transfer_one_u16(spi, t);
	if (spi->bits_per_word <= 32)
		return gpio_spi_transfer_one_u32(spi, t);
	return -EINVAL;
}

static int gpio_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_transfer *t;
	int ret;

	ret = gpio_spi_set_cs(spi, true);
	if (ret)
		return ret;

	list_for_each_entry(t, &msg->transfers, transfer_list) {
		ret = gpio_spi_transfer_one(spi, t);
		if (ret)
			return ret;
		msg->actual_length += t->len;
	}

	ret = gpio_spi_set_cs(spi, false);
	if (ret)
		return ret;

	return 0;
}

static int gpio_spi_setup(struct spi_device *spi)
{
	gpio_spi_set_cs(spi, 0);
	return 0;
}

static int gpio_spi_probe(struct device *dev)
{
	struct gpio_spi *priv;
	struct spi_master *master;

	priv = xzalloc(sizeof(*priv));

	priv->sck = gpiod_get(dev, "sck", GPIOD_OUT_LOW);
	if (IS_ERR(priv->sck))
		return dev_err_probe(dev, PTR_ERR(priv->sck), "No SCK GPIO\n");

	priv->miso = gpiod_get_optional(dev, "miso", GPIOD_IN);
	if (IS_ERR(priv->miso))
		return dev_err_probe(dev, PTR_ERR(priv->miso), "No MISO GPIO\n");

	priv->mosi = gpiod_get_optional(dev, "mosi", GPIOD_OUT_LOW);
	if (IS_ERR(priv->mosi))
		return dev_err_probe(dev, PTR_ERR(priv->mosi), "No MOSI GPIO\n");

	priv->cs = gpiod_get_array(dev, "cs", GPIOD_OUT_HIGH);
	if (IS_ERR(priv->cs))
		return dev_err_probe(dev, PTR_ERR(priv->cs), "No CS GPIOs\n");

	master = &priv->master;
	master->dev = dev;
	master->bus_num = dev->id;
	master->setup = gpio_spi_setup;
	master->transfer = gpio_spi_transfer;
	master->num_chipselect = priv->cs->ndescs;

	return spi_register_master(&priv->master);
}

static struct of_device_id __maybe_unused gpio_spi_dt_ids[] = {
	{ .compatible = "spi-gpio", },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_spi_dt_ids);

static struct driver gpio_spi_driver = {
	.name = "gpio-spi",
	.probe = gpio_spi_probe,
	.of_compatible = DRV_OF_COMPAT(gpio_spi_dt_ids),
};
device_platform_driver(gpio_spi_driver);

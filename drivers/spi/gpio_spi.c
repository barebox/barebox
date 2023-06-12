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
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of_gpio.h>
#include <spi/spi.h>
#include <spi/spi_gpio.h>

struct gpio_spi {
	struct spi_master master;
	struct gpio_spi_pdata *data;
};

#define priv_from_spi_device(s)	container_of(s->master, struct gpio_spi, master)

static inline void setsck(const struct spi_device *spi, int is_on)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);
	gpio_set_value(priv->data->sck, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);
	if (!gpio_is_valid(priv->data->mosi))
		return;
	gpio_set_value(priv->data->mosi, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);
	if (!gpio_is_valid(priv->data->miso))
		return 1;
	return !!gpio_get_value(priv->data->miso);
}

#define spidelay(nsecs) do { } while (0)

#include "spi-bitbang-txrx.h"

static int gpio_spi_set_cs(struct spi_device *spi, bool en)
{
	struct gpio_spi *priv = priv_from_spi_device(spi);

	if (!gpio_is_valid(priv->data->cs[spi->chip_select]))
		return 0;

	gpio_set_value(priv->data->cs[spi->chip_select],
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

static int gpio_spi_transfer_one(struct spi_device *spi, struct spi_transfer *t)
{
	bool read = (t->rx_buf) ? true : false;
	u32 word = 0;
	int n;

	for (n = 0; n < t->len; n++) {
		if (!read)
			word = ((const u8 *)t->tx_buf)[n];
		word = gpio_spi_txrx_word(spi, 0, word, 8);
		if (read)
			((u8 *)t->rx_buf)[n] = word & 0xff;
	}

	return 0;
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
	if (spi->bits_per_word != 8) {
		dev_err(spi->master->dev, "master does not support %d bits per word\n",
			spi->bits_per_word);
		return -EINVAL;
	}

	return 0;
}

static int gpio_spi_of_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct gpio_spi_pdata *pdata;
	int n, sck;

	if (!IS_ENABLED(CONFIG_OFDEVICE) || dev->platform_data)
		return 0;

	sck = of_get_named_gpio(np, "gpio-sck", 0);
	if (sck == -EPROBE_DEFER)
		return sck;
	if (!gpio_is_valid(sck)) {
		dev_err(dev, "missing mandatory SCK gpio\n");
		return sck;
	}

	pdata = xzalloc(sizeof(*pdata));
	pdata->sck = sck;
	pdata->num_cs = MAX_CHIPSELECT;

	pdata->miso = of_get_named_gpio(np, "gpio-miso", 0);
	if (!gpio_is_valid(pdata->miso))
		pdata->miso = SPI_GPIO_NO_MISO;

	pdata->mosi = of_get_named_gpio(np, "gpio-mosi", 0);
	if (!gpio_is_valid(pdata->mosi))
		pdata->mosi = SPI_GPIO_NO_MOSI;

	for (n = 0; n < MAX_CHIPSELECT; n++) {
		pdata->cs[n] = of_get_named_gpio(np, "cs-gpios", n);
		if (!gpio_is_valid(pdata->cs[n]))
		    pdata->cs[n] = SPI_GPIO_NO_CS;
	}

	dev->platform_data = pdata;

	return 0;
}

static int gpio_spi_probe(struct device *dev)
{
	struct gpio_spi *priv;
	struct gpio_spi_pdata *pdata;
	struct spi_master *master;
	int n, ret;

	ret = gpio_spi_of_probe(dev);
	if (ret)
		return ret;
	pdata = dev->platform_data;

	ret = gpio_request_one(pdata->sck, GPIOF_DIR_OUT, "spi-sck");
	if (ret)
		return ret;

	if (pdata->miso != SPI_GPIO_NO_MISO) {
		ret = gpio_request_one(pdata->miso, GPIOF_DIR_IN, "spi-miso");
		if (ret)
			return ret;
	}

	if (pdata->mosi != SPI_GPIO_NO_MOSI) {
		ret = gpio_request_one(pdata->mosi, GPIOF_DIR_OUT, "spi-mosi");
		if (ret)
			return ret;
	}

	for (n = 0; n < pdata->num_cs; n++) {
		char *cs_name;

		if (!gpio_is_valid(pdata->cs[n]))
			continue;

		cs_name = basprintf("spi-cs%d", n);
		ret = gpio_request_one(pdata->cs[n], GPIOF_DIR_OUT, cs_name);
		if (ret)
			return ret;
	}

	priv = xzalloc(sizeof(*priv));
	priv->data = pdata;
	master = &priv->master;
	master->dev = dev;
	master->bus_num = dev->id;
	master->setup = gpio_spi_setup;
	master->transfer = gpio_spi_transfer;
	master->num_chipselect = priv->data->num_cs;

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

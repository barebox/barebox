/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <firmware.h>
#include <of_gpio.h>
#include <xfuncs.h>
#include <malloc.h>
#include <gpio.h>
#include <clock.h>
#include <spi/spi.h>

#include <fcntl.h>
#include <fs.h>


/*
 * Physical requirements:
 * - three free GPIOs for the signals nCONFIG, CONFIGURE_DONE, nSTATUS
 * - 32 bit per word, LSB first capable SPI master (MOSI + clock)
 *
 * Example how to configure this driver via device tree
 *
 *	fpga@0 {
 *		compatible = "altr,fpga-passive-serial";
 *		nstat-gpios = <&gpio4 18 0>;
 *		confd-gpios = <&gpio4 19 0>;
 *		nconfig-gpios = <&gpio4 20 0>;
 *		spi-max-frequency = <10000000>;
 *		reg = <0>;
 *	};
 */

struct fpga_spi {
	struct firmware_handler fh;
	int nstat_gpio; /* input GPIO to read the status line */
	int confd_gpio; /* input GPIO to read the config done line */
	int nconfig_gpio; /* output GPIO to start the FPGA's config */
	struct device_d *dev;
	struct spi_device *spi;
	bool padding_done;
};

static int altera_spi_open(struct firmware_handler *fh)
{
	struct fpga_spi *this = container_of(fh, struct fpga_spi, fh);
	struct device_d *dev = this->dev;
	int ret;

	dev_dbg(dev, "Initiating programming\n");

	/* initiate an FPGA programming */
	gpio_set_value(this->nconfig_gpio, 0);

	/*
	 * after about 2 µs the FPGA must acknowledge with
	 * STATUS and CONFIG DONE lines at low level
	 */
	if (gpio_is_valid(this->nstat_gpio)) {
		ret = wait_on_timeout(2 * USECOND,
				(gpio_get_value(this->nstat_gpio) == 0) &&
				(gpio_get_value(this->confd_gpio) == 0));
	} else {
		ret = wait_on_timeout(2 * USECOND,
				(gpio_get_value(this->confd_gpio) == 0));
	}


	if (ret != 0) {
		dev_err(dev, "FPGA does not acknowledge the programming initiation\n");
		if (gpio_is_valid(this->nstat_gpio) && gpio_get_value(this->nstat_gpio))
			dev_err(dev, "STATUS is still high!\n");
		if (gpio_get_value(this->confd_gpio))
			dev_err(dev, "CONFIG DONE is still high!\n");
		return ret;
	}

	/* arm the FPGA to await its new firmware */
	gpio_set_value(this->nconfig_gpio, 1);

	/* once again, we might need padding the data */
	this->padding_done = false;

	/*
	 * after about 1506 µs the FPGA must acknowledge this step
	 * with the STATUS line at high level
	 */

	if (gpio_is_valid(this->nstat_gpio)) {
		ret = wait_on_timeout(1600 * USECOND,
				gpio_get_value(this->nstat_gpio) == 1);
		if (ret != 0) {
			dev_err(dev, "FPGA does not acknowledge the programming start\n");
			return ret;
		}
	} else {
		udelay(1600);
	}

	dev_dbg(dev, "Initiating passed\n");
	/* at the end, wait at least 2 µs prior beginning writing data */
	udelay(2);

	return 0;
}

static int altera_spi_write(struct firmware_handler *fh, const void *buf, size_t sz)
{
	struct fpga_spi *this = container_of(fh, struct fpga_spi, fh);
	struct device_d *dev = this->dev;
	struct spi_transfer t[2];
	struct spi_message m;
	u32 dummy;
	int ret;

	dev_dbg(dev, "Start writing %zu bytes.\n", sz);

	spi_message_init(&m);

	if (sz < sizeof(u32)) {
		/* simple padding */
		dummy = 0;
		memcpy(&dummy, buf, sz);
		buf = &dummy;
		sz = sizeof(u32);
		this->padding_done = true;
	}

	t[0].tx_buf = buf;
	t[0].rx_buf = NULL;
	t[0].len = sz;
	spi_message_add_tail(&t[0], &m);

	if (sz & 0x3) { /* padding required? */
		u32 *word_buf = (u32 *)buf;
		dummy = 0;
		memcpy(&dummy, &word_buf[sz >> 2], sz & 0x3);
		t[0].len &= ~0x03;
		t[1].tx_buf = &dummy;
		t[1].rx_buf = NULL;
		t[1].len = sizeof(u32);
		spi_message_add_tail(&t[1], &m);
		this->padding_done = true;
	}

	ret = spi_sync(this->spi, &m);
	if (ret != 0)
		dev_err(dev, "programming failure\n");

	return ret;
}

static int altera_spi_close(struct firmware_handler *fh)
{
	struct fpga_spi *this = container_of(fh, struct fpga_spi, fh);
	struct device_d *dev = this->dev;
	struct spi_transfer t;
	struct spi_message m;
	u32 dummy = 0;
	int ret;

	dev_dbg(dev, "Finalize programming\n");

	if (this->padding_done == false) {
		spi_message_init(&m);
		t.tx_buf = &dummy;
		t.rx_buf = NULL;
		t.len = sizeof(dummy);
		spi_message_add_tail(&t, &m);

		ret = spi_sync(this->spi, &m);
		if (ret != 0)
			dev_err(dev, "programming failure\n");
	}

	/*
	 * when programming was successful,
	 * both status lines should be at high level
	 */
	if (gpio_is_valid(this->nstat_gpio)) {
		ret = wait_on_timeout(10 * USECOND,
				(gpio_get_value(this->nstat_gpio) == 1) &&
				(gpio_get_value(this->confd_gpio) == 1));
	} else {
		ret = wait_on_timeout(10 * USECOND,
				(gpio_get_value(this->confd_gpio) == 1));

	}

	if (ret == 0) {
		dev_dbg(dev, "Programming successful\n");
		return ret;
	}

	dev_err(dev, "Programming failed due to time out\n");
	if (gpio_is_valid(this->nstat_gpio) &&
		gpio_get_value(this->nstat_gpio) == 0)
		dev_err(dev, "STATUS is still low!\n");
	if (gpio_get_value(this->confd_gpio) == 0)
		dev_err(dev, "CONFIG DONE is still low!\n");

	return -EIO;
}

static int altera_spi_of(struct device_d *dev, struct fpga_spi *this)
{
	struct device_node *n = dev->device_node;
	const char *name;
	int ret;

	name = "nstat-gpios";
	this->nstat_gpio = of_get_named_gpio(n, name, 0);
	if (this->nstat_gpio == -ENOENT) {
		dev_info(dev, "nstat-gpio is not specified, assuming it is not connected\n");
		this->nstat_gpio = -1;
	} else if (this->nstat_gpio < 0) {
		ret = this->nstat_gpio;
		goto out;
	}

	name = "confd-gpios";
	this->confd_gpio = of_get_named_gpio(n, name, 0);
	if (this->confd_gpio < 0) {
		ret = this->confd_gpio;
		goto out;
	}

	name = "nconfig-gpios";
	this->nconfig_gpio = of_get_named_gpio(n, name, 0);
	if (this->nconfig_gpio < 0) {
		ret = this->nconfig_gpio;
		goto out;
	}

	/* init to passive and sane values */
	ret = gpio_direction_output(this->nconfig_gpio, 1);
	if (ret)
		return ret;

	if (gpio_is_valid(this->nstat_gpio)) {
		ret = gpio_direction_input(this->nstat_gpio);
		if (ret)
			return ret;
	}

	ret = gpio_direction_input(this->confd_gpio);
	if (ret)
		return ret;

	return 0;

out:
	dev_err(dev, "Cannot request \"%s\" gpio: %s\n", name, strerror(-ret));

	return ret;
}

static void altera_spi_init_mode(struct spi_device *spi)
{
	spi->bits_per_word = 32;
	/*
	 * CPHA = CPOL = 0
	 * the FPGA expects its firmware data with LSB first
	 */
	spi->mode = SPI_MODE_0 | SPI_LSB_FIRST;
}

static int altera_spi_probe(struct device_d *dev)
{
	int rc;
	struct fpga_spi *this;
	struct firmware_handler *fh;
	const char *alias = of_alias_get(dev->device_node);
	const char *model = NULL;

	dev_dbg(dev, "Probing FPGA firmware programmer\n");

	this = xzalloc(sizeof(*this));
	fh = &this->fh;

	rc = altera_spi_of(dev, this);
	if (rc != 0)
		goto out;

	if (alias)
		fh->id = xstrdup(alias);
	else
		fh->id = xstrdup("altera-fpga");

	fh->open = altera_spi_open;
	fh->write = altera_spi_write;
	fh->close = altera_spi_close;
	of_property_read_string(dev->device_node, "compatible", &model);
	if (model)
		fh->model = xstrdup(model);
	fh->dev = dev;

	this->spi = (struct spi_device *)dev->type_data;
	altera_spi_init_mode(this->spi);
	this->dev = dev;

	dev_dbg(dev, "Registering FPGA firmware programmer\n");
	rc = firmwaremgr_register(fh);
	if (rc != 0) {
		free(this);
		goto out;
	}

	return 0;
out:
	free(fh->id);
	free(this);

	return rc;
}

static struct of_device_id altera_spi_id_table[] = {
	{
		.compatible = "altr,fpga-passive-serial",
	},
};

static struct driver_d altera_spi_driver = {
	.name = "altera-fpga",
	.of_compatible = DRV_OF_COMPAT(altera_spi_id_table),
	.probe = altera_spi_probe,
};
device_spi_driver(altera_spi_driver);

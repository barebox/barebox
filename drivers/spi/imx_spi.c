/*
 * Copyright (C) 2008 Sascha Hauer, Pengutronix 
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <xfuncs.h>
#include <asm/io.h>
#include <gpio.h>
#include <mach/spi.h>

#define CSPI_0_0_RXDATA		0x00
#define CSPI_0_0_TXDATA		0x04
#define CSPI_0_0_CTRL		0x08
#define CSPI_0_0_INT		0x0C
#define CSPI_0_0_DMA		0x18
#define CSPI_0_0_STAT		0x0C
#define CSPI_0_0_PERIOD		0x14
#define CSPI_0_0_TEST		0x10
#define CSPI_0_0_RESET		0x1C

#define CSPI_0_0_CTRL_ENABLE		(1 << 10)
#define CSPI_0_0_CTRL_MASTER		(1 << 11)
#define CSPI_0_0_CTRL_XCH		(1 << 9)
#define CSPI_0_0_CTRL_LOWPOL		(1 << 5)
#define CSPI_0_0_CTRL_PHA		(1 << 6)
#define CSPI_0_0_CTRL_SSCTL		(1 << 7)
#define CSPI_0_0_CTRL_HIGHSSPOL 	(1 << 8)
#define CSPI_0_0_CTRL_CS(x)		(((x) & 0x3) << 19)
#define CSPI_0_0_CTRL_BITCOUNT(x)	(((x) & 0x1f) << 0)
#define CSPI_0_0_CTRL_DATARATE(x)	(((x) & 0x7) << 14)

#define CSPI_0_0_CTRL_MAXDATRATE	0x10
#define CSPI_0_0_CTRL_DATAMASK		0x1F
#define CSPI_0_0_CTRL_DATASHIFT 	14

#define CSPI_0_0_STAT_TE		(1 << 0)
#define CSPI_0_0_STAT_TH		(1 << 1)
#define CSPI_0_0_STAT_TF		(1 << 2)
#define CSPI_0_0_STAT_RR		(1 << 4)
#define CSPI_0_0_STAT_RH		(1 << 5)
#define CSPI_0_0_STAT_RF		(1 << 6)
#define CSPI_0_0_STAT_RO		(1 << 7)

#define CSPI_0_0_PERIOD_32KHZ		(1 << 15)

#define CSPI_0_0_TEST_LBC		(1 << 14)

struct imx_spi {
	struct spi_master master;
	int *chipselect;
};

static int imx_spi_setup(struct spi_device *spi)
{
	debug("%s mode 0x%08x bits_per_word: %d speed: %d\n",
			__FUNCTION__, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);
	return 0;
}

static unsigned int spi_xchg_single(ulong base, unsigned int data)
{

	unsigned int cfg_reg = readl(base + CSPI_0_0_CTRL);

	writel(data, base + CSPI_0_0_TXDATA);

	cfg_reg |= CSPI_0_0_CTRL_XCH;

	writel(cfg_reg, base + CSPI_0_0_CTRL);

	while (!(readl(base + CSPI_0_0_INT) & CSPI_0_0_STAT_RR));

	return readl(base + CSPI_0_0_RXDATA);
}

static void mxc_spi_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_master *master = spi->master;
	struct imx_spi *imx = container_of(master, struct imx_spi, master);
	ulong base = master->dev->map_base;
	unsigned int cs = 0;
	int gpio = imx->chipselect[spi->chip_select];
	u32 ctrl_reg;

	if (spi->mode & SPI_CS_HIGH)
		cs = 1;

	if (!is_active) {
		if (gpio >= 0)
			gpio_set_value(gpio, !cs);
		return;
	}

	ctrl_reg = CSPI_0_0_CTRL_BITCOUNT(spi->bits_per_word - 1)
		| CSPI_0_0_CTRL_DATARATE(7) /* FIXME: calculate data rate */
		| CSPI_0_0_CTRL_ENABLE
		| CSPI_0_0_CTRL_MASTER;

	if (gpio < 0) {
		ctrl_reg |= CSPI_0_0_CTRL_CS(gpio + 32);
	}

	if (spi->mode & SPI_CPHA)
		ctrl_reg |= CSPI_0_0_CTRL_PHA;
	if (spi->mode & SPI_CPOL)
		ctrl_reg |= CSPI_0_0_CTRL_LOWPOL;
	if (spi->mode & SPI_CS_HIGH)
		ctrl_reg |= CSPI_0_0_CTRL_HIGHSSPOL;

	writel(ctrl_reg, base + CSPI_0_0_CTRL);

	if (gpio >= 0)
		gpio_set_value(gpio, cs);
}

static int imx_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct spi_master *master = spi->master;
	ulong base = master->dev->map_base;
	struct spi_transfer	*t = NULL;

	mxc_spi_chipselect(spi, 1);

	list_for_each_entry (t, &mesg->transfers, transfer_list) {
		const u32 *txbuf = t->tx_buf;
		u32 *rxbuf = t->rx_buf;
		int i = 0;

		while(i < t->len >> 2) {
			rxbuf[i] = spi_xchg_single(base, txbuf[i]);
			i++;
		}
	}

	mxc_spi_chipselect(spi, 0);

	return 0;
}

static int imx_spi_probe(struct device_d *dev)
{
	struct spi_master *master;
	struct imx_spi *imx;
	struct spi_imx_master *pdata = dev->platform_data;

	imx = xzalloc(sizeof(*imx));

	master = &imx->master;
	master->dev = dev;

	master->setup = imx_spi_setup;
	master->transfer = imx_spi_transfer;
	master->num_chipselect = pdata->num_chipselect;
	imx->chipselect = pdata->chipselect;

	writel(CSPI_0_0_CTRL_ENABLE | CSPI_0_0_CTRL_MASTER,
		     dev->map_base + CSPI_0_0_CTRL);
	writel(CSPI_0_0_PERIOD_32KHZ,
		     dev->map_base + CSPI_0_0_PERIOD);
	while (readl(dev->map_base + CSPI_0_0_INT) & CSPI_0_0_STAT_RR)
		readl(dev->map_base + CSPI_0_0_RXDATA);
	writel(0, dev->map_base + CSPI_0_0_INT);

	spi_register_master(master);

	return 0;
}

static struct driver_d imx_spi_driver = {
	.name  = "imx_spi",
	.probe = imx_spi_probe,
};

static int imx_spi_init(void)
{
	register_driver(&imx_spi_driver);
	return 0;
}

device_initcall(imx_spi_init);


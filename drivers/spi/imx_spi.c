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

#define MXC_CSPIRXDATA		0x00
#define MXC_CSPITXDATA		0x04
#define MXC_CSPICTRL		0x08
#define MXC_CSPIINT		0x0C
#define MXC_CSPIDMA		0x18
#define MXC_CSPISTAT		0x0C
#define MXC_CSPIPERIOD		0x14
#define MXC_CSPITEST		0x10
#define MXC_CSPIRESET		0x1C

#define MXC_CSPICTRL_ENABLE	(1 << 10)
#define MXC_CSPICTRL_MASTER	(1 << 11)
#define MXC_CSPICTRL_XCH	(1 << 9)
#define MXC_CSPICTRL_LOWPOL	(1 << 5)
#define MXC_CSPICTRL_PHA	(1 << 6)
#define MXC_CSPICTRL_SSCTL	(1 << 7)
#define MXC_CSPICTRL_HIGHSSPOL 	(1 << 8)
#define MXC_CSPICTRL_CS(x)	(((x) & 0x3) << 19)
#define MXC_CSPICTRL_BITCOUNT(x)	(((x) & 0x1f) << 0)
#define MXC_CSPICTRL_DATARATE(x)	(((x) & 0x7) << 14)

#define MXC_CSPICTRL_MAXDATRATE	0x10
#define MXC_CSPICTRL_DATAMASK	0x1F
#define MXC_CSPICTRL_DATASHIFT 	14

#define MXC_CSPISTAT_TE		(1 << 0)
#define MXC_CSPISTAT_TH		(1 << 1)
#define MXC_CSPISTAT_TF		(1 << 2)
#define MXC_CSPISTAT_RR		(1 << 3)
#define MXC_CSPISTAT_RH         (1 << 4)
#define MXC_CSPISTAT_RF         (1 << 5)
#define MXC_CSPISTAT_RO         (1 << 6)
#define MXC_CSPISTAT_TC_0_7	(1 << 7)
#define MXC_CSPISTAT_TC_0_5	(1 << 8)
#define MXC_CSPISTAT_TC_0_4	(1 << 8)
#define MXC_CSPISTAT_TC_0_0	(1 << 3)
#define MXC_CSPISTAT_BO_0_7	0
#define MXC_CSPISTAT_BO_0_5	(1 << 7)
#define MXC_CSPISTAT_BO_0_4	(1 << 7)
#define MXC_CSPISTAT_BO_0_0	(1 << 8)

#define MXC_CSPIPERIOD_32KHZ	(1 << 15)

#define MXC_CSPITEST_LBC	(1 << 14)

static int imx_spi_setup(struct spi_device *spi)
{
	debug("%s mode 0x%08x bits_per_word: %d speed: %d\n",
			__FUNCTION__, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);
	return 0;
}

static unsigned int spi_xchg_single(ulong base, unsigned int data)
{

	unsigned int cfg_reg = readl(base + MXC_CSPICTRL);

	writel(data, base + MXC_CSPITXDATA);

	cfg_reg |= MXC_CSPICTRL_XCH;

	writel(cfg_reg, base + MXC_CSPICTRL);

	while (readl(base + MXC_CSPICTRL) & MXC_CSPICTRL_XCH);

	return readl(base + MXC_CSPIRXDATA);
}

static void mxc_spi_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_master *master = spi->master;
	ulong base = master->dev->map_base;
	u32 ctrl_reg;

	ctrl_reg = MXC_CSPICTRL_CS(spi->chip_select)
		| MXC_CSPICTRL_BITCOUNT(spi->bits_per_word - 1)
		| MXC_CSPICTRL_DATARATE(7) /* FIXME: calculate data rate */
		| MXC_CSPICTRL_ENABLE
		| MXC_CSPICTRL_MASTER;

	if (spi->mode & SPI_CPHA)
		ctrl_reg |= MXC_CSPICTRL_PHA;
	if (!(spi->mode & SPI_CPOL))
		ctrl_reg |= MXC_CSPICTRL_LOWPOL;
	if (spi->mode & SPI_CS_HIGH)
		ctrl_reg |= MXC_CSPICTRL_HIGHSSPOL;

	writel(ctrl_reg, base + MXC_CSPICTRL);
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
	return 0;
}

static int imx_spi_probe(struct device_d *dev)
{
	struct spi_master *master;

	debug("%s\n", __FUNCTION__);

	master = xzalloc(sizeof(struct spi_master));
	debug("master: %p %d\n", master, sizeof(struct spi_master));

	master->dev = dev;

	master->setup = imx_spi_setup;
	master->transfer = imx_spi_transfer;
	master->num_chipselect = 1; /* FIXME: Board dependent */

	writel(MXC_CSPICTRL_ENABLE | MXC_CSPICTRL_MASTER,
		     dev->map_base + MXC_CSPICTRL);
	writel(MXC_CSPIPERIOD_32KHZ,
		     dev->map_base + MXC_CSPIPERIOD);
	writel(0, dev->map_base + MXC_CSPIINT);

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


/*
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
 *
 * Copyright (C) 2010 Dirk Behme <dirk.behme@googlemail.com>
 *
 * Driver for McSPI controller on OMAP3. Based on davinci_spi.c
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Copyright (C) 2007 Atmel Corporation
 *
 * Parts taken from linux/drivers/spi/omap2_mcspi.c
 * Copyright (C) 2005, 2006 Nokia Corporation
 *
 * Modified by Ruslan Araslanov <ruslan.araslanov@vitecmm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <clock.h>
#include <errno.h>
#include <spi/spi.h>
#include <malloc.h>
#include <io.h>
#include "omap3_spi.h"

#define WORD_LEN	8
#define SPI_WAIT_TIMEOUT MSECOND

#define SPI_XFER_BEGIN  0x01                    /* Assert CS before transfer */
#define SPI_XFER_END    0x02                    /* Deassert CS after transfer */

struct omap_spi_drvdata {
	unsigned register_offset;
};

static void spi_reset(struct spi_master *master)
{
	struct omap3_spi_master *omap3_master = container_of(master, struct omap3_spi_master, master);
	void __iomem *regs = omap3_master->regs;
	unsigned int tmp;

	writel(OMAP3_MCSPI_SYSCONFIG_SOFTRESET, regs + OMAP3_MCSPI_SYSCONFIG);
	do {
		tmp = readl(regs + OMAP3_MCSPI_SYSSTATUS);
	} while (!(tmp & OMAP3_MCSPI_SYSSTATUS_RESETDONE));

	writel(OMAP3_MCSPI_SYSCONFIG_AUTOIDLE |
				 OMAP3_MCSPI_SYSCONFIG_ENAWAKEUP |
				 OMAP3_MCSPI_SYSCONFIG_SMARTIDLE,
				 regs + OMAP3_MCSPI_SYSCONFIG);

	writel(OMAP3_MCSPI_WAKEUPENABLE_WKEN, regs + OMAP3_MCSPI_WAKEUPENABLE);
}

static int spi_claim_bus(struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	struct omap3_spi_master *omap3_master = container_of(master, struct omap3_spi_master, master);
	void __iomem *regs = omap3_master->regs;
	unsigned int conf, div = 0;

	/* McSPI global module configuration */

	/*
	 * setup when switching from (reset default) slave mode
	 * to single-channel master mode
	 */
	conf = readl(regs + OMAP3_MCSPI_MODULCTRL);
	conf &= ~(OMAP3_MCSPI_MODULCTRL_STEST | OMAP3_MCSPI_MODULCTRL_MS);
	conf |= OMAP3_MCSPI_MODULCTRL_SINGLE;
	writel(conf, regs + OMAP3_MCSPI_MODULCTRL);

	/* McSPI individual channel configuration */

	/* Calculate clock divisor. Valid range: 0x0 - 0xC ( /1 - /4096 ) */
	if (spi->max_speed_hz) {
		while (div <= 0xC && (OMAP3_MCSPI_MAX_FREQ / (1 << div))
					 > spi->max_speed_hz)
			div++;
	} else {
		div = 0xC;
	}

	conf = readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	/* standard 4-wire master mode:	SCK, MOSI/out, MISO/in, nCS
	 * REVISIT: this controller could support SPI_3WIRE mode.
	 */
	if (omap3_master->swap_miso_mosi) {
		/* swapped */
		conf |= (OMAP3_MCSPI_CHCONF_IS | OMAP3_MCSPI_CHCONF_DPE1);
		conf &= ~OMAP3_MCSPI_CHCONF_DPE0;
	} else {
		 /* bootloader default */
		conf &= ~(OMAP3_MCSPI_CHCONF_IS | OMAP3_MCSPI_CHCONF_DPE1);
		conf |= OMAP3_MCSPI_CHCONF_DPE0;
	}

	/* wordlength */
	conf &= ~OMAP3_MCSPI_CHCONF_WL_MASK;
	conf |= (WORD_LEN - 1) << 7;

	/* set chipselect polarity; manage with FORCE */
	if (!(spi->mode & SPI_CS_HIGH))
		conf |= OMAP3_MCSPI_CHCONF_EPOL; /* active-low; normal */
	else
		conf &= ~OMAP3_MCSPI_CHCONF_EPOL;

	/* set clock divisor */
	conf &= ~OMAP3_MCSPI_CHCONF_CLKD_MASK;
	conf |= div << 2;

	/* set SPI mode 0..3 */
	if (spi->mode & SPI_CPOL)
		conf |= OMAP3_MCSPI_CHCONF_POL;
	else
		conf &= ~OMAP3_MCSPI_CHCONF_POL;
	if (spi->mode & SPI_CPHA)
		conf |= OMAP3_MCSPI_CHCONF_PHA;
	else
		conf &= ~OMAP3_MCSPI_CHCONF_PHA;

	/* Transmit & receive mode */
	conf &= ~OMAP3_MCSPI_CHCONF_TRM_MASK;

	writel(conf, regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	return 0;
}

static int omap3_spi_write(struct spi_device *spi, unsigned int len, const u8 *txp,
		    unsigned long flags)
{
	struct spi_master *master = spi->master;
	struct omap3_spi_master *omap3_master = container_of(master, struct omap3_spi_master, master);
	void __iomem *regs = omap3_master->regs;
	int i;
	uint64_t timer_start;
	int chconf = readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	if (flags & SPI_XFER_BEGIN)
		writel(OMAP3_MCSPI_CHCTRL_EN,
		       regs + OMAP3_MCSPI_CHCTRL0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	chconf &= ~OMAP3_MCSPI_CHCONF_TRM_MASK;
	chconf |= OMAP3_MCSPI_CHCONF_TRM_TX_ONLY;
	chconf |= OMAP3_MCSPI_CHCONF_FORCE;
	writel(chconf, regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	for (i = 0; i < len; i++) {
		/* wait till TX register is empty (TXS == 1) */
		timer_start = get_time_ns();
		while (!(readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE) &
			 OMAP3_MCSPI_CHSTAT_TXS)) {
			if (is_timeout(timer_start, SPI_WAIT_TIMEOUT)) {
				printf("SPI TXS timed out, status=0x%08x\n",
				       readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE));
				return -ETIMEDOUT;
			}
		}
		/* write the data */
		writel(txp[i], regs + OMAP3_MCSPI_TX0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	}

	if (flags & SPI_XFER_END) {
		/* wait to finish of transfer */
		while (!(readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE) &
			 OMAP3_MCSPI_CHSTAT_EOT));

		chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
		writel(chconf, regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

		writel(0, regs + OMAP3_MCSPI_CHCTRL0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	}

	while (!(readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE) &
			 OMAP3_MCSPI_CHSTAT_TXS));
	while (!(readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE) &
			 OMAP3_MCSPI_CHSTAT_EOT));

	return 0;
}

static int omap3_spi_read(struct spi_device *spi, unsigned int len, u8 *rxp,
		   unsigned long flags)
{
	struct spi_master *master = spi->master;
	struct omap3_spi_master *omap3_master = container_of(master, struct omap3_spi_master, master);
	void __iomem *regs = omap3_master->regs;
	int i;
	uint64_t timer_start;
	int chconf = readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	if (flags & SPI_XFER_BEGIN)
		writel(OMAP3_MCSPI_CHCTRL_EN,
		       regs + OMAP3_MCSPI_CHCTRL0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	chconf &= ~OMAP3_MCSPI_CHCONF_TRM_MASK;
	chconf |= OMAP3_MCSPI_CHCONF_TRM_RX_ONLY;
	chconf |= OMAP3_MCSPI_CHCONF_FORCE;
	writel(chconf, regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	writel(0, regs + OMAP3_MCSPI_TX0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

	for (i = 0; i < len; i++) {
		/* wait till RX register contains data (RXS == 1) */
		timer_start = get_time_ns();
		while (!(readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE) &
			 OMAP3_MCSPI_CHSTAT_RXS)) {
			if (is_timeout(timer_start, SPI_WAIT_TIMEOUT)) {
				printf("SPI RXS timed out, status=0x%08x\n",
				       readl(regs + OMAP3_MCSPI_CHSTAT0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE));
				return -ETIMEDOUT;
			}
		}
		/* read the data */
		rxp[i] = readl(regs + OMAP3_MCSPI_RX0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	}

	if (flags & SPI_XFER_END) {
		chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
		writel(chconf, regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
		readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

		writel(0, regs + OMAP3_MCSPI_CHCTRL0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
	}

	return 0;
}

static int spi_xfer(struct spi_device *spi, struct spi_transfer *t, unsigned long flags)
{
	struct spi_master *master = spi->master;
	struct omap3_spi_master *omap3_master = container_of(master, struct omap3_spi_master, master);
	void __iomem *regs = omap3_master->regs;
	unsigned int    len = t->len;
	int             ret;
	const u8        *txp = t->tx_buf; /* can be NULL for read operation */
	u8              *rxp = t->rx_buf; /* can be NULL for write operation */

	if (len == 0) {	 /* only change CS */
		int chconf = readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);

		if (flags & SPI_XFER_BEGIN) {
			writel(OMAP3_MCSPI_CHCTRL_EN,
			       regs + OMAP3_MCSPI_CHCTRL0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
			chconf |= OMAP3_MCSPI_CHCONF_FORCE;
			writel(chconf,
			       regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
			readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
		}

		if (flags & SPI_XFER_END) {
			chconf &= ~OMAP3_MCSPI_CHCONF_FORCE;
			writel(chconf,
			       regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
			writel(0, regs + OMAP3_MCSPI_CHCTRL0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
			readl(regs + OMAP3_MCSPI_CHCONF0 + spi->chip_select * OMAP3_MCSPI_CH_SIZE);
		}

		ret = 0;
	} else if ((t->tx_buf != NULL) && (t->rx_buf != NULL)) {
		printf("SPI error: full duplex unsupported\n");
		ret = -EINVAL;
	} else if (t->tx_buf != NULL) {
		ret = omap3_spi_write(spi, len, txp, flags);
	} else if (t->rx_buf != NULL) {
		ret = omap3_spi_read(spi, len, rxp, flags);
	} else {
		printf("SPI error: neither tx_buf nor rx_buf set\n");
		ret = -EINVAL;
	}
	return ret;
}

static int omap3_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct spi_master *master = spi->master;
	struct spi_transfer *t, *t_first, *t_last = NULL;
	unsigned long flags;
	int ret = 0;

	ret = spi_claim_bus(spi);
	if (ret)
		return ret;

	if (list_empty(&mesg->transfers))
		return 0;

	t_first = list_first_entry(&mesg->transfers, struct spi_transfer, transfer_list);
	t_last = list_last_entry(&mesg->transfers, struct spi_transfer, transfer_list);

	mesg->actual_length = 0;

	dev_dbg(master->dev, "transfer start actual_length=%i\n", mesg->actual_length);
	list_for_each_entry(t, &mesg->transfers, transfer_list) {
		dev_dbg(master->dev,
			"  xfer %p: len %u tx %p rx %p\n",
			t, t->len, t->tx_buf, t->rx_buf);
		flags = 0;
		if (t == t_first)
			flags |= SPI_XFER_BEGIN;
		if (t == t_last)
			flags |= SPI_XFER_END;
		ret = spi_xfer(spi, t, flags);
		if (ret < 0)
			return ret;
		mesg->actual_length += t->len;
	}
	dev_dbg(master->dev, "transfer done actual_length=%i\n", mesg->actual_length);

	return ret;
}

static int omap3_spi_setup(struct spi_device *spi)
{
	if (spi->max_speed_hz > OMAP3_MCSPI_MAX_FREQ) {
		printf("SPI error: unsupported frequency %i Hz. \
			Max frequency is 48 Mhz\n", spi->max_speed_hz);
		return -EINVAL;
	}

	if (spi->mode > SPI_MODE_3) {
		printf("SPI error: unsupported SPI mode %i\n", spi->mode);
		return -EINVAL;
	}

	return 0;
}

static int omap3_spi_probe_dt(struct device_d *dev, struct omap3_spi_master *omap3_master)
{
	if (!IS_ENABLED(CONFIG_OFDEVICE) || !dev->device_node)
		return -ENODEV;

	if (of_property_read_bool(dev->device_node, "ti,pindir-d0-out-d1-in"))
		omap3_master->swap_miso_mosi = 1;

	return 0;
}

static int omap3_spi_probe(struct device_d *dev)
{
	struct resource *iores;
	struct spi_master *master;
	struct omap3_spi_master *omap3_master;
	struct omap_spi_drvdata *devtype;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	omap3_master = xzalloc(sizeof(*omap3_master));

	omap3_spi_probe_dt(dev, omap3_master);

	master = &omap3_master->master;
	master->dev = dev;

	/*
	 * OMAP3 McSPI (MultiChannel SPI) has 4 busses (modules)
	 * with different number of chip selects (CS, channels):
	 * McSPI1 has 4 CS (bus 1, cs 0 - 3)
	 * McSPI2 has 2 CS (bus 2, cs 0 - 1)
	 * McSPI3 has 2 CS (bus 3, cs 0 - 1)
	 * McSPI4 has 1 CS (bus 4, cs 0)
	 *
	 * AM335x McSPI has 2 busses with 2 chip selects:
	 * McSPI0 has 2 CS (bus 0, cs 0 - 1)
	 * McSPI1 has 2 CS (bus 1, cs 0 - 1)
	 *
	 * The board code has to make sure that it does not use
	 * invalid buses or chip selects.
	 */

	master->bus_num = dev->id;

	if (IS_ENABLED(CONFIG_ARCH_OMAP3))
		master->num_chipselect = 4;
	else
		master->num_chipselect = 2;
	master->setup = omap3_spi_setup;
	master->transfer = omap3_spi_transfer;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	omap3_master->base = IOMEM(iores->start);
	omap3_master->regs = omap3_master->base;

	omap3_master->regs += devtype->register_offset;

	spi_reset(master);

	spi_register_master(master);

	return 0;
}

static struct omap_spi_drvdata omap3_data = {
	.register_offset = 0x0,
};

static struct omap_spi_drvdata omap4_data = {
	.register_offset = 0x100,
};

static __maybe_unused struct of_device_id omap_spi_dt_ids[] = {
	{
		.compatible = "ti,omap2-mcspi",
		.data = &omap3_data,
	}, {
		.compatible = "ti,omap4-mcspi",
		.data = &omap4_data,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id omap_spi_ids[] = {
	{
		.name = "omap3-spi",
		.driver_data = (unsigned long)&omap3_data,
	}, {
		.name = "omap4-spi",
		.driver_data = (unsigned long)&omap4_data,
	}, {
		/* sentinel */
	},
};

static struct driver_d omap3_spi_driver = {
	.name = "omap-spi",
	.probe = omap3_spi_probe,
	.of_compatible = DRV_OF_COMPAT(omap_spi_dt_ids),
	.id_table = omap_spi_ids,
};
device_platform_driver(omap3_spi_driver);

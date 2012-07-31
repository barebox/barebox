/*
 * Driver for Atmel AT32 and AT91 SPI Controllers
 *
 * Copyright (C) 2011 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on imx_spi.c by:
 * Copyright (C) 2008 Sascha Hauer, Pengutronix
 *
 * based on atmel_spi.c from the linux kernel by:
 * Copyright (C) 2006 Atmel Corporation
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
#include <errno.h>
#include <clock.h>
#include <xfuncs.h>
#include <gpio.h>
#include <io.h>
#include <spi/spi.h>
#include <mach/io.h>
#include <mach/board.h>
#include <mach/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "atmel_spi.h"

struct atmel_spi {
	struct spi_master	master;
	void __iomem		*regs;
	struct clk		*clk;
	int			*cs_pins;
};

#define to_atmel_spi(p)		container_of(p, struct atmel_spi, master)
#define SPI_XCHG_TIMEOUT	(100 * MSECOND)

/*
 * Version 2 of the SPI controller has
 *  - CR.LASTXFER
 *  - SPI_MR.DIV32 may become FDIV or must-be-zero (here: always zero)
 *  - SPI_SR.TXEMPTY, SPI_SR.NSSR (and corresponding irqs)
 *  - SPI_CSRx.CSAAT
 *  - SPI_CSRx.SBCR allows faster clocking
 *
 * We can determine the controller version by reading the VERSION
 * register, but I haven't checked that it exists on all chips, and
 * this is cheaper anyway.
 */
static inline bool atmel_spi_is_v2(void)
{
	return !cpu_is_at91rm9200();
}

static int atmel_spi_setup(struct spi_device *spi)
{
	struct spi_master	*master = spi->master;
	struct atmel_spi	*as = to_atmel_spi(master);

	u32			scbr, csr;
	unsigned int		bits = spi->bits_per_word;
	unsigned long		bus_hz;

	if (spi->controller_data) {
		csr = (u32)spi->controller_data;
		spi_writel(as, CSR0, csr);
		return 0;
	}

	dev_dbg(master->dev, "%s mode 0x%08x bits_per_word: %d speed: %d\n",
			__func__, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);

	bus_hz = clk_get_rate(as->clk);
	if (!atmel_spi_is_v2())
		bus_hz /= 2;

	if (spi->max_speed_hz) {
		/*
		 * Calculate the lowest divider that satisfies the
		 * constraint, assuming div32/fdiv/mbz == 0.
		 */
		scbr = DIV_ROUND_UP(bus_hz, spi->max_speed_hz);

		/*
		 * If the resulting divider doesn't fit into the
		 * register bitfield, we can't satisfy the constraint.
		 */
		if (scbr >= (1 << SPI_SCBR_SIZE)) {
			dev_dbg(master->dev,
				"setup: %d Hz too slow, scbr %u; min %ld Hz\n",
				spi->max_speed_hz, scbr, bus_hz/255);
			return -EINVAL;
		}
	} else {
		/* speed zero means "as slow as possible" */
		scbr = 0xff;
	}

	csr = SPI_BF(SCBR, scbr) | SPI_BF(BITS, bits - 8);
	if (spi->mode & SPI_CPOL)
		csr |= SPI_BIT(CPOL);
	if (!(spi->mode & SPI_CPHA))
		csr |= SPI_BIT(NCPHA);

	/* DLYBS is mostly irrelevant since we manage chipselect using GPIOs.
	 *
	 * DLYBCT would add delays between words, slowing down transfers.
	 * It could potentially be useful to cope with DMA bottlenecks, but
	 * in those cases it's probably best to just use a lower bitrate.
	 */
	csr |= SPI_BF(DLYBS, 0);
	csr |= SPI_BF(DLYBCT, 0);

	/* gpio_direction_output(npcs_pin, !(spi->mode & SPI_CS_HIGH)); */
	dev_dbg(master->dev,
		"setup: %lu Hz bpw %u mode 0x%x -> csr%d %08x\n",
		bus_hz / scbr, bits, spi->mode, spi->chip_select, csr);

	spi_writel(as, CSR0, csr);

	/*
	 * store the csr-setting when bits are defined. This happens usually
	 * after the specific spi_device driver has been probed.
	 */
	if (bits > 0)
		spi->controller_data = (void *)csr;

	return 0;
}

static void atmel_spi_chipselect(struct spi_device *spi, struct atmel_spi *as, int on)
{
	struct spi_master *master = &as->master;
	int cs_pin;
	int val = ((spi->mode & SPI_CS_HIGH) != 0) == on;

	BUG_ON(spi->chip_select >= master->num_chipselect);
	cs_pin = as->cs_pins[spi->chip_select];

	gpio_direction_output(cs_pin, val);
}

static int atmel_spi_xchg(struct atmel_spi *as, u32 tx_val)
{
	uint64_t start;

	start = get_time_ns();
	while (!(spi_readl(as, SR) & SPI_BIT(TDRE))) {
		if (is_timeout(start, SPI_XCHG_TIMEOUT)) {
			dev_err(as->master.dev, "tx timeout\n");
			return -ETIMEDOUT;
		}
	}
	spi_writel(as, TDR, tx_val);

	start = get_time_ns();
	while (!(spi_readl(as, SR) & SPI_BIT(RDRF))) {
		if (is_timeout(start, SPI_XCHG_TIMEOUT)) {
			dev_err(as->master.dev, "rx timeout\n");
			return -ETIMEDOUT;
		}
	}
	return spi_readl(as, RDR) & 0xffff;
}

static int atmel_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	int ret;
	struct spi_master *master	= spi->master;
	struct atmel_spi *as		= to_atmel_spi(master);
	struct spi_transfer *t		= NULL;
	unsigned int bits		= spi->bits_per_word;

	mesg->actual_length = 0;
	ret = master->setup(spi);
	if (ret < 0) {
		dev_dbg(master->dev, "transfer: master setup failed\n");
		return ret;
	}

	dev_dbg(master->dev, "  csr0: %08x\n", spi_readl(as, CSR0));

#ifdef VERBOSE
	list_for_each_entry(t, &mesg->transfers, transfer_list) {
		dev_dbg(master->dev,
			"  xfer %p: len %u tx %p rx %p\n",
			t, t->len, t->tx_buf, t->rx_buf);
	}
#endif
	atmel_spi_chipselect(spi, as, 1);
	list_for_each_entry(t, &mesg->transfers, transfer_list) {
		u32 tx_val;
		int i = 0, rx_val;

		mesg->actual_length += t->len;
		if (bits <= 8) {
			const u8 *txbuf = t->tx_buf;
			u8 *rxbuf = t->rx_buf;

			while (i < t->len) {
				tx_val = txbuf ? txbuf[i] : 0;

				rx_val = atmel_spi_xchg(as, tx_val);
				if (rx_val < 0) {
					ret = rx_val;
					goto out;
				}

				if (rxbuf)
					rxbuf[i] = rx_val;
				i++;
			}
		} else if (bits <= 16) {
			const u16 *txbuf = t->tx_buf;
			u16 *rxbuf = t->rx_buf;

			while (i < t->len >> 1) {
				tx_val = txbuf ? txbuf[i] : 0;

				rx_val = atmel_spi_xchg(as, tx_val);
				if (rx_val < 0) {
					ret = rx_val;
					goto out;
				}

				if (rxbuf)
					rxbuf[i] = rx_val;
				i++;
			}
		}
	}
out:
	atmel_spi_chipselect(spi, as, 0);
	return ret;
}

static int atmel_spi_probe(struct device_d *dev)
{
	int ret = 0;
	struct spi_master *master;
	struct atmel_spi *as;
	struct at91_spi_platform_data *pdata = dev->platform_data;

	if (!pdata) {
		dev_err(dev, "missing platform data\n");
		return -EINVAL;
	}

	as = xzalloc(sizeof(*as));

	master = &as->master;
	master->dev = dev;

	as->clk = clk_get(dev, "spi_clk");
	if (IS_ERR(as->clk)) {
		dev_err(dev, "no spi_clk\n");
		ret = PTR_ERR(as->clk);
		goto out_free;
	}

	master->setup = atmel_spi_setup;
	master->transfer = atmel_spi_transfer;
	master->num_chipselect = pdata->num_chipselect;
	as->cs_pins = pdata->chipselect;
	as->regs = dev_request_mem_region(dev, 0);

	/* Initialize the hardware */
	clk_enable(as->clk);
	spi_writel(as, CR, SPI_BIT(SWRST));
	spi_writel(as, CR, SPI_BIT(SWRST)); /* AT91SAM9263 Rev B workaround */
	spi_writel(as, MR, SPI_BIT(MSTR) | SPI_BIT(MODFDIS));
	spi_writel(as, PTCR, SPI_BIT(RXTDIS) | SPI_BIT(TXTDIS));
	spi_writel(as, CR, SPI_BIT(SPIEN));

	dev_dbg(dev, "Atmel SPI Controller at initialized\n");

	ret = spi_register_master(master);
	if (ret)
		goto out_reset_hw;

	return 0;

out_reset_hw:
	spi_writel(as, CR, SPI_BIT(SWRST));
	spi_writel(as, CR, SPI_BIT(SWRST)); /* AT91SAM9263 Rev B workaround */
	clk_disable(as->clk);
	clk_put(as->clk);
out_free:
	free(as);
	return ret;
}

static struct driver_d atmel_spi_driver = {
	.name  = "atmel_spi",
	.probe = atmel_spi_probe,
};

static int atmel_spi_init(void)
{
	register_driver(&atmel_spi_driver);
	return 0;
}

device_initcall(atmel_spi_init);

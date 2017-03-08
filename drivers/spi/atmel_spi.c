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
#include <mach/iomux.h>
#include <mach/board.h>
#include <mach/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "atmel_spi.h"

struct atmel_spi_caps {
	bool	is_spi2;
};

struct atmel_spi {
	struct spi_master	master;
	void __iomem		*regs;
	struct clk		*clk;
	int			*cs_pins;
	struct atmel_spi_caps	caps;
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
static inline bool atmel_spi_is_v2(struct atmel_spi *as)
{
	return as->caps.is_spi2;
}

/*
 * Earlier SPI controllers (e.g. on at91rm9200) have a design bug whereby
 * they assume that spi slave device state will not change on deselect, so
 * that automagic deselection is OK.  ("NPCSx rises if no data is to be
 * transmitted")  Not so!  Workaround uses nCSx pins as GPIOs; or newer
 * controllers have CSAAT and friends.
 *
 * Since the CSAAT functionality is a bit weird on newer controllers as
 * well, we use GPIO to control nCSx pins on all controllers, updating
 * MR.PCS to avoid confusing the controller.  Using GPIOs also lets us
 * support active-high chipselects despite the controller's belief that
 * only active-low devices/systems exists.
 *
 * However, at91rm9200 has a second erratum whereby nCS0 doesn't work
 * right when driven with GPIO.  ("Mode Fault does not allow more than one
 * Master on Chip Select 0.")  No workaround exists for that ... so for
 * nCS0 on that chip, we (a) don't use the GPIO, (b) can't support CS_HIGH,
 * and (c) will trigger that first erratum in some cases.
 *
 * TODO: Test if the atmel_spi_is_v2() branch below works on
 * AT91RM9200 if we use some other register than CSR0. However, don't
 * do this unconditionally since AP7000 has an errata where the BITS
 * field in CSR0 overrides all other CSRs.
 */

static void cs_activate(struct atmel_spi *as, struct spi_device *spi)
{
	struct spi_master *master = &as->master;
	int npcs_pin;
	unsigned active = spi->mode & SPI_CS_HIGH;
	u32 mr, csr;

	BUG_ON(spi->chip_select >= master->num_chipselect);
	npcs_pin = as->cs_pins[spi->chip_select];

	csr = (u32)spi->controller_data;

	if (atmel_spi_is_v2(as)) {
		/*
		 * Always use CSR0. This ensures that the clock
		 * switches to the correct idle polarity before we
		 * toggle the CS.
		 */
		spi_writel(as, CSR0, csr);
		spi_writel(as, MR, SPI_BF(PCS, 0x0e) | SPI_BIT(MODFDIS)
				| SPI_BIT(MSTR));
		mr = spi_readl(as, MR);
		gpio_set_value(npcs_pin, active);
	} else {
		u32 cpol = (spi->mode & SPI_CPOL) ? SPI_BIT(CPOL) : 0;
		int i;
		u32 csr;

		/* Make sure clock polarity is correct */
		for (i = 0; i < spi->master->num_chipselect; i++) {
			csr = spi_readl(as, CSR0 + 4 * i);
			if ((csr ^ cpol) & SPI_BIT(CPOL))
				spi_writel(as, CSR0 + 4 * i,
						csr ^ SPI_BIT(CPOL));
		}

		mr = spi_readl(as, MR);
		mr = SPI_BFINS(PCS, ~(1 << spi->chip_select), mr);
		if (npcs_pin != AT91_PIN_PA3)
			gpio_set_value(npcs_pin, active);
		spi_writel(as, MR, mr);
	}

	dev_dbg(&spi->dev, "activate %u%s, mr %08x\n",
			npcs_pin, active ? " (high)" : "",
			mr);
}

static void cs_deactivate(struct atmel_spi *as, struct spi_device *spi)
{
	struct spi_master *master = &as->master;
	int npcs_pin;
	unsigned active = spi->mode & SPI_CS_HIGH;
	u32 mr;

	BUG_ON(spi->chip_select >= master->num_chipselect);
	npcs_pin = as->cs_pins[spi->chip_select];

	/* only deactivate *this* device; sometimes transfers to
	 * another device may be active when this routine is called.
	 */
	mr = spi_readl(as, MR);
	if (~SPI_BFEXT(PCS, mr) & (1 << spi->chip_select)) {
		mr = SPI_BFINS(PCS, 0xf, mr);
		spi_writel(as, MR, mr);
	}

	dev_dbg(&spi->dev, "DEactivate %u%s, mr %08x\n",
			npcs_pin, active ? " (low)" : "",
			mr);

	if (atmel_spi_is_v2(as) || npcs_pin != AT91_PIN_PA3) {
		gpio_set_value(npcs_pin, !active);
	}
}

static int atmel_spi_setup(struct spi_device *spi)
{
	struct spi_master	*master = spi->master;
	struct atmel_spi	*as = to_atmel_spi(master);

	int			npcs_pin;
	unsigned		active = spi->mode & SPI_CS_HIGH;
	u32			scbr, csr;
	unsigned int		bits = spi->bits_per_word;
	unsigned long		bus_hz;

	if (spi->chip_select > spi->master->num_chipselect) {
		dev_dbg(&spi->dev,
				"setup: invalid chipselect %u (%u defined)\n",
				spi->chip_select, spi->master->num_chipselect);
		return -EINVAL;
	}

	npcs_pin = as->cs_pins[spi->chip_select];

	if (bits < 8 || bits > 16) {
		dev_dbg(&spi->dev,
				"setup: invalid bits_per_word %u (8 to 16)\n",
				bits);
		return -EINVAL;
	}

	dev_dbg(master->dev, "%s mode 0x%08x bits_per_word: %d speed: %d\n",
			__func__, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);

	bus_hz = clk_get_rate(as->clk);
	if (!atmel_spi_is_v2(as))
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

	gpio_direction_output(npcs_pin, !active);
	dev_dbg(master->dev,
		"setup: %lu Hz bpw %u mode 0x%x -> csr%d %08x\n",
		bus_hz / scbr, bits, spi->mode, spi->chip_select, csr);

	spi->controller_data = (void *)csr;

	cs_deactivate(as, spi);

	if (!atmel_spi_is_v2(as))
		spi_writel(as, CSR0 + 4 * spi->chip_select, csr);

	return 0;
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

static int atmel_spi_do_xfer(struct spi_device *spi, struct atmel_spi *as,
			     struct spi_transfer *t)
{
	unsigned int bits = spi->bits_per_word;
	u32 tx_val;
	int i = 0, rx_val;

	if (bits <= 8) {
		const u8 *txbuf = t->tx_buf;
		u8 *rxbuf = t->rx_buf;

		while (i < t->len) {
			tx_val = txbuf ? txbuf[i] : 0;

			rx_val = atmel_spi_xchg(as, tx_val);
			if (rx_val < 0)
				return rx_val;

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
			if (rx_val < 0)
				return rx_val;

			if (rxbuf)
				rxbuf[i] = rx_val;
			i++;
		}
	}

	return t->len;
}

static int atmel_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	int ret;
	struct spi_master *master	= spi->master;
	struct atmel_spi *as		= to_atmel_spi(master);
	struct spi_transfer *t		= NULL;
	unsigned int cs_change;

	mesg->actual_length = 0;

	dev_dbg(master->dev, "  csr0: %08x\n", spi_readl(as, CSR0));

#ifdef VERBOSE
	list_for_each_entry(t, &mesg->transfers, transfer_list) {
		dev_dbg(master->dev,
			"  xfer %p: len %u tx %p rx %p\n",
			t, t->len, t->tx_buf, t->rx_buf);
	}
#endif

	cs_activate(as, spi);

	cs_change = 0;

	list_for_each_entry(t, &mesg->transfers, transfer_list) {

		if (cs_change) {
			udelay(1);
			cs_deactivate(as, spi);
			udelay(1);
			cs_activate(as, spi);
		}

		cs_change = t->cs_change;

		ret = atmel_spi_do_xfer(spi, as, t);
		if (ret < 0)
			goto err;
		mesg->actual_length += ret;

		if (cs_change)
			cs_activate(as, spi);
	}

	if (!cs_change)
		cs_deactivate(as, spi);

	return 0;

err:
	cs_deactivate(as, spi);
	return ret;
}

static inline unsigned int atmel_get_version(struct atmel_spi *as)
{
	return spi_readl(as, VERSION) & 0x00000fff;
}

static void atmel_get_caps(struct atmel_spi *as)
{
	unsigned int version;

	version = atmel_get_version(as);
	dev_info(as->master.dev, "version: 0x%x\n", version);

	as->caps.is_spi2 = version > 0x121;
}

static int atmel_spi_probe(struct device_d *dev)
{
	struct resource *iores;
	int ret = 0;
	int i;
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
	master->bus_num = dev->id;

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
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	as->regs = IOMEM(iores->start);

	atmel_get_caps(as);

	for (i = 0; i < master->num_chipselect; i++) {
		ret = gpio_request(as->cs_pins[i], dev_name(dev));
		if (ret)
			goto out_gpio;
	}

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
out_gpio:
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
device_platform_driver(atmel_spi_driver);

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <io.h>

struct litex_spiflash_spi {
	struct spi_master	master;
	void __iomem		*regs;
	u32			val;
};

#define SPIFLASH_BITBANG 0x0
#define  SPIFLASH_BB_MOSI BIT(0)
#define  SPIFLASH_BB_CLK BIT(1)
#define  SPIFLASH_BB_CSN BIT(2)
#define  SPIFLASH_BB_DIR BIT(3)

#define SPIFLASH_MISO 0x4
#define SPIFLASH_BITBANG_EN 0x8

static inline u32 litex_spiflash_spi_rr(struct litex_spiflash_spi *sp, int reg)
{
	return readl(sp->regs + reg);
}

static inline void litex_spiflash_spi_wr(struct litex_spiflash_spi *sp, u32 val, int reg)
{
	writel(val, sp->regs + reg);
}

static inline void litex_setbits(struct litex_spiflash_spi *sp, int bits, int on)
{
	/*
	 * We are the only user of SCSPTR so no locking is required.
	 * Reading bit 2 and 0 in SCSPTR gives pin state as input.
	 * Writing the same bits sets the output value.
	 * This makes regular read-modify-write difficult so we
	 * use sp->val to keep track of the latest register value.
	 */

	if (on)
		sp->val |= bits;
	else
		sp->val &= ~bits;

	litex_spiflash_spi_wr(sp, sp->val, SPIFLASH_BITBANG);
}

static inline struct litex_spiflash_spi *litex_spiflash_spidev_to_sp(struct spi_device *spi)
{
	return container_of(spi->master, struct litex_spiflash_spi, master);
}

static inline void setsck(struct spi_device *spi, int on)
{
	struct litex_spiflash_spi *sc = litex_spiflash_spidev_to_sp(spi);

	litex_setbits(sc, SPIFLASH_BB_CLK, on);
}

static inline void setmosi(struct spi_device *spi, int on)
{
	struct litex_spiflash_spi *sc = litex_spiflash_spidev_to_sp(spi);

	sc->val &= ~SPIFLASH_BB_DIR;
	litex_setbits(sc, SPIFLASH_BB_MOSI, on);
}

static inline u32 getmiso(struct spi_device *spi)
{
	struct litex_spiflash_spi *sc = litex_spiflash_spidev_to_sp(spi);

	litex_setbits(sc, SPIFLASH_BB_DIR, 1);
	return !!((litex_spiflash_spi_rr(sc, SPIFLASH_MISO) & 1));
}

#define spidelay(nsecs) udelay(nsecs/1000)

#include "spi-bitbang-txrx.h"

static inline void litex_spiflash_spi_chipselect(struct litex_spiflash_spi *sc, int on)
{
	litex_setbits(sc, SPIFLASH_BB_CSN, on);
}

static int litex_spiflash_spi_setup(struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	struct device spi_dev = spi->dev;

	if (spi->bits_per_word != 8) {
		dev_err(master->dev, "master doesn't support %d bits per word requested by %s\n",
			spi->bits_per_word, spi_dev.name);
		return -EINVAL;
	}

	if ((spi->mode & (SPI_CPHA | SPI_CPOL)) != SPI_MODE_0) {
		dev_err(master->dev, "master doesn't support SPI_MODE%d requested by %s\n",
			spi->mode & (SPI_CPHA | SPI_CPOL), spi_dev.name);
		return -EINVAL;
	}

	return 0;
}

static int litex_spiflash_spi_read(struct spi_device *spi, void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	u8 *rxf_buf = buf;

	while (cnt < nbyte) {
		*rxf_buf = bitbang_txrx_be_cpha1(spi, 1000, 1, 0, 8);
		rxf_buf++;
		cnt++;
	}

	return cnt;
}

static int litex_spiflash_spi_write(struct spi_device *spi,
				const void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	const u8 *txf_buf = buf;

	while (cnt < nbyte) {
		bitbang_txrx_be_cpha1(spi, 1000, 1, (u32)*txf_buf, 8);
		txf_buf++;
		cnt++;
	}

	return 0;
}

static int litex_spiflash_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct litex_spiflash_spi *sc = litex_spiflash_spidev_to_sp(spi);
	struct spi_transfer *t;

	mesg->actual_length = 0;

	/* activate chip select signal */
	litex_spiflash_spi_chipselect(sc, 0);

	list_for_each_entry(t, &mesg->transfers, transfer_list) {

		if (t->tx_buf)
			litex_spiflash_spi_write(spi, t->tx_buf, t->len);

		if (t->rx_buf)
			litex_spiflash_spi_read(spi, t->rx_buf, t->len);

		mesg->actual_length += t->len;
	}

	/* inactivate chip select signal */
	litex_spiflash_spi_chipselect(sc, 1);

	return 0;
}

static void litex_spiflash_spi_enable(struct litex_spiflash_spi *sp)
{
	u32 val;

	/* set SPIFLASH_BB_DIR = 0 */
	val = SPIFLASH_BB_CSN | SPIFLASH_BB_CLK | SPIFLASH_BB_MOSI;
	litex_spiflash_spi_wr(sp, val, SPIFLASH_BITBANG);

	/* enable GPIO mode */
	litex_spiflash_spi_wr(sp, 1, SPIFLASH_BITBANG_EN);
}

static void litex_spiflash_spi_disable(struct litex_spiflash_spi *sp)
{
	/* disable GPIO mode */
	litex_spiflash_spi_wr(sp, 0, SPIFLASH_BITBANG_EN);
}

static int litex_spiflash_spi_probe(struct device *dev)
{
	struct resource *iores;
	struct spi_master *master;
	struct litex_spiflash_spi *litex_spiflash_spi;

	litex_spiflash_spi = xzalloc(sizeof(*litex_spiflash_spi));
	dev->priv = litex_spiflash_spi;

	master = &litex_spiflash_spi->master;
	master->dev = dev;

	master->bus_num = dev->id;
	master->setup = litex_spiflash_spi_setup;
	master->transfer = litex_spiflash_spi_transfer;
	master->num_chipselect = 1;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	litex_spiflash_spi->regs = IOMEM(iores->start);

	litex_spiflash_spi_enable(litex_spiflash_spi);

	/* inactivate chip select signal */
	litex_spiflash_spi_chipselect(litex_spiflash_spi, 1);

	spi_register_master(master);

	return 0;
}

static void litex_spiflash_spi_remove(struct device *dev)
{
	struct litex_spiflash_spi *sp = dev->priv;

	litex_spiflash_spi_disable(sp);
}

static __maybe_unused struct of_device_id litex_spiflash_spi_dt_ids[] = {
	{
		.compatible = "litex,spiflash",
	},
	{
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, litex_spiflash_spi_dt_ids);

static struct driver litex_spiflash_spi_driver = {
	.name  = "litex-spiflash",
	.probe = litex_spiflash_spi_probe,
	.remove = litex_spiflash_spi_remove,
	.of_compatible = DRV_OF_COMPAT(litex_spiflash_spi_dt_ids),
};
device_platform_driver(litex_spiflash_spi_driver);

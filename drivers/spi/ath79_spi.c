// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2013, 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <io.h>
#include <clock.h>

struct ath79_spi {
	struct spi_master	master;
	void __iomem		*regs;
	u32			val;
	u32			reg_ctrl;
};

#define AR71XX_SPI_REG_FS	0x00	/* Function Select */
#define AR71XX_SPI_REG_CTRL	0x04	/* SPI Control */
#define AR71XX_SPI_REG_IOC	0x08	/* SPI I/O Control */
#define AR71XX_SPI_REG_RDS	0x0c	/* Read Data Shift */

#define AR71XX_SPI_FS_GPIO	BIT(0)	/* Enable GPIO mode */

#define AR71XX_SPI_IOC_DO	BIT(0)	/* Data Out pin */
#define AR71XX_SPI_IOC_CLK	BIT(8)	/* CLK pin */
#define AR71XX_SPI_IOC_CS(n)	BIT(16 + (n))
#define AR71XX_SPI_IOC_CS0	AR71XX_SPI_IOC_CS(0)
#define AR71XX_SPI_IOC_CS1	AR71XX_SPI_IOC_CS(1)
#define AR71XX_SPI_IOC_CS2	AR71XX_SPI_IOC_CS(2)
#define AR71XX_SPI_IOC_CS_ALL	(AR71XX_SPI_IOC_CS0 | AR71XX_SPI_IOC_CS1 | \
				 AR71XX_SPI_IOC_CS2)

static inline u32 ath79_spi_rr(struct ath79_spi *sp, int reg)
{
	return __raw_readl(sp->regs + reg);
}

static inline void ath79_spi_wr(struct ath79_spi *sp, u32 val, int reg)
{
	__raw_writel(val, sp->regs + reg);
}

static inline void ath79_spi_setbits(struct ath79_spi *sp, int bits, int on)
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

	ath79_spi_wr(sp, sp->val, AR71XX_SPI_REG_IOC);
}

static inline struct ath79_spi *ath79_spidev_to_sp(struct spi_device *spi)
{
	return container_of(spi->master, struct ath79_spi, master);
}

static inline void setsck(struct spi_device *spi, int on)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);

	ath79_spi_setbits(sc, AR71XX_SPI_IOC_CLK, on);
}

static inline void setmosi(struct spi_device *spi, int on)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);

	ath79_spi_setbits(sc, AR71XX_SPI_IOC_DO, on);
}

static inline u32 getmiso(struct spi_device *spi)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);

	return !!((ath79_spi_rr(sc, AR71XX_SPI_REG_RDS) & 1));
}

#define spidelay(nsecs) udelay(nsecs/1000)

#include "spi-bitbang-txrx.h"

static inline void ath79_spi_chipselect(struct ath79_spi *sp, int chipselect)
{
	int off_bits;

	off_bits = 0xffffffff;

	switch (chipselect) {
	case 0:
		off_bits &= ~AR71XX_SPI_IOC_CS0;
		break;

	case 1:
		off_bits &= ~AR71XX_SPI_IOC_CS1;
		break;

	case 2:
		off_bits &= ~AR71XX_SPI_IOC_CS2;
		break;

	case 3:
		break;
	}

	/* by default inactivate chip selects */
	sp->val |= AR71XX_SPI_IOC_CS_ALL;
	sp->val &= off_bits;

	ath79_spi_wr(sp, sp->val, AR71XX_SPI_REG_IOC);
}

static int ath79_spi_setup(struct spi_device *spi)
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

static int ath79_spi_read(struct spi_device *spi, void *buf, size_t nbyte)
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

static int ath79_spi_write(struct spi_device *spi,
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

static int ath79_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);
	struct spi_transfer *t;

	mesg->actual_length = 0;

	/* activate chip select signal */
	ath79_spi_chipselect(sc, spi->chip_select);

	list_for_each_entry(t, &mesg->transfers, transfer_list) {

		if (t->tx_buf)
			ath79_spi_write(spi, t->tx_buf, t->len);

		if (t->rx_buf)
			ath79_spi_read(spi, t->rx_buf, t->len);

		mesg->actual_length += t->len;
	}

	/* inactivate chip select signal */
	ath79_spi_chipselect(sc, -1);

	return 0;
}

static void ath79_spi_enable(struct ath79_spi *sp)
{
	/* enable GPIO mode */
	ath79_spi_wr(sp, AR71XX_SPI_FS_GPIO, AR71XX_SPI_REG_FS);

	/* save CTRL register */
	sp->reg_ctrl = ath79_spi_rr(sp, AR71XX_SPI_REG_CTRL);
	sp->val = ath79_spi_rr(sp, AR71XX_SPI_REG_IOC);

	/* TODO: setup speed? */
	ath79_spi_wr(sp, 0x43, AR71XX_SPI_REG_CTRL);
}

static void ath79_spi_disable(struct ath79_spi *sp)
{
	/* restore CTRL register */
	ath79_spi_wr(sp, sp->reg_ctrl, AR71XX_SPI_REG_CTRL);
	/* disable GPIO mode */
	ath79_spi_wr(sp, 0, AR71XX_SPI_REG_FS);
}

static int ath79_spi_probe(struct device *dev)
{
	struct resource *iores;
	struct spi_master *master;
	struct ath79_spi *ath79_spi;

	ath79_spi = xzalloc(sizeof(*ath79_spi));
	dev->priv = ath79_spi;

	master = &ath79_spi->master;
	master->dev = dev;

	master->bus_num = dev->id;
	master->setup = ath79_spi_setup;
	master->transfer = ath79_spi_transfer;
	master->num_chipselect = 3;

	if (IS_ENABLED(CONFIG_OFDEVICE)) {
		struct device_node *node = dev->of_node;
		u32 num_cs;
		int ret;

		ret = of_property_read_u32(node, "num-chipselects", &num_cs);
		if (ret)
			num_cs = 3;

		if (num_cs > 3) {
			dev_err(dev, "master doesn't support num-chipselects > 3\n");
		}

		master->num_chipselect = num_cs;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	ath79_spi->regs = IOMEM(iores->start);

	/* enable gpio mode */
	ath79_spi_enable(ath79_spi);

	/* inactivate chip select signal */
	ath79_spi_chipselect(ath79_spi, -1);

	spi_register_master(master);

	return 0;
}

static void ath79_spi_remove(struct device *dev)
{
	struct ath79_spi *sp = dev->priv;

	ath79_spi_disable(sp);
}

static __maybe_unused struct of_device_id ath79_spi_dt_ids[] = {
	{
		.compatible = "qca,ar7100-spi",
	},
	{
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ath79_spi_dt_ids);

static struct driver ath79_spi_driver = {
	.name  = "ath79-spi",
	.probe = ath79_spi_probe,
	.remove = ath79_spi_remove,
	.of_compatible = DRV_OF_COMPAT(ath79_spi_dt_ids),
};
device_platform_driver(ath79_spi_driver);

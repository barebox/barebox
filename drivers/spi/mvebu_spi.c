/*
 * Marvell MVEBU SoC SPI controller
 *  compatible with Dove, Kirkwood, MV78x00, Armada 370/XP
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <spi/spi.h>
#include <linux/clk.h>
#include <linux/err.h>

#define SPI_IF_CTRL		0x00
#define  IF_CS_NUM(x)		((x) << 2)
#define  IF_CS_NUM_MASK		IF_CS_NUM(7)
#define  IF_READ_READY		BIT(1)
#define  IF_CS_ENABLE		BIT(0)
#define SPI_IF_CONFIG		0x04

#define  IF_RXLSBF		BIT(14)
#define  IF_TXLSBF		BIT(13)

#define  IF_CLK_DIV(x)		((x) << 11)
#define  IF_CLK_DIV_MASK	(0x7 << 11)

#define  IF_FAST_READ		BIT(10)
#define  IF_ADDRESS_LEN_4BYTE	(3 << 8)
#define  IF_ADDRESS_LEN_3BYTE	(2 << 8)
#define  IF_ADDRESS_LEN_2BYTE	(1 << 8)
#define  IF_ADDRESS_LEN_1BYTE	(0 << 8)
#define  IF_CLK_PRESCALE_POW8	BIT(7)
#define  IF_CLK_PRESCALE_POW4	BIT(6)
#define  IF_TRANSFER_2BYTE	BIT(5)
#define  IF_CLK_PRESCALE_POW2	BIT(4)
#define  IF_CLK_PRESCALE(x)	((x) & 0x0f)
#define  IF_CLK_PRE_PRESCALE(x)	(((((x) & 0x6) << 6) | ((x) & 0x1)) << 4)
#define  IF_CLK_PRESCALE_MASK	(IF_CLK_PRESCALE(0xf) | IF_CLK_PRE_PRESCALE(7))
#define SPI_DATA_OUT		0x08
#define SPI_DATA_IN		0x0c
#define SPI_INT_CAUSE		0x10
#define SPI_INT_MASK		0x14
#define  INT_READ_READY		BIT(0)

#define SPI_SPI_MAX_CS	8

struct mvebu_spi {
	struct spi_master master;
	void __iomem *base;
	struct clk *clk;
	bool data16;
	int (*set_baudrate)(struct mvebu_spi *p, u32 speed);
};

#define priv_from_spi_device(s)	\
	container_of(s->master, struct mvebu_spi, master);

static inline int mvebu_spi_set_cs(struct mvebu_spi *p, u8 cs, u8 mode, bool en)
{
	u32 val;

	/*
	 * Only Armada 370/XP support up to 8 CS signals, for the
	 * others this register bits are read-only
	 */
	if (cs > SPI_SPI_MAX_CS)
		return -EINVAL;

	if (mode & SPI_CS_HIGH)
		en = !en;

	val = IF_CS_NUM(cs);
	if (en)
		val |= IF_CS_ENABLE;

	writel(val, p->base + SPI_IF_CTRL);

	return 0;
}

static int mvebu_spi_set_transfer_size(struct mvebu_spi *p, int size)
{
	u32 val;

	if (size != 8 && size != 16)
		return -EINVAL;

	p->data16 = (size == 16);

	val = readl(p->base + SPI_IF_CONFIG) & ~IF_TRANSFER_2BYTE;
	if (p->data16)
		val |= IF_TRANSFER_2BYTE;
	writel(val, p->base + SPI_IF_CONFIG);

	return 0;
}

static int mvebu_spi_set_baudrate(struct mvebu_spi *p, u32 speed)
{
	u32 pscl, val;

	/* standard prescaler values: 1,2,4,6,...,30 */
	pscl = DIV_ROUND_UP(clk_get_rate(p->clk), speed);
	pscl = roundup(pscl, 2);

	dev_dbg(p->master.dev, "%s: clk = %lu, speed = %u, pscl = %d\n",
		__func__, clk_get_rate(p->clk), speed, pscl);

	if (pscl > 30)
		return -EINVAL;

	val = readl(p->base + SPI_IF_CONFIG) & ~(IF_CLK_PRESCALE_MASK);
	val |= IF_CLK_PRESCALE_POW2 | IF_CLK_PRESCALE(pscl/2);
	writel(val, p->base + SPI_IF_CONFIG);

	return 0;
}

#if defined(CONFIG_ARCH_ARMADA_370) || defined(CONFIG_ARCH_ARMADA_XP)
static int armada_370_xp_spi_set_baudrate(struct mvebu_spi *p, u32 speed)
{
	u32 pscl, pdiv = 0, val;

	/* prescaler values: 1,2,3,...,15 */
	pscl = DIV_ROUND_UP(clk_get_rate(p->clk), speed);

	/* additional prescaler divider: 1, 2, 4, 8, 16, 32, 64, 128 */
	while (pscl > 15 && pdiv <= 7) {
		pscl = DIV_ROUND_UP(pscl, 2);
		pdiv++;
	}

	dev_dbg(p->master.dev, "%s: clk = %lu, speed = %u, pscl = %u, pdiv = %u\n",
		__func__, clk_get_rate(p->clk), speed, pscl, pdiv);

	if (pscl > 15)
		return -EINVAL;

	val = readl(p->base + SPI_IF_CONFIG) & ~(IF_CLK_PRESCALE_MASK);
	val |= IF_CLK_PRE_PRESCALE(pdiv) | IF_CLK_PRESCALE(pscl);
	writel(val, p->base + SPI_IF_CONFIG);

	return 0;
}
#endif

#if defined(CONFIG_ARCH_DOVE)
static int dove_spi_set_baudrate(struct mvebu_spi *p, u32 speed)
{
	u32 pscl, sdiv, rate, val;

	/* prescaler values: 1,2,3,...,15 and 1,2,4,6,...,30 */
	pscl = DIV_ROUND_UP(clk_get_rate(p->clk), speed);
	if (pscl > 15)
		pscl = roundup(pscl, 2);

	/* additional sclk divider: 1, 2, 4, 8, 16 */
	sdiv = 0; rate = pscl;
	while (rate > 30 && sdiv <= 4) {
		rate /= 2;
		sdiv++;
	}

	dev_dbg(p->master.dev, "%s: clk = %lu, speed = %u, pscl = %d, sdiv = %d\n",
		__func__, clk_get_rate(p->clk), speed, pscl, sdiv);

	if (rate > 30 || sdiv > 4)
		return -EINVAL;

	val = readl(p->base + SPI_IF_CONFIG) &
		~(IF_CLK_DIV_MASK | IF_CLK_PRESCALE_MASK);

	val |= IF_CLK_DIV(sdiv);
	if (pscl > 15)
		val |= IF_CLK_PRESCALE_POW2 | IF_CLK_PRESCALE(pscl/2);
	else
		val |= IF_CLK_PRESCALE(pscl);
	writel(val, p->base + SPI_IF_CONFIG);

	return 0;
}
#endif

static int mvebu_spi_set_mode(struct mvebu_spi *p, u8 mode)
{
	u32 val;

	/*
	 * From public datasheets of Orion SoCs, it is unclear
	 * if the SPI controller supports setting CPOL/CPHA.
	 * Dove has an SCK_INV but as with the other SoCs, it
	 * is tagged with "Must be 1".
	 *
	 * For now, we just bail out if device requests any
	 * other mode than SPI_MODE0.
	 */

	if ((mode & (SPI_CPOL|SPI_CPHA)) != SPI_MODE_0) {
		pr_err("%s: unsupported SPI mode %02x\n", __func__, mode);
		return -EINVAL;
	}

	val = readl(p->base + SPI_IF_CONFIG);
	if (mode & SPI_LSB_FIRST)
		val |= IF_RXLSBF | IF_TXLSBF;
	else
		val &= ~(IF_RXLSBF | IF_TXLSBF);
	writel(val, p->base + SPI_IF_CONFIG);

	return 0;
}

static int mvebu_spi_setup(struct spi_device *spi)
{
	int ret;
	struct mvebu_spi *priv = priv_from_spi_device(spi);

	dev_dbg(&spi->dev, "%s: mode %02x, bits_per_word = %d, speed = %d\n",
		__func__, spi->mode, spi->bits_per_word, spi->max_speed_hz);

	ret = mvebu_spi_set_cs(priv, spi->chip_select, spi->mode, false);
	if (ret)
		return ret;
	ret = mvebu_spi_set_mode(priv, spi->mode);
	if (ret)
		return ret;
	ret = mvebu_spi_set_transfer_size(priv, spi->bits_per_word);
	if (ret)
		return ret;

	return priv->set_baudrate(priv, spi->max_speed_hz);
}

static inline int mvebu_spi_wait_for_read_ready(struct mvebu_spi *p)
{
	int ret;

	ret = wait_on_timeout(100 * USECOND,
			      readl(p->base + SPI_IF_CTRL) & IF_READ_READY);
	return ret;
}

static int mvebu_spi_do_transfer(struct spi_device *spi,
				 struct spi_transfer *t)
{
	const u8 *txdata = t->tx_buf;
	u8 *rxdata = t->rx_buf;
	int ret = 0, n, inc;
	struct mvebu_spi *priv = priv_from_spi_device(spi);

	if (t->bits_per_word)
		ret = mvebu_spi_set_transfer_size(priv, spi->bits_per_word);
	if (ret) {
		dev_err(&spi->dev, "Failed to set transfer size (bpw = %u)\n",
			(unsigned)spi->bits_per_word);
		return ret;
	}

	if (t->speed_hz)
		ret = priv->set_baudrate(priv, t->speed_hz);
	if (ret) {
		dev_err(&spi->dev, "Failed to set baudrate to %u Hz\n",
			(unsigned)t->speed_hz);
		return ret;
	}

	inc = (priv->data16) ? 2 : 1;
	for (n = 0; n < t->len; n += inc) {
		u32 data = 0;

		if (txdata)
			data = *txdata++;
		if (txdata && priv->data16)
			data |= (*txdata++ << 8);

		writel(data, priv->base + SPI_DATA_OUT);

		ret = mvebu_spi_wait_for_read_ready(priv);
		if (ret) {
			dev_err(&spi->dev, "timeout reading from device %s\n",
				dev_name(&spi->dev));
			return ret;
		}

		if (rxdata) {
			data = readl(priv->base + SPI_DATA_IN);
			*rxdata++ = (data & 0xff);
			if (priv->data16)
				*rxdata++ = (data >> 8) & 0xff;
		}
	}

	return 0;
}

static int mvebu_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_transfer *t;
	int ret;
	struct mvebu_spi *priv = priv_from_spi_device(spi);

	ret = mvebu_spi_set_mode(priv, spi->mode);
	if (ret) {
		dev_err(&spi->dev, "Failed to set mode (0x%x)\n", (unsigned)spi->mode);
		return ret;
	}

	ret = mvebu_spi_set_cs(priv, spi->chip_select, spi->mode, true);
	if (ret) {
		dev_err(&spi->dev, "Failed to set chip select\n");
		return ret;
	}

	msg->actual_length = 0;

	list_for_each_entry(t, &msg->transfers, transfer_list) {
		ret = mvebu_spi_do_transfer(spi, t);
		if (ret)
			goto err_transfer;
		msg->actual_length += t->len;
	}

	return mvebu_spi_set_cs(priv, spi->chip_select, spi->mode, false);

err_transfer:
	mvebu_spi_set_cs(priv, spi->chip_select, spi->mode, false);
	return ret;
}

static struct of_device_id mvebu_spi_dt_ids[] = {
#if defined(CONFIG_ARCH_ARMADA_370) || defined(CONFIG_ARCH_ARMADA_XP)
	{ .compatible = "marvell,armada-370-spi",
	  .data = &armada_370_xp_spi_set_baudrate },
	{ .compatible = "marvell,armada-xp-spi",
	  .data = &armada_370_xp_spi_set_baudrate },
#endif
#if defined(CONFIG_ARCH_DOVE)
	{ .compatible = "marvell,dove-spi",
	  .data = &dove_spi_set_baudrate },
#endif
	{ .compatible = "marvell,orion-spi",
	  .data = &mvebu_spi_set_baudrate },
	{ }
};

static int mvebu_spi_probe(struct device_d *dev)
{
	struct resource *iores;
	struct spi_master *master;
	struct mvebu_spi *priv;
	const struct of_device_id *match;
	int ret = 0;

	match = of_match_node(mvebu_spi_dt_ids, dev->device_node);
	if (!match)
		return -EINVAL;

	priv = xzalloc(sizeof(*priv));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	priv->base = IOMEM(iores->start);
	priv->set_baudrate = (void *)match->data;
	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		goto err_free;
	}

	master = &priv->master;
	master->dev = dev;
	master->bus_num = dev->id;
	master->setup = mvebu_spi_setup;
	master->transfer = mvebu_spi_transfer;
	master->num_chipselect = 8;

	ret = spi_register_master(master);
	if (!ret)
		return 0;

err_free:
	free(priv);

	return ret;
}

static struct driver_d mvebu_spi_driver = {
	.name  = "mvebu-spi",
	.probe = mvebu_spi_probe,
	.of_compatible = DRV_OF_COMPAT(mvebu_spi_dt_ids),
};
device_platform_driver(mvebu_spi_driver);

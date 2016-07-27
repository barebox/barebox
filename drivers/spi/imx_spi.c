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
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <spi/imx-spi.h>
#include <xfuncs.h>
#include <io.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <gpio.h>
#include <of_gpio.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <clock.h>

struct imx_spi {
	struct spi_master	master;
	int			*cs_array;
	void __iomem		*regs;
	struct clk		*clk;

	unsigned int		(*xchg_single)(struct imx_spi *imx, u32 data);
	void			(*do_transfer)(struct spi_device *spi);
	void			(*chipselect)(struct spi_device *spi, int active);

	const void *tx_buf;
	void *rx_buf;
	int xfer_len;
	int bits_per_word;
};

struct spi_imx_devtype_data {
	unsigned int		(*xchg_single)(struct imx_spi *imx, u32 data);
	void			(*do_transfer)(struct spi_device *spi);
	void			(*chipselect)(struct spi_device *spi, int active);
	void			(*init)(struct imx_spi *imx);
};

static int imx_spi_setup(struct spi_device *spi)
{
	struct imx_spi *imx = container_of(spi->master, struct imx_spi, master);

	imx->chipselect(spi, 0);

	debug("%s mode 0x%08x bits_per_word: %d speed: %d\n",
			__FUNCTION__, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);
	return 0;
}

static unsigned int imx_spi_maybe_reverse_bits(struct spi_device *spi, unsigned int word)
{
	unsigned int result = word;

	if (spi->mode & SPI_LSB_FIRST) {
		size_t bits_left = spi->bits_per_word - 1;

		for (word >>= 1; word; word >>= 1) {
			result <<= 1;
			result |= word & 1;
			bits_left--;
		}

		result <<= bits_left;
	}

	return result;
}

static unsigned int cspi_0_0_xchg_single(struct imx_spi *imx, unsigned int data)
{
	void __iomem *base = imx->regs;

	unsigned int cfg_reg = readl(base + CSPI_0_0_CTRL);

	writel(data, base + CSPI_0_0_TXDATA);

	cfg_reg |= CSPI_0_0_CTRL_XCH;

	writel(cfg_reg, base + CSPI_0_0_CTRL);

	while (!(readl(base + CSPI_0_0_INT) & CSPI_0_0_STAT_RR));

	return readl(base + CSPI_0_0_RXDATA);
}

static void cspi_0_0_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_master *master = spi->master;
	struct imx_spi *imx = container_of(master, struct imx_spi, master);
	void __iomem *base = imx->regs;
	unsigned int cs = 0;
	int gpio = imx->cs_array[spi->chip_select];
	u32 ctrl_reg;

	if (spi->mode & SPI_CS_HIGH)
		cs = 1;

	if (!is_active) {
		if (gpio >= 0)
			gpio_direction_output(gpio, !cs);
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

static void cspi_0_0_init(struct imx_spi *imx)
{
	void __iomem *base = imx->regs;

	writel(CSPI_0_0_RESET_START, base + CSPI_0_0_RESET);
	do {
	} while (readl(base + CSPI_0_0_RESET) & CSPI_0_0_RESET_START);
}

static unsigned int cspi_0_7_xchg_single(struct imx_spi *imx, unsigned int data)
{
	void __iomem *base = imx->regs;

	unsigned int cfg_reg = readl(base + CSPI_0_7_CTRL);

	writel(data, base + CSPI_0_7_TXDATA);

	cfg_reg |= CSPI_0_7_CTRL_XCH;

	writel(cfg_reg, base + CSPI_0_7_CTRL);

	while (!(readl(base + CSPI_0_7_STAT) & CSPI_0_7_STAT_RR))
		;

	return readl(base + CSPI_0_7_RXDATA);
}

/* MX1, MX31, MX35, MX51 CSPI */
static unsigned int spi_imx_clkdiv_2(unsigned int fin,
		unsigned int fspi)
{
	int i, div = 4;

	for (i = 0; i < 7; i++) {
		if (fspi * div >= fin)
			return i;
		div <<= 1;
	}

	return 7;
}

static void cspi_0_7_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_master *master = spi->master;
	struct imx_spi *imx = container_of(master, struct imx_spi, master);
	void __iomem *base = imx->regs;
	unsigned int cs = 0;
	int gpio = imx->cs_array[spi->chip_select];
	unsigned int reg = CSPI_0_7_CTRL_ENABLE | CSPI_0_7_CTRL_MASTER;

	if (spi->mode & SPI_CS_HIGH)
		cs = 1;

	if (!is_active) {
		if (gpio >= 0)
			gpio_direction_output(gpio, !cs);
		return;
	}

	reg |= spi_imx_clkdiv_2(clk_get_rate(imx->clk), spi->max_speed_hz) <<
		CSPI_0_7_CTRL_DR_SHIFT;

	if (cpu_is_mx31())
		reg |= ((spi->bits_per_word - 1) & CSPI_0_4_CTRL_BL_MASK)
			<< CSPI_0_4_CTRL_BL_SHIFT;
	else
		reg |= (spi->bits_per_word - 1) << CSPI_0_7_CTRL_BL_SHIFT;

	reg |= CSPI_0_7_CTRL_SSCTL;

	if (spi->mode & SPI_CPHA)
		reg |= CSPI_0_7_CTRL_PHA;
	if (spi->mode & SPI_CPOL)
		reg |= CSPI_0_7_CTRL_POL;
	if (spi->mode & SPI_CS_HIGH)
		reg |= CSPI_0_7_CTRL_SSPOL;
	if (gpio < 0) {
		if (cpu_is_mx31())
			reg |= (gpio + 32) << CSPI_0_4_CTRL_CS_SHIFT;
		else
			reg |= (gpio + 32) << CSPI_0_7_CTRL_CS_SHIFT;
	}

	writel(reg, base + CSPI_0_7_CTRL);

	if (gpio >= 0)
		gpio_set_value(gpio, cs);
}

static void cspi_0_7_init(struct imx_spi *imx)
{
	void __iomem *base = imx->regs;

	/* drain receive buffer */
	while (readl(base + CSPI_0_7_STAT) & CSPI_0_7_STAT_RR)
		readl(base + CSPI_0_7_RXDATA);
}

static unsigned int cspi_2_3_xchg_single(struct imx_spi *imx, unsigned int data)
{
	void __iomem *base = imx->regs;

	writel(data, base + CSPI_2_3_TXDATA);

	while (!(readl(base + CSPI_2_3_STAT) & CSPI_2_3_STAT_RR));

	return readl(base + CSPI_2_3_RXDATA);
}

static unsigned int cspi_2_3_clkdiv(unsigned int fin, unsigned int fspi)
{
	/*
	 * there are two 4-bit dividers, the pre-divider divides by
	 * $pre, the post-divider by 2^$post
	 */
	unsigned int pre, post;

	if (unlikely(fspi > fin))
		return 0;

	post = fls(fin) - fls(fspi);
	if (fin > fspi << post)
		post++;

	/* now we have: (fin <= fspi << post) with post being minimal */

	post = max(4U, post) - 4;
	if (unlikely(post > 0xf)) {
		pr_err("%s: cannot set clock freq: %u (base freq: %u)\n",
				__func__, fspi, fin);
		return 0xff;
	}

	pre = DIV_ROUND_UP(fin, fspi << post) - 1;

	pr_debug("%s: fin: %u, fspi: %u, post: %u, pre: %u\n",
			__func__, fin, fspi, post, pre);
	return (pre << CSPI_2_3_CTRL_PREDIV_OFFSET) |
		(post << CSPI_2_3_CTRL_POSTDIV_OFFSET);
}

static void cspi_2_3_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_master *master = spi->master;
	struct imx_spi *imx = container_of(master, struct imx_spi, master);
	void __iomem *base = imx->regs;
	unsigned int cs = spi->chip_select, gpio_cs = 0;
	int gpio = imx->cs_array[spi->chip_select];
	u32 ctrl, cfg = 0;

	if (spi->mode & SPI_CS_HIGH)
		gpio_cs = 1;

	if (!is_active) {
		if (gpio >= 0)
			gpio_direction_output(gpio, !gpio_cs);
		return;
	}

	ctrl = CSPI_2_3_CTRL_ENABLE;

	/* set master mode */
	ctrl |= CSPI_2_3_CTRL_MODE(cs);

	/* set clock speed */
	ctrl |= cspi_2_3_clkdiv(clk_get_rate(imx->clk), spi->max_speed_hz);

	/* set chip select to use */
	ctrl |= CSPI_2_3_CTRL_CS(cs);

	ctrl |= (spi->bits_per_word - 1) << CSPI_2_3_CTRL_BL_OFFSET;

	ctrl |= CSPI_2_3_CTRL_SMC;

	cfg |= CSPI_2_3_CONFIG_SBBCTRL(cs);

	if (spi->mode & SPI_CPHA)
		cfg |= CSPI_2_3_CONFIG_SCLKPHA(cs);

	if (spi->mode & SPI_CPOL)
		cfg |= CSPI_2_3_CONFIG_SCLKPOL(cs);

	if (spi->mode & SPI_CS_HIGH)
		cfg |= CSPI_2_3_CONFIG_SSBPOL(cs);

	writel(ctrl, base + CSPI_2_3_CTRL);
	writel(cfg, base + CSPI_2_3_CONFIG);

	if (gpio >= 0)
		gpio_set_value(gpio, gpio_cs);
}

static u32 imx_xchg_single(struct spi_device *spi, u32 tx_val)
{
	u32 rx_val;
	struct imx_spi *imx = container_of(spi->master, struct imx_spi, master);


	tx_val = imx_spi_maybe_reverse_bits(spi, tx_val);
	rx_val = imx->xchg_single(imx, tx_val);

	return imx_spi_maybe_reverse_bits(spi, rx_val);
}

static void imx_spi_do_transfer(struct spi_device *spi)
{
	struct imx_spi *imx = container_of(spi->master, struct imx_spi, master);
	unsigned i;

	if (imx->bits_per_word <= 8) {
		const u8	*tx_buf = imx->tx_buf;
		u8		*rx_buf = imx->rx_buf;
		u8		rx_val;

		for (i = 0; i < imx->xfer_len; i++) {
			rx_val = imx_xchg_single(spi, tx_buf ? tx_buf[i] : 0);

			if (rx_buf)
				rx_buf[i] = rx_val;
		}
	} else if (imx->bits_per_word <= 16) {
		const u16	*tx_buf = imx->tx_buf;
		u16		*rx_buf = imx->rx_buf;
		u16		rx_val;

		for (i = 0; i < imx->xfer_len >> 1; i++) {
			rx_val = imx_xchg_single(spi, tx_buf ? tx_buf[i] : 0);

			if (rx_buf)
				rx_buf[i] = rx_val;
		}
	} else if (imx->bits_per_word <= 32) {
		const u32	*tx_buf = imx->tx_buf;
		u32		*rx_buf = imx->rx_buf;
		u32		rx_val;

		for (i = 0; i < imx->xfer_len >> 2; i++) {
			rx_val = imx_xchg_single(spi, tx_buf ? tx_buf[i] : 0);

			if (rx_buf)
				rx_buf[i] = rx_val;
		}
	}
}

static int cspi_2_3_xchg_burst(struct spi_device *spi)
{
	struct imx_spi *imx = container_of(spi->master, struct imx_spi, master);
	int now, txlen, rxlen;
	u32 ctrl;
	void __iomem *base = imx->regs;

	now = min(imx->xfer_len, 512);
	now >>= 2;

	if (!now)
		return 0;

	txlen = rxlen = now;

	ctrl = readl(base + CSPI_2_3_CTRL);
	ctrl &= ~(0xfff << CSPI_2_3_CTRL_BL_OFFSET);
	ctrl |= ((txlen * 32) - 1) << CSPI_2_3_CTRL_BL_OFFSET;
	ctrl |= 1 << 3;
	writel(ctrl, base + CSPI_2_3_CTRL);

	while (txlen || rxlen) {
		u32 status = readl(base + CSPI_2_3_STAT);

		if (txlen && !(status & CSPI_2_3_STAT_TF)) {
			if (imx->tx_buf) {
				u32 data = swab32(*(u32 *)imx->tx_buf);
				writel(data, base + CSPI_2_3_TXDATA);
				imx->tx_buf += sizeof(u32);
			} else {
				writel(0, base + CSPI_2_3_TXDATA);
			}
			txlen--;
		}

		if (rxlen && (status & CSPI_2_3_STAT_RR)) {
			u32 data = readl(base + CSPI_2_3_RXDATA);

			if (imx->rx_buf) {
				*(u32 *)imx->rx_buf = swab32(data);
				imx->rx_buf += sizeof(u32);
			}

			rxlen--;
		}
	}

	imx->xfer_len -= now * 4;

	return now;
}

static void cspi_2_3_do_transfer(struct spi_device *spi)
{
	struct imx_spi *imx = container_of(spi->master, struct imx_spi, master);
	u32 ctrl;

	if (imx->bits_per_word == 8 && !(spi->mode & SPI_LSB_FIRST))
		while (cspi_2_3_xchg_burst(spi) > 0);

	if (!imx->xfer_len)
		return;

	ctrl = readl(imx->regs + CSPI_2_3_CTRL);
	ctrl &= ~(0xfff << CSPI_2_3_CTRL_BL_OFFSET);
	ctrl |= (spi->bits_per_word - 1) << CSPI_2_3_CTRL_BL_OFFSET;
	writel(ctrl, imx->regs + CSPI_2_3_CTRL);

	imx_spi_do_transfer(spi);
}

static int imx_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct imx_spi *imx = container_of(spi->master, struct imx_spi, master);
	struct spi_transfer *t;
	unsigned int cs_change;
	const int nsecs = 50;

	imx->chipselect(spi, 1);

	cs_change = 0;

	mesg->actual_length = 0;

	list_for_each_entry(t, &mesg->transfers, transfer_list) {
		if (cs_change) {
			ndelay(nsecs);
			imx->chipselect(spi, 0);
			ndelay(nsecs);
			imx->chipselect(spi, 1);
		}

		cs_change = t->cs_change;

		imx->tx_buf = t->tx_buf;
		imx->rx_buf = t->rx_buf;
		imx->xfer_len = t->len;
		imx->bits_per_word = spi->bits_per_word;
		imx->do_transfer(spi);

		mesg->actual_length += t->len;

		if (cs_change)
			imx->chipselect(spi, 1);
	}

	if (!cs_change)
		imx->chipselect(spi, 0);

	return 0;
}

static __maybe_unused struct spi_imx_devtype_data spi_imx_devtype_data_0_0 = {
	.chipselect = cspi_0_0_chipselect,
	.xchg_single = cspi_0_0_xchg_single,
	.do_transfer = imx_spi_do_transfer,
	.init = cspi_0_0_init,
};

static __maybe_unused struct spi_imx_devtype_data spi_imx_devtype_data_0_7 = {
	.chipselect = cspi_0_7_chipselect,
	.xchg_single = cspi_0_7_xchg_single,
	.do_transfer = imx_spi_do_transfer,
	.init = cspi_0_7_init,
};

static __maybe_unused struct spi_imx_devtype_data spi_imx_devtype_data_2_3 = {
	.chipselect = cspi_2_3_chipselect,
	.do_transfer = cspi_2_3_do_transfer,
	.xchg_single = cspi_2_3_xchg_single,
};

static int imx_spi_dt_probe(struct imx_spi *imx)
{
	struct device_node *node = imx->master.dev->device_node;
	int i;

	if (!node)
		return -ENODEV;

	imx->master.num_chipselect = of_gpio_named_count(node, "cs-gpios");
	imx->cs_array = xzalloc(sizeof(u32) * imx->master.num_chipselect);

	for (i = 0; i < imx->master.num_chipselect; i++)
		imx->cs_array[i] = of_get_named_gpio(node, "cs-gpios", i);

	return 0;
}

static int imx_spi_probe(struct device_d *dev)
{
	struct resource *iores;
	struct spi_master *master;
	struct imx_spi *imx;
	struct spi_imx_master *pdata = dev->platform_data;
	struct spi_imx_devtype_data *devdata = NULL;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&devdata);
	if (ret)
		return -ENODEV;

	imx = xzalloc(sizeof(*imx));

	master = &imx->master;
	master->dev = dev;
	master->bus_num = dev->id;

	master->setup = imx_spi_setup;
	master->transfer = imx_spi_transfer;

	if (pdata) {
		master->num_chipselect = pdata->num_chipselect;
		imx->cs_array = pdata->chipselect;
	} else {
		if (IS_ENABLED(CONFIG_OFDEVICE))
			imx_spi_dt_probe(imx);
	}

	imx->clk = clk_get(dev, NULL);
	if (IS_ERR(imx->clk)) {
		ret = PTR_ERR(imx->clk);
		goto err_free;
	}

	imx->chipselect = devdata->chipselect;
	imx->xchg_single = devdata->xchg_single;
	imx->do_transfer = devdata->do_transfer;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	imx->regs = IOMEM(iores->start);

	if (devdata->init)
		devdata->init(imx);

	spi_register_master(master);

	return 0;

err_free:
	free(imx);

	return ret;
}

static __maybe_unused struct of_device_id imx_spi_dt_ids[] = {
#if IS_ENABLED(CONFIG_DRIVER_SPI_IMX_0_0)
	{
		.compatible = "fsl,imx27-cspi",
		.data = &spi_imx_devtype_data_0_0,
	},
#endif
#if IS_ENABLED(CONFIG_DRIVER_SPI_IMX_0_7)
	{
		.compatible = "fsl,imx35-cspi",
		.data = &spi_imx_devtype_data_0_7,
	},
#endif
#if IS_ENABLED(CONFIG_DRIVER_SPI_IMX_2_3)
	{
		.compatible = "fsl,imx51-ecspi",
		.data = &spi_imx_devtype_data_2_3,
	},
#endif
	{
		/* sentinel */
	}
};

static struct platform_device_id imx_spi_ids[] = {
#if IS_ENABLED(CONFIG_DRIVER_SPI_IMX_0_0)
	{
		.name = "imx27-spi",
		.driver_data = (unsigned long)&spi_imx_devtype_data_0_0,
	},
#endif
#if IS_ENABLED(CONFIG_DRIVER_SPI_IMX_0_7)
	{
		.name = "imx35-spi",
		.driver_data = (unsigned long)&spi_imx_devtype_data_0_7,
	},
#endif
#if IS_ENABLED(CONFIG_DRIVER_SPI_IMX_2_3)
	{
		.name = "imx51-spi",
		.driver_data = (unsigned long)&spi_imx_devtype_data_2_3,
	},
#endif
	{
		/* sentinel */
	}
};

static struct driver_d imx_spi_driver = {
	.name  = "imx_spi",
	.probe = imx_spi_probe,
	.of_compatible = DRV_OF_COMPAT(imx_spi_dt_ids),
	.id_table = imx_spi_ids,
};
coredevice_platform_driver(imx_spi_driver);

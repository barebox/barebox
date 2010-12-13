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
#include <mach/generic.h>

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

#define CSPI_2_3_RXDATA		0x00
#define CSPI_2_3_TXDATA		0x04
#define CSPI_2_3_CTRL		0x08
#define CSPI_2_3_CTRL_ENABLE		(1 <<  0)
#define CSPI_2_3_CTRL_XCH		(1 <<  2)
#define CSPI_2_3_CTRL_MODE(cs)	(1 << ((cs) +  4))
#define CSPI_2_3_CTRL_POSTDIV_OFFSET	8
#define CSPI_2_3_CTRL_PREDIV_OFFSET	12
#define CSPI_2_3_CTRL_CS(cs)		((cs) << 18)
#define CSPI_2_3_CTRL_BL_OFFSET	20

#define CSPI_2_3_CONFIG	0x0c
#define CSPI_2_3_CONFIG_SCLKPHA(cs)	(1 << ((cs) +  0))
#define CSPI_2_3_CONFIG_SCLKPOL(cs)	(1 << ((cs) +  4))
#define CSPI_2_3_CONFIG_SBBCTRL(cs)	(1 << ((cs) +  8))
#define CSPI_2_3_CONFIG_SSBPOL(cs)	(1 << ((cs) + 12))

#define CSPI_2_3_INT		0x10
#define CSPI_2_3_INT_TEEN		(1 <<  0)
#define CSPI_2_3_INT_RREN		(1 <<  3)

#define CSPI_2_3_STAT		0x18
#define CSPI_2_3_STAT_RR		(1 <<  3)

enum imx_spi_devtype {
#ifdef CONFIG_DRIVER_SPI_IMX1
	SPI_IMX_VER_IMX1,
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_0_0
	SPI_IMX_VER_0_0,
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_0_4
	SPI_IMX_VER_0_4,
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_0_5
	SPI_IMX_VER_0_5,
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_0_7
	SPI_IMX_VER_0_7,
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_2_3
	SPI_IMX_VER_2_3,
#endif
};

struct imx_spi {
	struct spi_master	master;
	int			*cs_array;
	void __iomem		*regs;

	unsigned int		(*xchg_single)(struct imx_spi *imx, u32 data);
	void			(*chipselect)(struct spi_device *spi, int active);
	void			(*init)(struct imx_spi *imx);
};

struct spi_imx_devtype_data {
	unsigned int		(*xchg_single)(struct imx_spi *imx, u32 data);
	void			(*chipselect)(struct spi_device *spi, int active);
	void			(*init)(struct imx_spi *imx);
};

static int imx_spi_setup(struct spi_device *spi)
{
	debug("%s mode 0x%08x bits_per_word: %d speed: %d\n",
			__FUNCTION__, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);
	return 0;
}

#ifdef CONFIG_DRIVER_SPI_IMX_0_0
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

static void cspi_0_0_init(struct imx_spi *imx)
{
	void __iomem *base = imx->regs;

	writel(CSPI_0_0_CTRL_ENABLE | CSPI_0_0_CTRL_MASTER,
		     base + CSPI_0_0_CTRL);
	writel(CSPI_0_0_PERIOD_32KHZ,
		     base + CSPI_0_0_PERIOD);
	while (readl(base + CSPI_0_0_INT) & CSPI_0_0_STAT_RR)
		readl(base + CSPI_0_0_RXDATA);
	writel(0, base + CSPI_0_0_INT);
}
#endif

#ifdef CONFIG_DRIVER_SPI_IMX_2_3
static unsigned int cspi_2_3_xchg_single(struct imx_spi *imx, unsigned int data)
{
	void __iomem *base = imx->regs;

	unsigned int cfg_reg = readl(base + CSPI_2_3_CTRL);

	writel(data, base + CSPI_2_3_TXDATA);

	cfg_reg |= CSPI_2_3_CTRL_XCH;

	writel(cfg_reg, base + CSPI_2_3_CTRL);

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
			gpio_set_value(gpio, !gpio_cs);
		return;
	}

	ctrl = CSPI_2_3_CTRL_ENABLE;

	/* set master mode */
	ctrl |= CSPI_2_3_CTRL_MODE(cs);

	/* set clock speed */
	ctrl |= cspi_2_3_clkdiv(166000000, spi->max_speed_hz);

	/* set chip select to use */
	ctrl |= CSPI_2_3_CTRL_CS(cs);

	ctrl |= (spi->bits_per_word - 1) << CSPI_2_3_CTRL_BL_OFFSET;

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

static void cspi_2_3_init(struct imx_spi *imx)
{
}
#endif

static int imx_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct spi_master *master = spi->master;
	struct imx_spi *imx = container_of(master, struct imx_spi, master);
	struct spi_transfer	*t = NULL;

	imx->chipselect(spi, 1);

	list_for_each_entry (t, &mesg->transfers, transfer_list) {
		const u32 *txbuf = t->tx_buf;
		u32 *rxbuf = t->rx_buf;
		int i = 0;

		while(i < t->len >> 2) {
			rxbuf[i] = imx->xchg_single(imx, txbuf[i]);
			i++;
		}
	}

	imx->chipselect(spi, 0);

	return 0;
}

static struct spi_imx_devtype_data spi_imx_devtype_data[] = {
#ifdef CONFIG_DRIVER_SPI_IMX_0_0
	[SPI_IMX_VER_0_0] = {
		.chipselect = cspi_0_0_chipselect,
		.xchg_single = cspi_0_0_xchg_single,
		.init = cspi_0_0_init,
	},
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_2_3
	[SPI_IMX_VER_2_3] = {
		.chipselect = cspi_2_3_chipselect,
		.xchg_single = cspi_2_3_xchg_single,
		.init = cspi_2_3_init,
	},
#endif
};

static int imx_spi_probe(struct device_d *dev)
{
	struct spi_master *master;
	struct imx_spi *imx;
	struct spi_imx_master *pdata = dev->platform_data;
	enum imx_spi_devtype version;

	imx = xzalloc(sizeof(*imx));

	master = &imx->master;
	master->dev = dev;

	master->setup = imx_spi_setup;
	master->transfer = imx_spi_transfer;
	master->num_chipselect = pdata->num_chipselect;
	imx->cs_array = pdata->chipselect;

#ifdef CONFIG_DRIVER_SPI_IMX_0_0
	if (cpu_is_mx27())
		version = SPI_IMX_VER_0_0;
#endif
#ifdef CONFIG_DRIVER_SPI_IMX_2_3
	if (cpu_is_mx51())
		version = SPI_IMX_VER_2_3;
#endif
	imx->chipselect = spi_imx_devtype_data[version].chipselect;
	imx->xchg_single = spi_imx_devtype_data[version].xchg_single;
	imx->init = spi_imx_devtype_data[version].init;
	imx->regs = (void __iomem *)dev->map_base;

	imx->init(imx);

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


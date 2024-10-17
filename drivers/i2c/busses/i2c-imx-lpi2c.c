// SPDX-License-Identifier: GPL-2.0+
/*
 * This is i.MX low power i2c controller driver.
 *
 * Copyright 2016 Freescale Semiconductor, Inc.
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <gpio.h>
#include <malloc.h>
#include <types.h>
#include <xfuncs.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <pinctrl.h>
#include <of_gpio.h>
#include <of_device.h>
#include <pbl/i2c.h>

#include <io.h>
#include <i2c/i2c.h>

#define DRIVER_NAME "imx-lpi2c"

#define LPI2C_PARAM	0x04	/* i2c RX/TX FIFO size */
#define LPI2C_MCR	0x10	/* i2c contrl register */
#define LPI2C_MSR	0x14	/* i2c status register */
#define LPI2C_MIER	0x18	/* i2c interrupt enable */
#define LPI2C_MCFGR0	0x20	/* i2c master configuration */
#define LPI2C_MCFGR1	0x24	/* i2c master configuration */
#define LPI2C_MCFGR2	0x28	/* i2c master configuration */
#define LPI2C_MCFGR3	0x2C	/* i2c master configuration */
#define LPI2C_MCCR0	0x48	/* i2c master clk configuration */
#define LPI2C_MCCR1	0x50	/* i2c master clk configuration */
#define LPI2C_MFCR	0x58	/* i2c master FIFO control */
#define LPI2C_MFSR	0x5C	/* i2c master FIFO status */
#define LPI2C_MTDR	0x60	/* i2c master TX data register */
#define LPI2C_MRDR	0x70	/* i2c master RX data register */

/* i2c command */
#define TRAN_DATA	0X00
#define RECV_DATA	0X01
#define GEN_STOP	0X02
#define RECV_DISCARD	0X03
#define GEN_START	0X04
#define START_NACK	0X05
#define START_HIGH	0X06
#define START_HIGH_NACK	0X07

#define MCR_MEN		BIT(0)
#define MCR_RST		BIT(1)
#define MCR_DOZEN	BIT(2)
#define MCR_DBGEN	BIT(3)
#define MCR_RTF		BIT(8)
#define MCR_RRF		BIT(9)
#define MSR_TDF		BIT(0)
#define MSR_RDF		BIT(1)
#define MSR_SDF		BIT(9)
#define MSR_NDF		BIT(10)
#define MSR_ALF		BIT(11)
#define MSR_MBF		BIT(24)
#define MSR_BBF		BIT(25)
#define MIER_TDIE	BIT(0)
#define MIER_RDIE	BIT(1)
#define MIER_SDIE	BIT(9)
#define MIER_NDIE	BIT(10)
#define MCFGR1_AUTOSTOP	BIT(8)
#define MCFGR1_IGNACK	BIT(9)
#define MRDR_RXEMPTY	BIT(14)

#define I2C_CLK_RATIO	2
#define CHUNK_DATA	256

#define I2C_PM_TIMEOUT		10 /* ms */

enum lpi2c_imx_mode {
	STANDARD,	/* 100+Kbps */
	FAST,		/* 400+Kbps */
	FAST_PLUS,	/* 1.0+Mbps */
	HS,		/* 3.4+Mbps */
	ULTRA_FAST,	/* 5.0+Mbps */
};

enum lpi2c_imx_pincfg {
	TWO_PIN_OD,
	TWO_PIN_OO,
	TWO_PIN_PP,
	FOUR_PIN_PP,
};

struct lpi2c_imx_struct {
	struct i2c_adapter	adapter;
	struct pbl_i2c		pbl_i2c;
	int			num_clks;
	struct clk_bulk_data	*clks;
	void __iomem		*base;
	__u8			*rx_buf;
	__u8			*tx_buf;
	unsigned int		msglen;
	unsigned int		delivered;
	unsigned int		bitrate;
	unsigned int		txfifosize;
	unsigned int		rxfifosize;
	enum lpi2c_imx_mode	mode;
	unsigned long		clk_rate;
};

static void lpi2c_imx_intctrl(struct lpi2c_imx_struct *lpi2c_imx,
			      unsigned int enable)
{
	writel(enable, lpi2c_imx->base + LPI2C_MIER);
}

static int lpi2c_imx_bus_busy(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int temp;
	unsigned int timeout = 500000;

	while (1) {
		temp = readl(lpi2c_imx->base + LPI2C_MSR);

		/* check for arbitration lost, clear if set */
		if (temp & (MSR_ALF | MSR_NDF)) {
			writel(temp, lpi2c_imx->base + LPI2C_MSR);
			return -EAGAIN;
		}

		if (temp & (MSR_BBF | MSR_MBF))
			break;

		udelay(1);
		if (!timeout--) {
			dev_dbg(&lpi2c_imx->adapter.dev, "bus not work\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void lpi2c_imx_set_mode(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int bitrate = lpi2c_imx->bitrate;
	enum lpi2c_imx_mode mode;

	if (bitrate < I2C_MAX_FAST_MODE_FREQ)
		mode = STANDARD;
	else if (bitrate < I2C_MAX_FAST_MODE_PLUS_FREQ)
		mode = FAST;
	else if (bitrate < I2C_MAX_HIGH_SPEED_MODE_FREQ)
		mode = FAST_PLUS;
	else if (bitrate < I2C_MAX_ULTRA_FAST_MODE_FREQ)
		mode = HS;
	else
		mode = ULTRA_FAST;

	lpi2c_imx->mode = mode;
}

static int lpi2c_imx_start(struct lpi2c_imx_struct *lpi2c_imx,
			   struct i2c_msg *msgs)
{
	unsigned int temp;

	temp = readl(lpi2c_imx->base + LPI2C_MCR);
	temp |= MCR_RRF | MCR_RTF;
	writel(temp, lpi2c_imx->base + LPI2C_MCR);
	writel(0x7f00, lpi2c_imx->base + LPI2C_MSR);

	temp = i2c_8bit_addr_from_msg(msgs) | (GEN_START << 8);
	writel(temp, lpi2c_imx->base + LPI2C_MTDR);

	return lpi2c_imx_bus_busy(lpi2c_imx);
}

static void lpi2c_imx_stop(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int temp;
	unsigned int timeout = 500000;

	writel(GEN_STOP << 8, lpi2c_imx->base + LPI2C_MTDR);

	do {
		temp = readl(lpi2c_imx->base + LPI2C_MSR);
		if (temp & MSR_SDF)
			break;

		udelay(1);
		if (!timeout--) {
			dev_dbg(&lpi2c_imx->adapter.dev, "stop timeout\n");
			break;
		}

	} while (1);
}

/* CLKLO = I2C_CLK_RATIO * CLKHI, SETHOLD = CLKHI, DATAVD = CLKHI/2 */
static int lpi2c_imx_config(struct lpi2c_imx_struct *lpi2c_imx)
{
	u8 prescale, filt, sethold, datavd;
	unsigned int clk_cycle, clkhi, clklo;
	enum lpi2c_imx_pincfg pincfg;
	unsigned int temp;

	lpi2c_imx_set_mode(lpi2c_imx);

	if (lpi2c_imx->mode == HS || lpi2c_imx->mode == ULTRA_FAST)
		filt = 0;
	else
		filt = 2;

	for (prescale = 0; prescale <= 7; prescale++) {
		clk_cycle = lpi2c_imx->clk_rate / ((1 << prescale) * lpi2c_imx->bitrate)
			    - 3 - (filt >> 1);
		clkhi = DIV_ROUND_UP(clk_cycle, I2C_CLK_RATIO + 1);
		clklo = clk_cycle - clkhi;
		if (clklo < 64)
			break;
	}

	if (prescale > 7)
		return -EINVAL;

	/* set MCFGR1: PINCFG, PRESCALE, IGNACK */
	if (lpi2c_imx->mode == ULTRA_FAST)
		pincfg = TWO_PIN_OO;
	else
		pincfg = TWO_PIN_OD;
	temp = prescale | pincfg << 24;

	if (lpi2c_imx->mode == ULTRA_FAST)
		temp |= MCFGR1_IGNACK;

	writel(temp, lpi2c_imx->base + LPI2C_MCFGR1);

	/* set MCFGR2: FILTSDA, FILTSCL */
	temp = (filt << 16) | (filt << 24);
	writel(temp, lpi2c_imx->base + LPI2C_MCFGR2);

	/* set MCCR: DATAVD, SETHOLD, CLKHI, CLKLO */
	sethold = clkhi;
	datavd = clkhi >> 1;
	temp = datavd << 24 | sethold << 16 | clkhi << 8 | clklo;

	if (lpi2c_imx->mode == HS)
		writel(temp, lpi2c_imx->base + LPI2C_MCCR1);
	else
		writel(temp, lpi2c_imx->base + LPI2C_MCCR0);

	return 0;
}

static int lpi2c_imx_master_enable(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int temp;
	int ret;

	temp = MCR_RST;
	writel(temp, lpi2c_imx->base + LPI2C_MCR);
	writel(0, lpi2c_imx->base + LPI2C_MCR);

	ret = lpi2c_imx_config(lpi2c_imx);
	if (ret)
		return ret;

	temp = readl(lpi2c_imx->base + LPI2C_MCR);
	temp |= MCR_MEN;
	writel(temp, lpi2c_imx->base + LPI2C_MCR);

	return 0;
}

static int lpi2c_imx_master_disable(struct lpi2c_imx_struct *lpi2c_imx)
{
	u32 temp;

	temp = readl(lpi2c_imx->base + LPI2C_MCR);
	temp &= ~MCR_MEN;
	writel(temp, lpi2c_imx->base + LPI2C_MCR);

	return 0;
}

static int lpi2c_imx_txfifo_empty(struct lpi2c_imx_struct *lpi2c_imx)
{
	u32 txcnt;
	unsigned int timeout = 500000;

	do {
		txcnt = readl(lpi2c_imx->base + LPI2C_MFSR) & 0xff;

		if (readl(lpi2c_imx->base + LPI2C_MSR) & MSR_NDF) {
			dev_dbg(&lpi2c_imx->adapter.dev, "NDF detected\n");
			return -EIO;
		}

		udelay(1);
		if (!timeout--) {
			dev_dbg(&lpi2c_imx->adapter.dev, "txfifo empty timeout\n");
			return -ETIMEDOUT;
		}

	} while (txcnt);

	return 0;
}

static void lpi2c_imx_set_tx_watermark(struct lpi2c_imx_struct *lpi2c_imx)
{
	writel(lpi2c_imx->txfifosize >> 1, lpi2c_imx->base + LPI2C_MFCR);
}

static void lpi2c_imx_set_rx_watermark(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int temp, remaining;

	remaining = lpi2c_imx->msglen - lpi2c_imx->delivered;

	if (remaining > (lpi2c_imx->rxfifosize >> 1))
		temp = lpi2c_imx->rxfifosize >> 1;
	else
		temp = 0;

	writel(temp << 16, lpi2c_imx->base + LPI2C_MFCR);
}

static int lpi2c_imx_write_txfifo(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int data, remaining = lpi2c_imx->msglen;
	unsigned int timeout = 100000;;

	do {
		u32 cnt = readl(lpi2c_imx->base + LPI2C_MFSR) & 0xff;
		if (cnt == lpi2c_imx->txfifosize) {
			udelay(1);
			if (!timeout--)
				return -EIO;
			continue;
		}

		data = lpi2c_imx->tx_buf[lpi2c_imx->delivered++];

		writel(data, lpi2c_imx->base + LPI2C_MTDR);
		remaining = lpi2c_imx->msglen - lpi2c_imx->delivered;
	} while (remaining);

	return 0;
}

static int lpi2c_imx_read_rxfifo(struct lpi2c_imx_struct *lpi2c_imx)
{
	unsigned int remaining = lpi2c_imx->msglen;
	unsigned int data;
	unsigned int timeout = 100000;;

	do {
		data = readl(lpi2c_imx->base + LPI2C_MRDR);
		if (data & MRDR_RXEMPTY) {
			udelay(1);
			if (!timeout--)
				return -EIO;
			continue;
		}

		lpi2c_imx->rx_buf[lpi2c_imx->delivered++] = data & 0xff;

		remaining = lpi2c_imx->msglen - lpi2c_imx->delivered;
	} while (remaining);

	return 0;
}

static int lpi2c_imx_write(struct lpi2c_imx_struct *lpi2c_imx,
			    struct i2c_msg *msgs)
{
	lpi2c_imx->tx_buf = msgs->buf;
	lpi2c_imx_set_tx_watermark(lpi2c_imx);

	return lpi2c_imx_write_txfifo(lpi2c_imx);
}

static int lpi2c_imx_read(struct lpi2c_imx_struct *lpi2c_imx,
			   struct i2c_msg *msgs)
{
	unsigned int temp;

	lpi2c_imx->rx_buf = msgs->buf;

	lpi2c_imx_set_rx_watermark(lpi2c_imx);
	temp = msgs->len > CHUNK_DATA ? CHUNK_DATA - 1 : msgs->len - 1;
	temp |= (RECV_DATA << 8);
	writel(temp, lpi2c_imx->base + LPI2C_MTDR);

	lpi2c_imx_intctrl(lpi2c_imx, MIER_RDIE | MIER_NDIE);

	return lpi2c_imx_read_rxfifo(lpi2c_imx);
}

static int lpi2c_imx_xfer(struct i2c_adapter *adapter,
			  struct i2c_msg *msgs, int num)
{
	struct lpi2c_imx_struct *lpi2c_imx = container_of(adapter, struct lpi2c_imx_struct, adapter);
	unsigned int temp;
	int i, result;

	result = lpi2c_imx_master_enable(lpi2c_imx);
	if (result)
		return result;

	for (i = 0; i < num; i++) {
		result = lpi2c_imx_start(lpi2c_imx, &msgs[i]);
		if (result)
			goto disable;

		/* quick smbus */
		if (num == 1 && msgs[0].len == 0)
			goto stop;

		lpi2c_imx->rx_buf = NULL;
		lpi2c_imx->tx_buf = NULL;
		lpi2c_imx->delivered = 0;
		lpi2c_imx->msglen = msgs[i].len;

		if (msgs[i].flags & I2C_M_RD)
			result = lpi2c_imx_read(lpi2c_imx, &msgs[i]);
		else
			result = lpi2c_imx_write(lpi2c_imx, &msgs[i]);

		if (result)
			goto stop;

		if (!(msgs[i].flags & I2C_M_RD)) {
			result = lpi2c_imx_txfifo_empty(lpi2c_imx);
			if (result)
				goto stop;
		}
	}

stop:
	lpi2c_imx_stop(lpi2c_imx);

	temp = readl(lpi2c_imx->base + LPI2C_MSR);
	if ((temp & MSR_NDF) && !result)
		result = -EIO;

disable:
	lpi2c_imx_master_disable(lpi2c_imx);

	dev_dbg(&lpi2c_imx->adapter.dev, "<%s> exit with: %s: %d\n", __func__,
		(result < 0) ? "error" : "success msg",
		(result < 0) ? result : num);

	return (result < 0) ? result : num;
}

#ifdef __PBL__

static int lpi2c_pbl_imx_xfer(struct pbl_i2c *lpi2c, struct i2c_msg *msgs, int num)
{
	struct lpi2c_imx_struct *lpi2c_imx = container_of(lpi2c, struct lpi2c_imx_struct, pbl_i2c);

	return lpi2c_imx_xfer(&lpi2c_imx->adapter, msgs, num);
}

struct pbl_i2c *imx93_i2c_early_init(void __iomem *regs)
{
	static struct lpi2c_imx_struct lpi2c;
	u32 temp;

	lpi2c.base = regs;

	temp = readl(lpi2c.base + LPI2C_PARAM);
	lpi2c.txfifosize = 1 << (temp & 0x0f);
	lpi2c.rxfifosize = 1 << ((temp >> 8) & 0x0f);
	lpi2c.bitrate = 100000;
	lpi2c.clk_rate = 24000000;

	lpi2c.pbl_i2c.xfer = lpi2c_pbl_imx_xfer;

	return &lpi2c.pbl_i2c;
}

#else

static const struct of_device_id lpi2c_imx_of_match[] = {
	{ .compatible = "fsl,imx7ulp-lpi2c" },
	{ },
};
MODULE_DEVICE_TABLE(of, lpi2c_imx_of_match);

static int lpi2c_imx_probe(struct device *dev)
{
	struct lpi2c_imx_struct *lpi2c_imx;
	struct resource *iores;
	unsigned int temp;
	int ret;

	lpi2c_imx = xzalloc(sizeof(*lpi2c_imx));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	lpi2c_imx->base = IOMEM(iores->start);

	lpi2c_imx->adapter.nr = -1;
	lpi2c_imx->adapter.master_xfer	= lpi2c_imx_xfer;
	lpi2c_imx->adapter.dev.parent	= dev;
	lpi2c_imx->adapter.dev.of_node	= dev->of_node;

	ret = clk_bulk_get_all(dev, &lpi2c_imx->clks);
	if (ret < 0)
		return dev_err_probe(dev, ret, "can't get I2C peripheral clock\n");
	lpi2c_imx->num_clks = ret;

	ret = of_property_read_u32(dev->of_node,
				   "clock-frequency", &lpi2c_imx->bitrate);
	if (ret)
		lpi2c_imx->bitrate = I2C_MAX_STANDARD_MODE_FREQ;

	ret = clk_bulk_enable(lpi2c_imx->num_clks, lpi2c_imx->clks);
	if (ret)
		return ret;

	lpi2c_imx->clk_rate = clk_get_rate(lpi2c_imx->clks[0].clk);

	temp = readl(lpi2c_imx->base + LPI2C_PARAM);
	lpi2c_imx->txfifosize = 1 << (temp & 0x0f);
	lpi2c_imx->rxfifosize = 1 << ((temp >> 8) & 0x0f);

	ret = i2c_add_numbered_adapter(&lpi2c_imx->adapter);
	if (ret)
		return ret;

	dev_dbg(&lpi2c_imx->adapter.dev, "LPI2C adapter registered\n");

	return 0;
}

static struct driver lpi2c_imx_driver = {
        .probe  = lpi2c_imx_probe,
        .name   = "i2c-fsl",
        .of_compatible = DRV_OF_COMPAT(lpi2c_imx_of_match),
};
coredevice_platform_driver(lpi2c_imx_driver);

#endif

MODULE_AUTHOR("Gao Pan <pandy.gao@nxp.com>");
MODULE_DESCRIPTION("I2C adapter driver for LPI2C bus");
MODULE_LICENSE("GPL");

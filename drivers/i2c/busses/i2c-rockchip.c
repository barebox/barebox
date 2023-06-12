// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2015 Google, Inc
 *
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 */

#include <common.h>
#include <i2c/i2c.h>
#include <linux/iopoll.h>
#include <errno.h>
#include <linux/err.h>
#include <driver.h>
#include <io.h>
#include <linux/clk.h>
#include <mfd/syscon.h>
#include <regmap.h>
#include <linux/sizes.h>

struct i2c_regs {
	u32 con;
	u32 clkdiv;
	u32 mrxaddr;
	u32 mrxraddr;
	u32 mtxcnt;
	u32 mrxcnt;
	u32 ien;
	u32 ipd;
	u32 fcnt;
	u32 reserved0[0x37];
	u32 txdata[8];
	u32 reserved1[0x38];
	u32 rxdata[8];
};

/* Control register */
#define I2C_CON_EN		(1 << 0)
#define I2C_CON_MOD(mod)	((mod) << 1)
#define I2C_MODE_TX		0x00
#define I2C_MODE_TRX		0x01
#define I2C_MODE_RX		0x02
#define I2C_MODE_RRX		0x03
#define I2C_CON_MASK		(3 << 1)

#define I2C_CON_START		(1 << 3)
#define I2C_CON_STOP		(1 << 4)
#define I2C_CON_LASTACK		(1 << 5)
#define I2C_CON_ACTACK		(1 << 6)

/* Clock divider register */
#define I2C_CLKDIV_VAL(divl, divh) \
	(((divl) & 0xffff) | (((divh) << 16) & 0xffff0000))

/* the slave address accessed  for master rx mode */
#define I2C_MRXADDR_SET(vld, addr)	(((vld) << 24) | (addr))

/* the slave register address accessed  for master rx mode */
#define I2C_MRXRADDR_SET(vld, raddr)	(((vld) << 24) | (raddr))

/* interrupt enable register */
#define I2C_BTFIEN		(1 << 0)
#define I2C_BRFIEN		(1 << 1)
#define I2C_MBTFIEN		(1 << 2)
#define I2C_MBRFIEN		(1 << 3)
#define I2C_STARTIEN		(1 << 4)
#define I2C_STOPIEN		(1 << 5)
#define I2C_NAKRCVIEN		(1 << 6)

/* interrupt pending register */
#define I2C_BTFIPD              (1 << 0)
#define I2C_BRFIPD              (1 << 1)
#define I2C_MBTFIPD             (1 << 2)
#define I2C_MBRFIPD             (1 << 3)
#define I2C_STARTIPD            (1 << 4)
#define I2C_STOPIPD             (1 << 5)
#define I2C_NAKRCVIPD           (1 << 6)
#define I2C_IPD_ALL_CLEAN       0x7f

/* i2c timeout */
#define I2C_TIMEOUT_US		(100 * USEC_PER_MSEC)

/* rk i2c fifo max transfer bytes */
#define RK_I2C_FIFO_SIZE	32

struct rk_i2c {
	struct i2c_adapter	adapter;
	struct clk *clk;
	struct i2c_regs *regs;
	unsigned int speed;
};

static inline struct rk_i2c *to_rk_i2c(struct i2c_adapter *adapter)
{
	return container_of(adapter, struct rk_i2c, adapter);
}

static inline void rk_i2c_get_div(int div, int *divh, int *divl)
{
	*divl = div / 2;
	*divh = DIV_ROUND_UP(div, 2);
}

/*
 * SCL Divisor = 8 * (CLKDIVL+1 + CLKDIVH+1)
 * SCL = PCLK / SCLK Divisor
 * i2c_rate = PCLK
 */
static void rk_i2c_set_clk(struct rk_i2c *i2c, uint32_t scl_rate)
{
	struct device *dev = i2c->adapter.dev.parent;
	uint32_t i2c_rate;
	int div, divl, divh;

	/* First get i2c rate from pclk */
	i2c_rate = clk_get_rate(i2c->clk);

	div = DIV_ROUND_UP(i2c_rate, scl_rate * 8) - 2;
	divh = 0;
	divl = 0;
	if (div >= 0)
		rk_i2c_get_div(div, &divh, &divl);
	writel(I2C_CLKDIV_VAL(divl, divh), &i2c->regs->clkdiv);

	dev_dbg(dev, "rk_i2c_set_clk: i2c rate = %d, scl rate = %d\n", i2c_rate,
		scl_rate);
	dev_dbg(dev, "set i2c clk div = %d, divh = %d, divl = %d\n", div, divh, divl);
	dev_dbg(dev, "set clk(I2C_CLKDIV: 0x%08x)\n", readl(&i2c->regs->clkdiv));
}

static void rk_i2c_show_regs(struct rk_i2c *i2c)
{
	struct device *dev = &i2c->adapter.dev;
	struct i2c_regs *regs = i2c->regs;
	int i;

	dev_dbg(dev, "i2c_con: 0x%08x\n", readl(&regs->con));
	dev_dbg(dev, "i2c_clkdiv: 0x%08x\n", readl(&regs->clkdiv));
	dev_dbg(dev, "i2c_mrxaddr: 0x%08x\n", readl(&regs->mrxaddr));
	dev_dbg(dev, "i2c_mrxraddR: 0x%08x\n", readl(&regs->mrxraddr));
	dev_dbg(dev, "i2c_mtxcnt: 0x%08x\n", readl(&regs->mtxcnt));
	dev_dbg(dev, "i2c_mrxcnt: 0x%08x\n", readl(&regs->mrxcnt));
	dev_dbg(dev, "i2c_ien: 0x%08x\n", readl(&regs->ien));
	dev_dbg(dev, "i2c_ipd: 0x%08x\n", readl(&regs->ipd));
	dev_dbg(dev, "i2c_fcnt: 0x%08x\n", readl(&regs->fcnt));

	for (i = 0; i < 8; i++)
		dev_dbg(dev, "i2c_txdata%d: 0x%08x\n", i, readl(&regs->txdata[i]));
	for (i = 0; i < 8; i++)
		dev_dbg(dev, "i2c_rxdata%d: 0x%08x\n", i, readl(&regs->rxdata[i]));
}

static int rk_i2c_send_start_bit(struct rk_i2c *i2c)
{
	struct device *dev = &i2c->adapter.dev;
	struct i2c_regs *regs = i2c->regs;
	u32 val;
	int err;

	dev_dbg(dev, "I2c Send Start bit.\n");
	writel(I2C_IPD_ALL_CLEAN, &regs->ipd);

	writel(I2C_CON_EN | I2C_CON_START, &regs->con);
	writel(I2C_STARTIEN, &regs->ien);

	err = readl_poll_timeout(&regs->ipd, val, val & I2C_STARTIPD, I2C_TIMEOUT_US);
	if (err) {
		dev_dbg(dev, "I2C Send Start Bit Timeout\n");
		rk_i2c_show_regs(i2c);
		return err;
	}

	writel(I2C_STARTIPD, &regs->ipd);
	return 0;
}

static int rk_i2c_send_stop_bit(struct rk_i2c *i2c)
{
	struct device *dev = &i2c->adapter.dev;
	struct i2c_regs *regs = i2c->regs;
	u32 val;
	int err;

	dev_dbg(dev, "I2c Send Stop bit.\n");
	writel(I2C_IPD_ALL_CLEAN, &regs->ipd);

	writel(I2C_CON_EN | I2C_CON_STOP, &regs->con);
	writel(I2C_CON_STOP, &regs->ien);

	err = readl_poll_timeout(&regs->ipd, val, val & I2C_STOPIPD, I2C_TIMEOUT_US);
	if (err) {
		dev_dbg(dev, "I2C Send Start Bit Timeout\n");
		rk_i2c_show_regs(i2c);
		return err;
	}

	writel(I2C_STOPIPD, &regs->ipd);
	return 0;
}

static inline void rk_i2c_disable(struct rk_i2c *i2c)
{
	writel(0, &i2c->regs->con);
}

static int rk_i2c_read(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len,
		       uchar *buf, uint b_len)
{
	struct device *dev = &i2c->adapter.dev;
	struct i2c_regs *regs = i2c->regs;
	uchar *pbuf = buf;
	uint bytes_remain_len = b_len;
	uint bytes_xferred = 0;
	uint words_xferred = 0;
	uint con = 0;
	uint rxdata;
	uint i, j;
	int err;
	u32 val;
	bool snd_chunk = false;

	dev_dbg(dev, "rk_i2c_read: chip = %d, reg = %d, r_len = %d, b_len = %d\n",
	      chip, reg, r_len, b_len);

	err = rk_i2c_send_start_bit(i2c);
	if (err)
		return err;

	writel(I2C_MRXADDR_SET(1, chip << 1 | 1), &regs->mrxaddr);
	if (r_len == 0) {
		writel(0, &regs->mrxraddr);
	} else if (r_len < 4) {
		writel(I2C_MRXRADDR_SET(r_len, reg), &regs->mrxraddr);
	} else {
		dev_dbg(dev, "I2C Read: addr len %d not supported\n", r_len);
		return -EIO;
	}

	while (bytes_remain_len) {
		if (bytes_remain_len > RK_I2C_FIFO_SIZE) {
			con = I2C_CON_EN;
			bytes_xferred = RK_I2C_FIFO_SIZE;
		} else {
			/*
			 * The hw can read up to 32 bytes at a time. If we need
			 * more than one chunk, send an ACK after the last byte.
			 */
			con = I2C_CON_EN | I2C_CON_LASTACK;
			bytes_xferred = bytes_remain_len;
		}
		words_xferred = DIV_ROUND_UP(bytes_xferred, 4);

		/*
		 * make sure we are in plain RX mode if we read a second chunk
		 */
		if (snd_chunk)
			con |= I2C_CON_MOD(I2C_MODE_RX);
		else
			con |= I2C_CON_MOD(I2C_MODE_TRX);

		writel(con, &regs->con);
		writel(bytes_xferred, &regs->mrxcnt);
		writel(I2C_MBRFIEN | I2C_NAKRCVIEN, &regs->ien);

		err = readl_poll_timeout(&regs->ipd, val, val & (I2C_NAKRCVIPD|I2C_MBRFIPD), I2C_TIMEOUT_US);
		if (err) {
			dev_dbg(dev, "I2C Read Data Timeout\n");
			goto i2c_exit;
		}

		if (val & I2C_NAKRCVIPD) {
			writel(I2C_NAKRCVIPD, &regs->ipd);
			err = -EREMOTEIO;
			goto i2c_exit;
		}

		writel(I2C_MBRFIPD, &regs->ipd);

		for (i = 0; i < words_xferred; i++) {
			rxdata = readl(&regs->rxdata[i]);
			dev_dbg(dev, "I2c Read RXDATA[%d] = 0x%x\n", i, rxdata);
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_xferred)
					break;
				*pbuf++ = (rxdata >> (j * 8)) & 0xff;
			}
		}

		bytes_remain_len -= bytes_xferred;
		snd_chunk = true;
		dev_dbg(dev, "I2C Read bytes_remain_len %d\n", bytes_remain_len);
	}

i2c_exit:
	if (err)
		rk_i2c_show_regs(i2c);
	rk_i2c_disable(i2c);

	return err;
}

static int rk_i2c_write(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len,
			uchar *buf, uint b_len)
{
	struct device *dev = &i2c->adapter.dev;
	struct i2c_regs *regs = i2c->regs;
	u32 val;
	int err;
	uchar *pbuf = buf;
	uint bytes_remain_len = b_len + r_len + 1;
	uint bytes_xferred = 0;
	uint words_xferred = 0;
	uint txdata;
	uint i, j;

	dev_dbg(dev, "rk_i2c_write: chip = %d, reg = %d, r_len = %d, b_len = %d\n",
	      chip, reg, r_len, b_len);
	err = rk_i2c_send_start_bit(i2c);
	if (err)
		return err;

	while (bytes_remain_len) {
		if (bytes_remain_len > RK_I2C_FIFO_SIZE)
			bytes_xferred = RK_I2C_FIFO_SIZE;
		else
			bytes_xferred = bytes_remain_len;
		words_xferred = DIV_ROUND_UP(bytes_xferred, 4);

		for (i = 0; i < words_xferred; i++) {
			txdata = 0;
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_xferred)
					break;

				if (i == 0 && j == 0 && pbuf == buf) {
					txdata |= (chip << 1);
				} else if (i == 0 && j <= r_len && pbuf == buf) {
					txdata |= (reg &
						(0xff << ((j - 1) * 8))) << 8;
				} else {
					txdata |= (*pbuf++)<<(j * 8);
				}
			}
			writel(txdata, &regs->txdata[i]);
			dev_dbg(dev, "I2c Write TXDATA[%d] = 0x%08x\n", i, txdata);
		}

		writel(I2C_CON_EN | I2C_CON_MOD(I2C_MODE_TX), &regs->con);
		writel(bytes_xferred, &regs->mtxcnt);
		writel(I2C_MBTFIEN | I2C_NAKRCVIEN, &regs->ien);

		err = readl_poll_timeout(&regs->ipd, val, val & (I2C_NAKRCVIPD|I2C_MBTFIPD), I2C_TIMEOUT_US);
		if (err) {
			dev_dbg(dev, "I2C Write Data Timeout\n");
			goto i2c_exit;
		}

		if (val & I2C_NAKRCVIPD) {
			writel(I2C_NAKRCVIPD, &regs->ipd);
			err = -EREMOTEIO;
			goto i2c_exit;
		}

		writel(I2C_MBTFIPD, &regs->ipd);

		bytes_remain_len -= bytes_xferred;
		dev_dbg(dev, "I2C Write bytes_remain_len %d\n", bytes_remain_len);
	}

i2c_exit:
	if (err)
		rk_i2c_show_regs(i2c);
	rk_i2c_disable(i2c);

	return err;
}

static int rockchip_i2c_xfer(struct i2c_adapter *adapter, struct i2c_msg *msgs,
			     int nmsgs)
{
	struct rk_i2c *i2c = to_rk_i2c(adapter);
	struct device *dev = &adapter->dev;
	int i, ret = 0;

	dev_dbg(dev, "i2c_xfer: %d messages\n", nmsgs);
	for (i = 0; i < nmsgs; i++) {
		struct i2c_msg *msg = &msgs[i];

		dev_dbg(dev, "i2c_xfer: chip=0x%x, len=0x%x\n", msg->addr, msg->len);
		if (msg->flags & I2C_M_RD) {
			ret = rk_i2c_read(i2c, msg->addr, 0, 0, msg->buf,
					  msg->len);
		} else {
			ret = rk_i2c_write(i2c, msg->addr, 0, 0, msg->buf,
					   msg->len);
		}
		if (ret) {
			dev_dbg(dev, "i2c_write: error sending: %pe\n",
				ERR_PTR(ret));
			ret = -EREMOTEIO;
			break;
		}
	}

	rk_i2c_send_stop_bit(i2c);
	rk_i2c_disable(i2c);

	return ret < 0 ? ret : nmsgs;
}

static int rk_i2c_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct resource *iores;
	struct rk_i2c *i2c;
	unsigned bitrate;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	i2c = kzalloc(sizeof(struct rk_i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	dev->priv = i2c;
	i2c->regs = IOMEM(iores->start);

	/* Only one clock to use for bus clock and peripheral clock */
	i2c->clk = clk_get(dev, NULL);
	if (IS_ERR(i2c->clk))
		return dev_err_probe(dev, PTR_ERR(i2c->clk), "Can't get bus clk\n");

	i2c->adapter.master_xfer = rockchip_i2c_xfer;
	i2c->adapter.nr = dev->id;
	i2c->adapter.dev.parent = dev;
	i2c->adapter.dev.of_node = np;

	/* Set up clock divider */
	bitrate = 100000;
	of_property_read_u32(np, "clock-frequency", &bitrate);

	rk_i2c_set_clk(i2c, bitrate);

	return i2c_add_numbered_adapter(&i2c->adapter);
}

static const struct of_device_id rk_i2c_match[] = {
	{ .compatible = "rockchip,rv1108-i2c" },
	{ .compatible = "rockchip,rk3228-i2c" },
	{ .compatible = "rockchip,rk3288-i2c" },
	{ .compatible = "rockchip,rk3399-i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, rk_i2c_match);

static struct driver rk_i2c_driver = {
	.name  = "rk3x-i2c",
	.of_compatible = rk_i2c_match,
	.probe   = rk_i2c_probe,
};
coredevice_platform_driver(rk_i2c_driver);

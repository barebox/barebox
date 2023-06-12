// SPDX-License-Identifier: GPL-2.0-only
/*
 * I2C bus driver for the BSC peripheral on Broadcom's bcm283x family of SoCs
 *
 * Based on documentation published by Raspberry Pi foundation and the kernel
 * driver written by Stephen Warren.
 *
 * Copyright (C) Stephen Warren
 * Copyright (C) 2022 Daniel Brát
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <i2c/i2c.h>
#include <i2c/i2c-algo-bit.h>
#include <linux/iopoll.h>
#include <linux/clk.h>
#include <init.h>
#include <of_address.h>

// BSC C (Control) register
#define BSC_C_READ		BIT(0)
#define BSC_C_CLEAR1		BIT(4)
#define BSC_C_CLEAR2		BIT(5)
#define BSC_C_ST		BIT(7)
#define BSC_C_INTD		BIT(8)
#define BSC_C_INTT		BIT(9)
#define BSC_C_INTR		BIT(10)
#define BSC_C_I2CEN		BIT(15)

// BSC S (Status) register
#define BSC_S_TA		BIT(0)
#define BSC_S_DONE		BIT(1)
#define BSC_S_TXW		BIT(2)
#define BSC_S_RXR		BIT(3)
#define BSC_S_TXD		BIT(4)
#define BSC_S_RXD		BIT(5)
#define BSC_S_TXE		BIT(6)
#define BSC_S_RXF		BIT(7)
#define BSC_S_ERR		BIT(8)
#define BSC_S_CLKT		BIT(9)

// BSC A (Address) register
#define BSC_A_MASK		0x7f

// Constants
#define BSC_CDIV_MIN		0x0002
#define BSC_CDIV_MAX		0xfffe
#define BSC_FIFO_SIZE		16U

struct __packed bcm283x_i2c_regs {
	u32 c;
	u32 s;
	u32 dlen;
	u32 a;
	u32 fifo;
	u32 div;
	u32 del;
	u32 clkt;
};

struct bcm283x_i2c {
	struct i2c_adapter adapter;
	struct clk *mclk;
	struct bcm283x_i2c_regs __iomem *regs;
	u32 bitrate;
};

static inline struct bcm283x_i2c *to_bcm283x_i2c(struct i2c_adapter *adapter)
{
	return container_of(adapter, struct bcm283x_i2c, adapter);
}

static inline int bcm283x_i2c_init(struct bcm283x_i2c *bcm_i2c)
{
	struct device *dev = bcm_i2c->adapter.dev.parent;
	u32 mclk_rate, cdiv, redl, fedl;

	/*
	 * Reset control reg, flush FIFO, clear all flags and disable
	 * clock stretching
	 */
	writel(0UL, &bcm_i2c->regs->c);
	writel(BSC_C_CLEAR1, &bcm_i2c->regs->c);
	writel(BSC_S_DONE | BSC_S_ERR | BSC_S_CLKT, &bcm_i2c->regs->s);
	writel(0UL, &bcm_i2c->regs->clkt);

	/*
	 * Set the divider based on the master clock frequency and the
	 * requested i2c bitrate
	 */
	mclk_rate = clk_get_rate(bcm_i2c->mclk);
	cdiv = DIV_ROUND_UP(mclk_rate, bcm_i2c->bitrate);
	dev_dbg(dev, "bcm283x_i2c_init: mclk_rate=%u, cdiv=%08x\n",
		mclk_rate, cdiv);
	/* Note from kernel driver:
	 *   Per the datasheet, the register is always interpreted as an even
	 *   number, by rounding down. In other words, the LSB is ignored. So,
	 *   if the LSB is set, increment the divider to avoid any issue.
	 */
	if (cdiv & 1)
		cdiv++;
	if ((cdiv < BSC_CDIV_MIN) || (cdiv > BSC_CDIV_MAX)) {
		dev_err(dev, "failed to calculate valid clock divider value\n");
		return -EINVAL;
	}
	dev_dbg(dev, "bcm283x_i2c_init: cdiv adjusted to %04x\n", cdiv);
	fedl = max(cdiv / 16, 1U);
	redl = max(cdiv / 4, 1U);
	dev_dbg(dev, "bcm283x_i2c_init: fedl=%04x, redl=%04x\n", fedl, redl);
	writel(cdiv & 0xffff, &bcm_i2c->regs->div);
	writel((fedl << 16) | redl, &bcm_i2c->regs->del);
	dev_dbg(dev, "bcm283x_i2c_init: regs->div=%08x, regs->del=%08x\n",
		readl(&bcm_i2c->regs->div), readl(&bcm_i2c->regs->del));

	return 0;
}

/*
 * Macro to calculate generous timeout for given bitrate and number of bytes
 */
#define calc_byte_timeout_us(bitrate) \
	(3 * 9 * DIV_ROUND_UP(1000000, bitrate))
#define calc_msg_timeout_us(bitrate, bytes) \
	((bytes + 1) * calc_byte_timeout_us(bitrate))

static int bcm283x_i2c_msg_xfer(struct bcm283x_i2c *bcm_i2c,
				struct i2c_msg *msg)
{
	int ret;
	u32 reg_c, reg_s, reg_dlen, timeout;
	struct device *dev = &bcm_i2c->adapter.dev;
	bool msg_read = (msg->flags & I2C_M_RD) > 0;
	bool msg_10bit = (msg->flags & I2C_M_TEN) > 0;
	u16 buf_pos = 0;
	u32 bytes_left = reg_dlen = msg->len;

	if (msg_10bit && msg_read) {
		timeout = calc_byte_timeout_us(bcm_i2c->bitrate);
		writel(1UL, &bcm_i2c->regs->dlen);
		writel(msg->addr & 0xff, &bcm_i2c->regs->fifo);
		writel(((msg->addr >> 8) | 0x78) & BSC_A_MASK, &bcm_i2c->regs->a);
		writel(BSC_C_ST | BSC_C_I2CEN, &bcm_i2c->regs->c);
		ret = readl_poll_timeout(&bcm_i2c->regs->s, reg_s,
					 reg_s & (BSC_S_TA | BSC_S_ERR), timeout);

		if (ret) {
			dev_err(dev, "timeout: 10bit read initilization\n");
			goto out;
		}
		if (reg_s & BSC_S_ERR)
			goto nack;

	} else if (msg_10bit) {
		reg_dlen++;
		writel(msg->addr & 0xff, &bcm_i2c->regs->fifo);
		writel(((msg->addr >> 8) | 0x78) & BSC_A_MASK, &bcm_i2c->regs->a);
	} else {
		writel(msg->addr & BSC_A_MASK, &bcm_i2c->regs->a);
	}

	writel(reg_dlen, &bcm_i2c->regs->dlen);
	reg_c = BSC_C_ST | BSC_C_I2CEN;
	if (msg_read)
		reg_c |= BSC_C_READ;
	writel(reg_c, &bcm_i2c->regs->c);

	if (msg_read) {
		/*
		 * Read out data from FIFO as soon as it is available
		 */
		timeout = calc_byte_timeout_us(bcm_i2c->bitrate);
		for (; bytes_left; bytes_left--) {
			ret = readl_poll_timeout(&bcm_i2c->regs->s, reg_s,
						 reg_s & (BSC_S_RXD | BSC_S_ERR),
						 timeout);

			if (ret) {
				dev_err(dev, "timeout: waiting for data in FIFO\n");
				goto out;
			}
			if (reg_s & BSC_S_ERR)
				goto nack;

			msg->buf[buf_pos++] = (u8) readl(&bcm_i2c->regs->fifo);
		}
	} else {
		timeout = calc_byte_timeout_us(bcm_i2c->bitrate);
		/*
		 * Feed data to FIFO as soon as there is space for them
		 */
		for (; bytes_left; bytes_left--) {
			ret = readl_poll_timeout(&bcm_i2c->regs->s, reg_s,
						 reg_s & (BSC_S_TXD | BSC_S_ERR),
						 timeout);

			if (ret) {
				dev_err(dev, "timeout: waiting for space in FIFO\n");
				goto out;
			}
			if (reg_s & BSC_S_ERR)
				goto nack;

			writel(msg->buf[buf_pos++], &bcm_i2c->regs->fifo);
		}
	}

	/*
	 * Wait for the current transfer to finish and then flush FIFO
	 * and clear any flags so that we are ready for next msg
	 */
	timeout = calc_msg_timeout_us(bcm_i2c->bitrate, reg_dlen);
	ret = readl_poll_timeout(&bcm_i2c->regs->s, reg_s,
				 reg_s & (BSC_S_DONE | BSC_S_ERR), timeout);

	if (ret) {
		dev_err(dev, "timeout: waiting for transfer to end\n");
		goto out;
	}
	if (reg_s & BSC_S_ERR)
		goto nack;
	goto out;
nack:
	dev_dbg(dev, "device with addr %x didn't ACK\n", msg->addr);
	writel(BSC_S_ERR, &bcm_i2c->regs->s);
	timeout = calc_byte_timeout_us(bcm_i2c->bitrate);
	// Wait for end of transfer so BSC has time to send STOP condition
	readl_poll_timeout(&bcm_i2c->regs->s, reg_s, ~reg_s & BSC_S_TA, timeout);
	ret = -EREMOTEIO;
out:
	// Return to default state for next xfer
	writel(BSC_S_DONE | BSC_S_ERR | BSC_S_CLKT, &bcm_i2c->regs->s);
	writel(BSC_C_CLEAR1 | BSC_C_I2CEN, &bcm_i2c->regs->c);
	return ret;
}

static int bcm283x_i2c_xfer(struct i2c_adapter *adapter,
			    struct i2c_msg *msgs, int count)
{
	int ret, i;
	struct i2c_msg *msg;
	struct bcm283x_i2c *bcm_i2c = to_bcm283x_i2c(adapter);

	/*
	 * Reset control reg, flush FIFO, clear flags and enable the BSC
	 */
	writel(0UL, &bcm_i2c->regs->c);
	writel(BSC_C_CLEAR1, &bcm_i2c->regs->c);
	writel(BSC_S_DONE | BSC_S_ERR | BSC_S_CLKT, &bcm_i2c->regs->s);
	writel(BSC_C_I2CEN, &bcm_i2c->regs->c);

	for (i = 0; i < count; i++) {
		msg = &msgs[i];
		ret = bcm283x_i2c_msg_xfer(bcm_i2c, msg);
		if (ret)
			goto out;
	}

	writel(0UL, &bcm_i2c->regs->c);
	return count;
out:
	writel(0UL, &bcm_i2c->regs->c);
	writel(BSC_C_CLEAR1, &bcm_i2c->regs->c);
	writel(BSC_S_DONE | BSC_S_ERR | BSC_S_CLKT, &bcm_i2c->regs->s);
	return ret;
}

static int bcm283x_i2c_probe(struct device *dev)
{
	int ret;
	struct resource *iores;
	struct bcm283x_i2c *bcm_i2c;
	struct device_node *np = dev->of_node;

	bcm_i2c = xzalloc(sizeof(*bcm_i2c));

	if (!np) {
		ret = -ENXIO;
		goto err;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get iomem region\n");
		ret = PTR_ERR(iores);
		goto err;
	}
	bcm_i2c->regs = IOMEM(iores->start);

	bcm_i2c->mclk = clk_get(dev, NULL);
	if (IS_ERR(bcm_i2c->mclk)) {
		dev_err(dev, "could not acquire clock\n");
		ret = PTR_ERR(bcm_i2c->mclk);
		goto err;
	}
	clk_enable(bcm_i2c->mclk);

	bcm_i2c->bitrate = I2C_MAX_STANDARD_MODE_FREQ;
	of_property_read_u32(np, "clock-frequency", &bcm_i2c->bitrate);
	if (bcm_i2c->bitrate > I2C_MAX_FAST_MODE_FREQ) {
		dev_err(dev, "clock frequency of %u is not supported\n",
			bcm_i2c->bitrate);
		ret = -EINVAL;
		goto err;
	}

	bcm_i2c->adapter.master_xfer = bcm283x_i2c_xfer;
	bcm_i2c->adapter.nr = dev->id;
	bcm_i2c->adapter.dev.parent = dev;
	bcm_i2c->adapter.dev.of_node = np;

	ret = bcm283x_i2c_init(bcm_i2c);
	if (ret)
		goto err;

	return i2c_add_numbered_adapter(&bcm_i2c->adapter);
err:
	free(bcm_i2c);
	return ret;
}

static struct of_device_id bcm283x_i2c_dt_ids[] = {
	{ .compatible = "brcm,bcm2835-i2c", },
	{ .compatible = "brcm,bcm2711-i2c", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bcm283x_i2c_dt_ids);

static struct driver bcm283x_i2c_driver = {
	.name		= "i2c-bcm283x",
	.probe		= bcm283x_i2c_probe,
	.of_compatible	= DRV_OF_COMPAT(bcm283x_i2c_dt_ids),
};
device_platform_driver(bcm283x_i2c_driver);

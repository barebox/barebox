// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  i2c Support for Atmel's AT91 Two-Wire Interface (TWI)
 *
 *  Copyright (C) 2011 Weinmann Medical GmbH
 *  Author: Nikolaus Voss <n.voss@weinmann.de>
 *
 *  Evolved from original work by:
 *  Copyright (C) 2004 Rick Bronson
 *  Converted to 2.6 by Andrew Victor <andrew@sanpeople.com>
 *
 *  Borrowed heavily from original work by:
 *  Copyright (C) 2000 Philip Edelbrock <phil@stimpy.netroedge.com>
 */

#include <common.h>
#include <clock.h>
#include <i2c/i2c.h>
#include <malloc.h>
#include <of.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <driver.h>
#include <init.h>

#define DEFAULT_TWI_CLK_HZ		100000		/* max 400 Kbits/s */
#define AT91_I2C_TIMEOUT		(100 * MSECOND)	/* transfer timeout */
#define AT91_I2C_DMA_THRESHOLD	8			/* enable DMA if transfer size is bigger than this threshold */

/* AT91 TWI register definitions */
#define	AT91_TWI_CR		0x0000	/* Control Register */
#define	AT91_TWI_START		0x0001	/* Send a Start Condition */
#define	AT91_TWI_STOP		0x0002	/* Send a Stop Condition */
#define	AT91_TWI_MSEN		0x0004	/* Master Transfer Enable */
#define	AT91_TWI_SVDIS		0x0020	/* Slave Transfer Disable */
#define	AT91_TWI_QUICK		0x0040	/* SMBus quick command */
#define	AT91_TWI_SWRST		0x0080	/* Software Reset */

#define	AT91_TWI_MMR		0x0004	/* Master Mode Register */
#define	AT91_TWI_IADRSZ_1	0x0100	/* Internal Device Address Size */
#define	AT91_TWI_MREAD		0x1000	/* Master Read Direction */

#define	AT91_TWI_IADR		0x000c	/* Internal Address Register */

#define	AT91_TWI_CWGR		0x0010	/* Clock Waveform Generator Reg */
#define	AT91_TWI_CWGR_HOLD_MAX	0x1f
#define	AT91_TWI_CWGR_HOLD(x)	(((x) & AT91_TWI_CWGR_HOLD_MAX) << 24)

#define	AT91_TWI_SR		0x0020	/* Status Register */
#define	AT91_TWI_TXCOMP		0x0001	/* Transmission Complete */
#define	AT91_TWI_RXRDY		0x0002	/* Receive Holding Register Ready */
#define	AT91_TWI_TXRDY		0x0004	/* Transmit Holding Register Ready */

#define	AT91_TWI_OVRE		0x0040	/* Overrun Error */
#define	AT91_TWI_UNRE		0x0080	/* Underrun Error */
#define	AT91_TWI_NACK		0x0100	/* Not Acknowledged */

#define	AT91_TWI_IER		0x0024	/* Interrupt Enable Register */
#define	AT91_TWI_IDR		0x0028	/* Interrupt Disable Register */
#define	AT91_TWI_IMR		0x002c	/* Interrupt Mask Register */
#define	AT91_TWI_RHR		0x0030	/* Receive Holding Register */
#define	AT91_TWI_THR		0x0034	/* Transmit Holding Register */

#define AT91_TWI_FILTR		0x0044
#define AT91_TWI_FILTR_FILT	BIT(0)
#define AT91_TWI_FILTR_PADFEN	BIT(1)
#define AT91_TWI_FILTR_THRES(v)		((v) << 8)
#define AT91_TWI_FILTR_THRES_MAX	7
#define AT91_TWI_FILTR_THRES_MASK	GENMASK(10, 8)

struct at91_twi_pdata {
	unsigned clk_max_div;
	unsigned clk_offset;
	bool has_unre_flag;
	bool has_alt_cmd;
	bool has_hold_field;
	bool has_dig_filtr;
	bool has_adv_dig_filtr;
	bool has_ana_filtr;
};

struct at91_twi_dev {
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	u8 *buf;
	size_t buf_len;
	struct i2c_msg *msg;
	unsigned imr;
	unsigned transfer_status;
	struct i2c_adapter adapter;
	unsigned twi_cwgr_reg;
	struct at91_twi_pdata *pdata;
	u32 filter_width;

	bool enable_dig_filt;
	bool enable_ana_filt;
};

#define to_at91_twi_dev(a) container_of(a, struct at91_twi_dev, adapter)

static unsigned at91_twi_read(struct at91_twi_dev *dev, unsigned reg)
{
	return __raw_readl(dev->base + reg);
}

static void at91_twi_write(struct at91_twi_dev *dev, unsigned reg, unsigned val)
{
	__raw_writel(val, dev->base + reg);
}

static void at91_disable_twi_interrupts(struct at91_twi_dev *dev)
{
	at91_twi_write(dev, AT91_TWI_IDR,
		       AT91_TWI_TXCOMP | AT91_TWI_RXRDY | AT91_TWI_TXRDY);
}

static void at91_init_twi_bus(struct at91_twi_dev *dev)
{
	struct at91_twi_pdata *pdata = dev->pdata;
	u32 filtr = 0;

	at91_disable_twi_interrupts(dev);
	at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_SWRST);
	at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_MSEN);
	at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_SVDIS);
	at91_twi_write(dev, AT91_TWI_CWGR, dev->twi_cwgr_reg);

	/* enable digital filter */
	if (pdata->has_dig_filtr && dev->enable_dig_filt)
		filtr |= AT91_TWI_FILTR_FILT;

	/* enable advanced digital filter */
	if (pdata->has_adv_dig_filtr && dev->enable_dig_filt)
		filtr |= AT91_TWI_FILTR_FILT |
			 (AT91_TWI_FILTR_THRES(dev->filter_width) &
			 AT91_TWI_FILTR_THRES_MASK);

	/* enable analog filter */
	if (pdata->has_ana_filtr && dev->enable_ana_filt)
		filtr |= AT91_TWI_FILTR_PADFEN;

	if (filtr)
		at91_twi_write(dev, AT91_TWI_FILTR, filtr);
}

/*
 * Calculate symmetric clock as stated in datasheet:
 * twi_clk = F_MAIN / (2 * (cdiv * (1 << ckdiv) + offset))
 */
static void at91_calc_twi_clock(struct at91_twi_dev *dev, int twi_clk)
{
	int ckdiv, cdiv, div, hold = 0, filter_width = 0;
	struct at91_twi_pdata *pdata = dev->pdata;
	int offset = pdata->clk_offset;
	int max_ckdiv = pdata->clk_max_div;
	struct i2c_timings timings, *t = &timings;

	i2c_parse_fw_timings(dev->dev, t, true);

	div = max(0, (int)DIV_ROUND_UP(clk_get_rate(dev->clk),
				       2 * twi_clk) - offset);
	ckdiv = fls(div >> 8);
	cdiv = div >> ckdiv;

	if (ckdiv > max_ckdiv) {
		dev_warn(dev->dev, "%d exceeds ckdiv max value which is %d.\n",
			 ckdiv, max_ckdiv);
		ckdiv = max_ckdiv;
		cdiv = 255;
	}

	if (pdata->has_hold_field) {
		/*
		 * hold time = HOLD + 3 x T_peripheral_clock
		 * Use clk rate in kHz to prevent overflows when computing
		 * hold.
		 */
		hold = DIV_ROUND_UP(t->sda_hold_ns
				    * (clk_get_rate(dev->clk) / 1000), 1000000);
		hold -= 3;
		if (hold < 0)
			hold = 0;
		if (hold > AT91_TWI_CWGR_HOLD_MAX) {
			dev_warn(dev->dev,
				 "HOLD field set to its maximum value (%d instead of %d)\n",
				 AT91_TWI_CWGR_HOLD_MAX, hold);
			hold = AT91_TWI_CWGR_HOLD_MAX;
		}
	}

	if (pdata->has_adv_dig_filtr) {
		/*
		 * filter width = 0 to AT91_TWI_FILTR_THRES_MAX
		 * peripheral clocks
		 */
		filter_width = DIV_ROUND_UP(t->digital_filter_width_ns
				* (clk_get_rate(dev->clk) / 1000), 1000000);
		if (filter_width > AT91_TWI_FILTR_THRES_MAX) {
			dev_warn(dev->dev,
				"Filter threshold set to its maximum value (%d instead of %d)\n",
				AT91_TWI_FILTR_THRES_MAX, filter_width);
			filter_width = AT91_TWI_FILTR_THRES_MAX;
		}
	}

	dev->twi_cwgr_reg = (ckdiv << 16) | (cdiv << 8) | cdiv
			    | AT91_TWI_CWGR_HOLD(hold);

	dev->filter_width = filter_width;

	dev_dbg(dev->dev, "cdiv %d ckdiv %d hold %d (%d ns), filter_width %d (%d ns)\n",
		cdiv, ckdiv, hold, t->sda_hold_ns, filter_width,
		t->digital_filter_width_ns);
}

static void at91_twi_write_next_byte(struct at91_twi_dev *dev)
{
	if (!dev->buf_len)
		return;

	at91_twi_write(dev, AT91_TWI_THR, *dev->buf);

	/* send stop when last byte has been written */
	if (--dev->buf_len == 0)
		at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_STOP);

	dev_dbg(dev->dev, "wrote 0x%x, to go %d\n", *dev->buf, dev->buf_len);

	++dev->buf;
}

static void at91_twi_read_next_byte(struct at91_twi_dev *dev)
{
	if (!dev->buf_len)
		return;

	*dev->buf = at91_twi_read(dev, AT91_TWI_RHR) & 0xff;
	--dev->buf_len;

	/* send stop if second but last byte has been read */
	if (dev->buf_len == 1)
		at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_STOP);

	dev_dbg(dev->dev, "read 0x%x, to go %d\n", *dev->buf, dev->buf_len);

	++dev->buf;
}

static int at91_twi_wait_completion(struct at91_twi_dev *dev)
{
	uint64_t start = get_time_ns();
	unsigned int status;
	unsigned int irqstatus;

	do {
		status = at91_twi_read(dev, AT91_TWI_SR);
		irqstatus = at91_twi_read(dev, AT91_TWI_IMR);

		if (!(status & irqstatus)) {
			if (is_timeout(start, AT91_I2C_TIMEOUT)) {
				dev_warn(dev->dev, "timeout waiting for bus ready\n");
				return -ETIMEDOUT;
			} else {
				continue;
			}
		}

		if (irqstatus & AT91_TWI_RXRDY)
			at91_twi_read_next_byte(dev);
		else if (irqstatus & AT91_TWI_TXRDY)
			at91_twi_write_next_byte(dev);
		else
			dev_warn(dev->dev, "neither rx and tx are ready\n");

		dev->transfer_status |= status;

	} while (!(at91_twi_read(dev, AT91_TWI_SR) & AT91_TWI_TXCOMP));

	at91_disable_twi_interrupts(dev);

	return 0;
}

static int at91_do_twi_transfer(struct at91_twi_dev *dev)
{
	int ret;
	bool has_unre_flag = dev->pdata->has_unre_flag;

	dev_dbg(dev->dev, "transfer: %s %d bytes.\n",
		(dev->msg->flags & I2C_M_RD) ? "read" : "write", dev->buf_len);

	dev->transfer_status = 0;

	if (!dev->buf_len) {
		at91_twi_write(dev, AT91_TWI_CR, AT91_TWI_QUICK);
		at91_twi_write(dev, AT91_TWI_IER, AT91_TWI_TXCOMP);
	} else if (dev->msg->flags & I2C_M_RD) {
		unsigned start_flags = AT91_TWI_START;

		if (at91_twi_read(dev, AT91_TWI_SR) & AT91_TWI_RXRDY) {
			dev_err(dev->dev, "RXRDY still set!");
			at91_twi_read(dev, AT91_TWI_RHR);
		}

		/* if only one byte is to be read, immediately stop transfer */
		if (dev->buf_len <= 1)
			start_flags |= AT91_TWI_STOP;

		at91_twi_write(dev, AT91_TWI_CR, start_flags);

		at91_twi_write(dev, AT91_TWI_IER,
			    AT91_TWI_TXCOMP | AT91_TWI_RXRDY);
	} else {
		at91_twi_write_next_byte(dev);
		at91_twi_write(dev, AT91_TWI_IER,
			    AT91_TWI_TXCOMP | AT91_TWI_TXRDY);
	}

	ret = at91_twi_wait_completion(dev);
	if (ret < 0) {
		dev_err(dev->dev, "controller timed out\n");
		at91_init_twi_bus(dev);
		ret = -ETIMEDOUT;
		goto error;
	}
	if (dev->transfer_status & AT91_TWI_NACK) {
		dev_dbg(dev->dev, "received nack\n");
		ret = -EREMOTEIO;
		goto error;
	}
	if (dev->transfer_status & AT91_TWI_OVRE) {
		dev_err(dev->dev, "overrun while reading\n");
		ret = -EIO;
		goto error;
	}
	if (has_unre_flag && dev->transfer_status & AT91_TWI_UNRE) {
		dev_err(dev->dev, "underrun while writing\n");
		ret = -EIO;
		goto error;
	}
	dev_dbg(dev->dev, "transfer complete\n");

	return 0;

error:
	return ret;
}

static int at91_twi_xfer(struct i2c_adapter *adap, struct i2c_msg *msg, int num)
{
	struct at91_twi_dev *dev = to_at91_twi_dev(adap);
	int ret;
	unsigned int_addr_flag = 0;
	struct i2c_msg *m_start = msg;

	dev_dbg(&adap->dev, "at91_xfer: processing %d messages:\n", num);

	/*
	 * The hardware can handle at most two messages concatenated by a
	 * repeated start via it's internal address feature.
	 */
	if (num > 2) {
		dev_err(dev->dev,
			"cannot handle more than two concatenated messages.\n");
		return 0;
	} else if (num == 2) {
		int internal_address = 0;
		int i;

		if (msg->flags & I2C_M_RD) {
			dev_err(dev->dev, "first transfer must be write.\n");
			return -EINVAL;
		}
		if (msg->len > 3) {
			dev_err(dev->dev, "first message size must be <= 3.\n");
			return -EINVAL;
		}

		/* 1st msg is put into the internal address, start with 2nd */
		m_start = &msg[1];
		for (i = 0; i < msg->len; ++i) {
			const unsigned addr = msg->buf[msg->len - 1 - i];

			internal_address |= addr << (8 * i);
			int_addr_flag += AT91_TWI_IADRSZ_1;
		}
		at91_twi_write(dev, AT91_TWI_IADR, internal_address);
	}

	at91_twi_write(dev, AT91_TWI_MMR, (m_start->addr << 16) | int_addr_flag
		       | ((m_start->flags & I2C_M_RD) ? AT91_TWI_MREAD : 0));

	dev->buf_len = m_start->len;
	dev->buf = m_start->buf;
	dev->msg = m_start;

	ret = at91_do_twi_transfer(dev);

	return (ret < 0) ? ret : num;
}

static struct at91_twi_pdata at91rm9200_config = {
	.clk_max_div = 5,
	.clk_offset = 3,
	.has_unre_flag = true,
};

static struct at91_twi_pdata at91sam9261_config = {
	.clk_max_div = 5,
	.clk_offset = 4,
	.has_unre_flag = false,
};

static struct at91_twi_pdata at91sam9260_config = {
	.clk_max_div = 7,
	.clk_offset = 4,
	.has_unre_flag = false,
};

static struct at91_twi_pdata at91sam9g20_config = {
	.clk_max_div = 7,
	.clk_offset = 4,
	.has_unre_flag = false,
};

static struct at91_twi_pdata at91sam9g10_config = {
	.clk_max_div = 7,
	.clk_offset = 4,
	.has_unre_flag = false,
};

static struct at91_twi_pdata at91sam9x5_config = {
	.clk_max_div = 7,
	.clk_offset = 4,
	.has_unre_flag = false,
};

static struct at91_twi_pdata sama5d4_config = {
	.clk_max_div = 7,
	.clk_offset = 4,
	.has_hold_field = true,
	.has_dig_filtr = true,
};

static struct at91_twi_pdata sama5d2_config = {
	.clk_max_div = 7,
	.clk_offset = 3,
	.has_unre_flag = true,
	.has_alt_cmd = true,
	.has_hold_field = true,
	.has_dig_filtr = true,
	.has_adv_dig_filtr = true,
	.has_ana_filtr = true,
};

static struct platform_device_id at91_twi_devtypes[] = {
	{
		.name = "at91rm9200-i2c",
		.driver_data = (unsigned long) &at91rm9200_config,
	}, {
		.name = "at91sam9261-i2c",
		.driver_data = (unsigned long) &at91sam9261_config,
	}, {
		.name = "at91sam9260-i2c",
		.driver_data = (unsigned long) &at91sam9260_config,
	}, {
		.name = "at91sam9g20-i2c",
		.driver_data = (unsigned long) &at91sam9g20_config,
	}, {
		.name = "at91sam9g10-i2c",
		.driver_data = (unsigned long) &at91sam9g10_config,
	}, {
		.name = "at91sam9x5-i2c",
		.driver_data = (unsigned long) &at91sam9x5_config,
	}, {
		/* sentinel */
	}
};

static struct of_device_id at91_twi_dt_ids[] = {
	{
		.compatible = "atmel,at91rm9200-i2c",
		.data = &at91rm9200_config,
	} , {
		.compatible = "atmel,at91sam9260-i2c",
		.data = &at91sam9260_config,
	} , {
		.compatible = "atmel,at91sam9261-i2c",
		.data = &at91sam9261_config,
	} , {
		.compatible = "atmel,at91sam9g20-i2c",
		.data = &at91sam9g20_config,
	} , {
		.compatible = "atmel,at91sam9g10-i2c",
		.data = &at91sam9g10_config,
	}, {
		.compatible = "atmel,at91sam9x5-i2c",
		.data = &at91sam9x5_config,
	}, {
		.compatible = "atmel,sama5d4-i2c",
		.data = &sama5d4_config,
	}, {
		.compatible = "atmel,sama5d2-i2c",
		.data = &sama5d2_config,
	}, {
		.compatible = "microchip,sam9x60-i2c",
		.data = &sama5d2_config,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, at91_twi_dt_ids);

static int at91_twi_probe(struct device *dev)
{
	struct resource *iores;
	struct at91_twi_dev *i2c_at91;
	struct at91_twi_pdata *i2c_data;
	int rc = 0;
	u32 bus_clk_rate;

	i2c_at91 = xzalloc(sizeof(struct at91_twi_dev));
	i2c_at91->dev = dev;

	rc = dev_get_drvdata(dev, (const void **)&i2c_data);
	if (rc < 0) {
		dev_err(dev, "failed to retrieve driver data\n");
		goto out_free;
	}

	i2c_at91->pdata = i2c_data;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	i2c_at91->base = IOMEM(iores->start);
	if (IS_ERR(i2c_at91->base)) {
		dev_err(dev, "could not get memory region\n");
		rc = PTR_ERR(i2c_at91->base);
		goto out_free;
	}

	i2c_at91->clk = clk_get(dev, NULL);
	if (IS_ERR(i2c_at91->clk)) {
		dev_err(dev, "no clock defined\n");
		rc = -ENODEV;
		goto out_free;
	}

	clk_enable(i2c_at91->clk);

	bus_clk_rate = DEFAULT_TWI_CLK_HZ;

	at91_calc_twi_clock(i2c_at91, bus_clk_rate);
	at91_init_twi_bus(i2c_at91);

	i2c_at91->adapter.master_xfer = at91_twi_xfer;
	i2c_at91->adapter.dev.parent = dev;
	i2c_at91->adapter.nr = dev->id;
	i2c_at91->adapter.dev.of_node = dev->of_node;

	rc = i2c_add_numbered_adapter(&i2c_at91->adapter);
	if (rc) {
		dev_err(dev, "Failed to add I2C adapter\n");
		goto out_adap_fail;
	}

	dev_info(dev, "AT91 i2c bus driver.\n");
	return 0;

out_adap_fail:
	clk_disable(i2c_at91->clk);
	clk_put(i2c_at91->clk);
out_free:
	kfree(i2c_at91);
	return rc;
}

static struct driver at91_twi_driver = {
	.name		= "at91-twi",
	.probe		= at91_twi_probe,
	.id_table	= at91_twi_devtypes,
	.of_compatible	= DRV_OF_COMPAT(at91_twi_dt_ids),
};
device_platform_driver(at91_twi_driver);

MODULE_AUTHOR("Nikolaus Voss <n.voss@weinmann.de>");
MODULE_DESCRIPTION("I2C (TWI) driver for Atmel AT91");
MODULE_LICENSE("GPL");

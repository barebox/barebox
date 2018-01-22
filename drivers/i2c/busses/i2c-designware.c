/*
 * Synopsys DesignWare I2C adapter driver (master only).
 *
 * Partly based on code of similar driver from U-Boot:
 *    Copyright (C) 2009 ST Micoelectronics
 *
 * and corresponding code from Linux Kernel
 *    Copyright (C) 2006 Texas Instruments.
 *    Copyright (C) 2007 MontaVista Software Inc.
 *    Copyright (C) 2009 Provigent Ltd.
 *
 * Copyright (C) 2015 Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <malloc.h>
#include <types.h>
#include <xfuncs.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/math64.h>

#include <io.h>
#include <i2c/i2c.h>

#define DW_I2C_BIT_RATE			100000

#define DW_IC_CON			0x0
#define DW_IC_CON_MASTER		(1 << 0)
#define DW_IC_CON_SPEED_STD		(1 << 1)
#define DW_IC_CON_SPEED_FAST		(1 << 2)
#define DW_IC_CON_SLAVE_DISABLE		(1 << 6)

#define DW_IC_TAR			0x4

#define DW_IC_DATA_CMD			0x10
#define DW_IC_DATA_CMD_CMD		(1 << 8)
#define DW_IC_DATA_CMD_STOP		(1 << 9)

#define DW_IC_SS_SCL_HCNT		0x14
#define DW_IC_SS_SCL_LCNT		0x18
#define DW_IC_FS_SCL_HCNT		0x1c
#define DW_IC_FS_SCL_LCNT		0x20

#define DW_IC_INTR_MASK			0x30

#define DW_IC_RAW_INTR_STAT		0x34
#define DW_IC_INTR_RX_UNDER		(1 << 0)
#define DW_IC_INTR_RX_OVER		(1 << 1)
#define DW_IC_INTR_RX_FULL		(1 << 2)
#define DW_IC_INTR_TX_OVER		(1 << 3)
#define DW_IC_INTR_TX_EMPTY		(1 << 4)
#define DW_IC_INTR_RD_REQ		(1 << 5)
#define DW_IC_INTR_TX_ABRT		(1 << 6)
#define DW_IC_INTR_RX_DONE		(1 << 7)
#define DW_IC_INTR_ACTIVITY		(1 << 8)
#define DW_IC_INTR_STOP_DET		(1 << 9)
#define DW_IC_INTR_START_DET		(1 << 10)
#define DW_IC_INTR_GEN_CALL		(1 << 11)

#define DW_IC_RX_TL			0x38
#define DW_IC_TX_TL			0x3c
#define DW_IC_CLR_INTR			0x40
#define DW_IC_CLR_TX_ABRT		0x54
#define DW_IC_SDA_HOLD			0x7c

#define DW_IC_ENABLE			0x6c
#define DW_IC_ENABLE_ENABLE		(1 << 0)

#define DW_IC_STATUS			0x70
#define DW_IC_STATUS_TFNF		(1 << 1)
#define DW_IC_STATUS_TFE		(1 << 2)
#define DW_IC_STATUS_RFNE		(1 << 3)
#define DW_IC_STATUS_MST_ACTIVITY	(1 << 5)

#define DW_IC_TX_ABRT_SOURCE		0x80

#define DW_IC_ENABLE_STATUS		0x9c
#define DW_IC_ENABLE_STATUS_IC_EN	(1 << 0)

#define DW_IC_COMP_VERSION		0xf8
#define DW_IC_SDA_HOLD_MIN_VERS		0x3131312A
#define DW_IC_COMP_TYPE			0xfc
#define DW_IC_COMP_TYPE_VALUE		0x44570140

#define MAX_T_POLL_COUNT		100

#define DW_TIMEOUT_IDLE			(40 * MSECOND)
#define DW_TIMEOUT_TX			(2 * MSECOND)
#define DW_TIMEOUT_RX			(2 * MSECOND)

#define DW_IC_SDA_HOLD_RX_SHIFT		16
#define DW_IC_SDA_HOLD_RX_MASK		GENMASK(23, DW_IC_SDA_HOLD_RX_SHIFT)


struct dw_i2c_dev {
	void __iomem *base;
	struct clk *clk;
	struct i2c_adapter adapter;
	u32 sda_hold_time;
};

static inline struct dw_i2c_dev *to_dw_i2c_dev(struct i2c_adapter *a)
{
	return container_of(a, struct dw_i2c_dev, adapter);
}

static void i2c_dw_enable(struct dw_i2c_dev *dw, bool enable)
{
	/*
	 * This subrotine is an implementation of an algorithm
	 * described in "Cyclone V Hard Processor System Technical
	 * Reference * Manual" p. 20-19, "Disabling the I2C Controller"
	 */
	int timeout = MAX_T_POLL_COUNT;

	enable = enable ? DW_IC_ENABLE_ENABLE : 0;

	do {
		uint32_t ic_enable_status;

		writel(enable, dw->base + DW_IC_ENABLE);

		ic_enable_status = readl(dw->base + DW_IC_ENABLE_STATUS);
		if ((ic_enable_status & DW_IC_ENABLE_STATUS_IC_EN) == enable)
			return;

		udelay(250);
	} while (timeout--);

	dev_warn(&dw->adapter.dev, "timeout in %sabling adapter\n",
		 enable ? "en" : "dis");
}

/*
 * All of the code pertaining to tming calculation is taken from
 * analogous driver in Linux kernel
 */
static uint32_t
i2c_dw_scl_hcnt(uint32_t ic_clk, uint32_t tSYMBOL, uint32_t tf, int cond,
		int offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return (ic_clk * tSYMBOL + 500000) / 1000000 - 8 + offset;
	else
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3).  With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_dw_scl_lcnt().
		 */
		return (ic_clk * (tSYMBOL + tf) + 500000) / 1000000
			- 3 + offset;
}

static uint32_t
i2c_dw_scl_lcnt(uint32_t ic_clk, uint32_t tLOW, uint32_t tf, int offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return ((ic_clk * (tLOW + tf) + 500000) / 1000000) - 1 + offset;
}

static void i2c_dw_setup_timings(struct dw_i2c_dev *dw)
{
	uint32_t hcnt, lcnt;
	u32 reg;

	const uint32_t sda_falling_time = 300; /* ns */
	const uint32_t scl_falling_time = 300; /* ns */

	const unsigned int input_clock_khz = clk_get_rate(dw->clk) / 1000;

	/* Set SCL timing parameters for standard-mode */
	hcnt = i2c_dw_scl_hcnt(input_clock_khz,
			       4000,	/* tHD;STA = tHIGH = 4.0 us */
			       sda_falling_time,
			       0,	/* 0: DW default, 1: Ideal */
			       0);	/* No offset */
	lcnt = i2c_dw_scl_lcnt(input_clock_khz,
			       4700,	/* tLOW = 4.7 us */
			       scl_falling_time,
			       0);	/* No offset */

	writel(hcnt, dw->base + DW_IC_SS_SCL_HCNT);
	writel(lcnt, dw->base + DW_IC_SS_SCL_LCNT);

	hcnt = i2c_dw_scl_hcnt(input_clock_khz,
			       600,	/* tHD;STA = tHIGH = 0.6 us */
			       sda_falling_time,
			       0,	/* 0: DW default, 1: Ideal */
			       0);	/* No offset */
	lcnt = i2c_dw_scl_lcnt(input_clock_khz,
			       1300,	/* tLOW = 1.3 us */
			       scl_falling_time,
			       0);	/* No offset */

	writel(hcnt, dw->base + DW_IC_FS_SCL_HCNT);
	writel(lcnt, dw->base + DW_IC_FS_SCL_LCNT);

	/* Configure SDA Hold Time if required */
	reg = readl(dw->base + DW_IC_COMP_VERSION);
	if (reg >= DW_IC_SDA_HOLD_MIN_VERS) {
		u32 ht;
		int ret;

		ret = of_property_read_u32(dw->adapter.dev.device_node,
					   "i2c-sda-hold-time-ns", &ht);
		if (ret) {
			/* Keep previous hold time setting if no one set it */
			dw->sda_hold_time = readl(dw->base + DW_IC_SDA_HOLD);
		} else if (ht) {
			dw->sda_hold_time = div_u64((u64)input_clock_khz * ht + 500000,
						    1000000);
		}

		/*
		 * Workaround for avoiding TX arbitration lost in case I2C
		 * slave pulls SDA down "too quickly" after falling egde of
		 * SCL by enabling non-zero SDA RX hold. Specification says it
		 * extends incoming SDA low to high transition while SCL is
		 * high but it apprears to help also above issue.
		 */
		if (!(dw->sda_hold_time & DW_IC_SDA_HOLD_RX_MASK))
			dw->sda_hold_time |= 1 << DW_IC_SDA_HOLD_RX_SHIFT;

		dev_dbg(&dw->adapter.dev, "adjust SDA hold time.\n");
		writel(dw->sda_hold_time, dw->base + DW_IC_SDA_HOLD);
	}
}

static int i2c_dw_wait_for_bits(struct dw_i2c_dev *dw, uint32_t offset,
				uint32_t mask, uint32_t value, uint64_t timeout)
{
	const uint64_t start = get_time_ns();

	do {
		const uint32_t reg = readl(dw->base + offset);

		if ((reg & mask) == value)
			return 0;

	} while (!is_timeout(start, timeout));

	return -ETIMEDOUT;
}

static int i2c_dw_wait_for_idle(struct dw_i2c_dev *dw)
{
	const uint32_t mask  = DW_IC_STATUS_MST_ACTIVITY | DW_IC_STATUS_TFE;
	const uint32_t value = DW_IC_STATUS_TFE;

	return i2c_dw_wait_for_bits(dw, DW_IC_STATUS, mask, value,
				    DW_TIMEOUT_IDLE);
}

static int i2c_dw_wait_for_tx_fifo_not_full(struct dw_i2c_dev *dw)
{
	const uint32_t mask  = DW_IC_STATUS_TFNF;
	const uint32_t value = DW_IC_STATUS_TFNF;

	return i2c_dw_wait_for_bits(dw, DW_IC_STATUS, mask, value,
				    DW_TIMEOUT_TX);
}

static int i2c_dw_wait_for_rx_fifo_not_empty(struct dw_i2c_dev *dw)
{
	const uint32_t mask  = DW_IC_STATUS_RFNE;
	const uint32_t value = DW_IC_STATUS_RFNE;

	return i2c_dw_wait_for_bits(dw, DW_IC_STATUS, mask, value,
				    DW_TIMEOUT_RX);
}

static void i2c_dw_reset(struct dw_i2c_dev *dw)
{
	i2c_dw_enable(dw, false);
	i2c_dw_enable(dw, true);
}

static void i2c_dw_abort_tx(struct dw_i2c_dev *dw)
{
	i2c_dw_reset(dw);
}

static void i2c_dw_abort_rx(struct dw_i2c_dev *dw)
{
	i2c_dw_reset(dw);
}

static int i2c_dw_read(struct dw_i2c_dev *dw,
		       const struct i2c_msg *msg)
{
	int i;
	for (i = 0; i < msg->len; i++) {
		int ret;
		const bool last_byte = i == msg->len - 1;
		uint32_t ic_cmd_data = DW_IC_DATA_CMD_CMD;

		if (last_byte)
			ic_cmd_data |= DW_IC_DATA_CMD_STOP;

		writel(ic_cmd_data, dw->base + DW_IC_DATA_CMD);

		ret = i2c_dw_wait_for_rx_fifo_not_empty(dw);
		if (ret < 0) {
			i2c_dw_abort_rx(dw);
			return ret;
		}

		msg->buf[i] = (uint8_t)readl(dw->base + DW_IC_DATA_CMD);
	}

	return msg->len;
}

static int i2c_dw_write(struct dw_i2c_dev *dw,
			const struct i2c_msg *msg)
{
	int i;
	uint32_t ic_int_stat;

	for (i = 0; i < msg->len; i++) {
		int ret;
		uint32_t ic_cmd_data;
		const bool last_byte = i == msg->len - 1;

		ic_int_stat = readl(dw->base + DW_IC_RAW_INTR_STAT);

		if (ic_int_stat & DW_IC_INTR_TX_ABRT)
			return -EIO;

		ret = i2c_dw_wait_for_tx_fifo_not_full(dw);
		if (ret < 0) {
			i2c_dw_abort_tx(dw);
			return ret;
		}

		ic_cmd_data = msg->buf[i];

		if (last_byte)
			ic_cmd_data |= DW_IC_DATA_CMD_STOP;

		writel(ic_cmd_data, dw->base + DW_IC_DATA_CMD);
	}

	return msg->len;
}

static int i2c_dw_wait_for_stop(struct dw_i2c_dev *dw)
{
	const uint32_t mask  = DW_IC_INTR_STOP_DET;
	const uint32_t value = DW_IC_INTR_STOP_DET;

	return i2c_dw_wait_for_bits(dw, DW_IC_RAW_INTR_STAT, mask, value,
				    DW_TIMEOUT_IDLE);
}

static int i2c_dw_finish_xfer(struct dw_i2c_dev *dw)
{
	int ret;
	uint32_t ic_int_stat;

	/*
	 * We expect the controller to signal STOP condition on the
	 * bus, so we are going to wait for that first.
	 */
	ret = i2c_dw_wait_for_stop(dw);
	if (ret < 0)
		return ret;

	/*
	 * Now that we now that the stop condition has been signaled
	 * we need to wait for controller to go into IDLE state to
	 * make sure all of the possible error conditions on the bus
	 * have been propagated to apporpriate status
	 * registers. Experiment shows that not doing so often results
	 * in false positive "successful" transfers
	*/
	ret = i2c_dw_wait_for_idle(dw);

	if (ret >= 0) {
		ic_int_stat = readl(dw->base + DW_IC_RAW_INTR_STAT);

		if (ic_int_stat & DW_IC_INTR_TX_ABRT)
			return -EIO;
	}

	return ret;
}

static int i2c_dw_set_address(struct dw_i2c_dev *dw, uint8_t address)
{
	int ret;
	uint32_t ic_tar;
	/*
	 * As per "Cyclone V Hard Processor System Technical Reference
	 * Manual" p. 20-19, we have to wait for controller to be in
	 * idle state in order to be able to set the address
	 * dynamically
	 */
	ret = i2c_dw_wait_for_idle(dw);
	if (ret < 0)
		return ret;

	ic_tar = readl(dw->base + DW_IC_TAR);
	ic_tar &= 0xfffffc00;

	writel(ic_tar | address, dw->base + DW_IC_TAR);

	return 0;
}

static int i2c_dw_xfer(struct i2c_adapter *adapter,
		       struct i2c_msg *msgs, int num)
{
	int i, ret = 0;
	struct dw_i2c_dev *dw = to_dw_i2c_dev(adapter);

	for (i = 0; i < num; i++) {
		if (msgs[i].flags & I2C_M_DATA_ONLY)
			return -ENOTSUPP;

		ret = i2c_dw_set_address(dw, msgs[i].addr);
		if (ret < 0)
			break;

		if (msgs[i].flags & I2C_M_RD)
			ret = i2c_dw_read(dw, &msgs[i]);
		else
			ret = i2c_dw_write(dw, &msgs[i]);

		if (ret < 0)
			break;

		ret = i2c_dw_finish_xfer(dw);
		if (ret < 0)
			break;
	}

	if (ret == -EIO) {
		/*
		 * If we got -EIO it means that transfer was for some
		 * reason aborted, so we should figure out the reason
		 * and take steps to clear that condition
		 */
		const uint32_t ic_tx_abrt_source =
			readl(dw->base + DW_IC_TX_ABRT_SOURCE);
		dev_dbg(&dw->adapter.dev,
			"<%s> ic_tx_abrt_source: 0x%04x\n",
			__func__, ic_tx_abrt_source);
		readl(dw->base + DW_IC_CLR_TX_ABRT);

		return ret;
	}

	if (ret < 0) {
		i2c_dw_reset(dw);
		return ret;
	}

	return num;
}


static int i2c_dw_probe(struct device_d *pdev)
{
	struct resource *iores;
	struct dw_i2c_dev *dw;
	struct i2c_platform_data *pdata;
	int ret, bitrate;
	uint32_t ic_con, ic_comp_type_value;

	pdata = pdev->platform_data;

	dw = xzalloc(sizeof(*dw));

	if (IS_ENABLED(CONFIG_COMMON_CLK)) {
		dw->clk = clk_get(pdev, NULL);
		if (IS_ERR(dw->clk)) {
			ret = PTR_ERR(dw->clk);
			goto fail;
		}
	}

	dw->adapter.master_xfer = i2c_dw_xfer;
	dw->adapter.nr = pdev->id;
	dw->adapter.dev.parent = pdev;
	dw->adapter.dev.device_node = pdev->device_node;

	iores = dev_request_mem_resource(pdev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto fail;
	}
	dw->base = IOMEM(iores->start);

	ic_comp_type_value = readl(dw->base + DW_IC_COMP_TYPE);
	if (ic_comp_type_value != DW_IC_COMP_TYPE_VALUE) {
		dev_err(pdev,
			"unknown DesignWare IP block 0x%08x",
			ic_comp_type_value);
		ret = -ENODEV;
		goto fail;
	}

	i2c_dw_enable(dw, false);

	if (IS_ENABLED(CONFIG_COMMON_CLK))
		i2c_dw_setup_timings(dw);

	bitrate = (pdata && pdata->bitrate) ? pdata->bitrate : DW_I2C_BIT_RATE;

	/*
	 * We have to clear 'ic_10bitaddr_master' in 'ic_tar'
	 * register, otherwise 'ic_10bitaddr_master' in 'ic_con'
	 * wouldn't clear. We don't care about preserving the contents
	 * of that register so we set it to zero.
	 */
	writel(0, dw->base + DW_IC_TAR);

	switch (bitrate) {
	case 400000:
		ic_con = DW_IC_CON_SPEED_FAST;
		break;
	default:
		dev_warn(pdev, "requested bitrate (%d) is not supported."
			 " Falling back to 100kHz", bitrate);
	case 100000:		/* FALLTHROUGH */
		ic_con = DW_IC_CON_SPEED_STD;
		break;
	}

	ic_con |= DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE;

	writel(ic_con, dw->base + DW_IC_CON);

	/*
	 * Since we will be working in polling mode set both
	 * thresholds to their minimum
	 */
	writel(0, dw->base + DW_IC_RX_TL);
	writel(0, dw->base + DW_IC_TX_TL);

	/* Disable and clear all interrrupts */
	writel(0, dw->base + DW_IC_INTR_MASK);
	readl(dw->base + DW_IC_CLR_INTR);

	i2c_dw_enable(dw, true);

	ret = i2c_add_numbered_adapter(&dw->adapter);
fail:
	if (ret < 0)
		kfree(dw);

	return ret;
}

static __maybe_unused struct of_device_id i2c_dw_dt_ids[] = {
	{ .compatible = "snps,designware-i2c", },
	{ /* sentinel */ }
};

static struct driver_d i2c_dw_driver = {
	.probe = i2c_dw_probe,
	.name = "i2c-designware",
	.of_compatible = DRV_OF_COMPAT(i2c_dw_dt_ids),
};
coredevice_platform_driver(i2c_dw_driver);

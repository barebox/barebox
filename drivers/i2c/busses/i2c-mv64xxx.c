/*
 * Driver for the i2c controller on the Marvell line of host bridges
 * (e.g, gt642[46]0, mv643[46]0, mv644[46]0, and Orion SoC family).
 *
 * This code was ported from linux-3.15 kernel by Antony Pavlov.
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * 2005 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <malloc.h>
#include <types.h>
#include <xfuncs.h>
#include <clock.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <io.h>
#include <i2c/i2c.h>

#define ADDR_ADDR(val)			((val & 0x7f) << 1)
#define BAUD_DIV_N(val)			(val & 0x7)
#define BAUD_DIV_M(val)			((val & 0xf) << 3)

#define	REG_CONTROL_ACK				0x00000004
#define	REG_CONTROL_IFLG			0x00000008
#define	REG_CONTROL_STOP			0x00000010
#define	REG_CONTROL_START			0x00000020
#define	REG_CONTROL_TWSIEN			0x00000040
#define	REG_CONTROL_INTEN			0x00000080

/* Ctlr status values */
#define	STATUS_MAST_START			0x08
#define	STATUS_MAST_REPEAT_START		0x10
#define	STATUS_MAST_WR_ADDR_ACK			0x18
#define	STATUS_MAST_WR_ADDR_NO_ACK		0x20
#define	STATUS_MAST_WR_ACK			0x28
#define	STATUS_MAST_WR_NO_ACK			0x30
#define	STATUS_MAST_RD_ADDR_ACK			0x40
#define	STATUS_MAST_RD_ADDR_NO_ACK		0x48
#define	STATUS_MAST_RD_DATA_ACK			0x50
#define	STATUS_MAST_RD_DATA_NO_ACK		0x58
#define	STATUS_MAST_WR_ADDR_2_ACK		0xd0
#define	STATUS_MAST_RD_ADDR_2_ACK		0xe0

/* Driver states */
enum mv64xxx_state {
	STATE_INVALID,
	STATE_IDLE,
	STATE_WAITING_FOR_START_COND,
	STATE_WAITING_FOR_RESTART,
	STATE_WAITING_FOR_ADDR_1_ACK,
	STATE_WAITING_FOR_ADDR_2_ACK,
	STATE_WAITING_FOR_SLAVE_ACK,
	STATE_WAITING_FOR_SLAVE_DATA,
};

/* Driver actions */
enum mv64xxx_action {
	ACTION_INVALID,
	ACTION_CONTINUE,
	ACTION_SEND_RESTART,
	ACTION_OFFLOAD_RESTART,
	ACTION_SEND_ADDR_1,
	ACTION_SEND_ADDR_2,
	ACTION_SEND_DATA,
	ACTION_RCV_DATA,
	ACTION_RCV_DATA_STOP,
	ACTION_SEND_STOP,
	ACTION_OFFLOAD_SEND_STOP,
};

struct mv64xxx_i2c_regs {
	u8	addr;
	u8	ext_addr;
	u8	data;
	u8	control;
	u8	status;
	u8	clock;
	u8	soft_reset;
};

struct mv64xxx_i2c_data {
	struct i2c_msg		*msgs;
	int			num_msgs;
	enum mv64xxx_state	state;
	enum mv64xxx_action	action;
	u8			cntl_bits;
	void __iomem		*reg_base;
	struct mv64xxx_i2c_regs	reg_offsets;
	u8			addr1;
	u8			addr2;
	u8			bytes_left;
	u8			byte_posn;
	u8			send_stop;
	bool			block;
	int			rc;
	u32			freq_m;
	u32			freq_n;
	struct clk              *clk;
	struct i2c_msg		*msg;
	struct i2c_adapter	adapter;
/* 5us delay in order to avoid repeated start timing violation */
	bool			errata_delay;
	void (*write_reg)(struct mv64xxx_i2c_data *drv_data, u32 v, unsigned reg);
	u32 (*read_reg)(struct mv64xxx_i2c_data *drv_data, unsigned reg);
};

static struct mv64xxx_i2c_regs mv64xxx_i2c_regs_mv64xxx = {
	.addr		= 0x00,
	.ext_addr	= 0x10,
	.data		= 0x04,
	.control	= 0x08,
	.status		= 0x0c,
	.clock		= 0x0c,
	.soft_reset	= 0x1c,
};

static void mv64xxx_writeb(struct mv64xxx_i2c_data *drv_data,
				u32 v, unsigned reg)
{
	writeb(v, drv_data->reg_base + reg);
}

static void mv64xxx_writel(struct mv64xxx_i2c_data *drv_data,
				u32 v, unsigned reg)
{
	writel(v, drv_data->reg_base + reg);
}

static inline void mv64xxx_write(struct mv64xxx_i2c_data *drv_data,
				u32 v, unsigned reg)
{
	drv_data->write_reg(drv_data, v, reg);
}

static u32 mv64xxx_readb(struct mv64xxx_i2c_data *drv_data, unsigned reg)
{
	return readb(drv_data->reg_base + reg);
}

static u32 mv64xxx_readl(struct mv64xxx_i2c_data *drv_data, unsigned reg)
{
	return readl(drv_data->reg_base + reg);
}

static inline u32 mv64xxx_read(struct mv64xxx_i2c_data *drv_data, unsigned reg)
{
	return drv_data->read_reg(drv_data, reg);
}

static void
mv64xxx_i2c_prepare_for_io(struct mv64xxx_i2c_data *drv_data,
	struct i2c_msg *msg)
{
	u32	dir = 0;

	drv_data->cntl_bits = REG_CONTROL_ACK |
		REG_CONTROL_INTEN | REG_CONTROL_TWSIEN;

	if (msg->flags & I2C_M_RD)
		dir = 1;

	if (msg->flags & I2C_M_TEN) {
		drv_data->addr1 = 0xf0 | (((u32)msg->addr & 0x300) >> 7) | dir;
		drv_data->addr2 = (u32)msg->addr & 0xff;
	} else {
		drv_data->addr1 = ADDR_ADDR((u32)msg->addr) | dir;
		drv_data->addr2 = 0;
	}
}

/*
 *****************************************************************************
 *
 *	Finite State Machine & Interrupt Routines
 *
 *****************************************************************************
 */

/* Reset hardware and initialize FSM */
static void
mv64xxx_i2c_hw_init(struct mv64xxx_i2c_data *drv_data)
{
	mv64xxx_write(drv_data, 0, drv_data->reg_offsets.soft_reset);
	mv64xxx_write(drv_data, BAUD_DIV_M(drv_data->freq_m)
		| BAUD_DIV_N(drv_data->freq_n),
		drv_data->reg_offsets.clock);
	mv64xxx_write(drv_data, 0, drv_data->reg_offsets.addr);
	mv64xxx_write(drv_data, 0, drv_data->reg_offsets.ext_addr);
	mv64xxx_write(drv_data, REG_CONTROL_TWSIEN
		| REG_CONTROL_STOP,
		drv_data->reg_offsets.control);
	drv_data->state = STATE_IDLE;
}

static void
mv64xxx_i2c_fsm(struct mv64xxx_i2c_data *drv_data, u32 status)
{
	/*
	 * If state is idle, then this is likely the remnants of an old
	 * operation that driver has given up on or the user has killed.
	 * If so, issue the stop condition and go to idle.
	 */
	if (drv_data->state == STATE_IDLE) {
		drv_data->action = ACTION_SEND_STOP;
		return;
	}

	/* The status from the ctlr [mostly] tells us what to do next */
	switch (status) {
	/* Start condition interrupt */
	case STATUS_MAST_START: /* 0x08 */
	case STATUS_MAST_REPEAT_START: /* 0x10 */
		drv_data->action = ACTION_SEND_ADDR_1;
		drv_data->state = STATE_WAITING_FOR_ADDR_1_ACK;
		break;

	/* Performing a write */
	case STATUS_MAST_WR_ADDR_ACK: /* 0x18 */
		if (drv_data->msg->flags & I2C_M_TEN) {
			drv_data->action = ACTION_SEND_ADDR_2;
			drv_data->state =
				STATE_WAITING_FOR_ADDR_2_ACK;
			break;
		}
		/* FALLTHRU */
	case STATUS_MAST_WR_ADDR_2_ACK: /* 0xd0 */
	case STATUS_MAST_WR_ACK: /* 0x28 */
		if (drv_data->bytes_left == 0) {
			if (drv_data->send_stop) {
				drv_data->action = ACTION_SEND_STOP;
				drv_data->state = STATE_IDLE;
			} else {
				drv_data->action =
					ACTION_SEND_RESTART;
				drv_data->state =
					STATE_WAITING_FOR_RESTART;
			}
		} else {
			drv_data->action = ACTION_SEND_DATA;
			drv_data->state =
				STATE_WAITING_FOR_SLAVE_ACK;
			drv_data->bytes_left--;
		}
		break;

	/* Performing a read */
	case STATUS_MAST_RD_ADDR_ACK: /* 40 */
		if (drv_data->msg->flags & I2C_M_TEN) {
			drv_data->action = ACTION_SEND_ADDR_2;
			drv_data->state =
				STATE_WAITING_FOR_ADDR_2_ACK;
			break;
		}
		/* FALLTHRU */
	case STATUS_MAST_RD_ADDR_2_ACK: /* 0xe0 */
		if (drv_data->bytes_left == 0) {
			drv_data->action = ACTION_SEND_STOP;
			drv_data->state = STATE_IDLE;
			break;
		}
		/* FALLTHRU */
	case STATUS_MAST_RD_DATA_ACK: /* 0x50 */
		udelay(2);
		if (status != STATUS_MAST_RD_DATA_ACK)
			drv_data->action = ACTION_CONTINUE;
		else {
			drv_data->action = ACTION_RCV_DATA;
			drv_data->bytes_left--;
		}
		drv_data->state = STATE_WAITING_FOR_SLAVE_DATA;

		if (drv_data->bytes_left == 1)
			drv_data->cntl_bits &= ~REG_CONTROL_ACK;
		udelay(2);
		break;

	case STATUS_MAST_RD_DATA_NO_ACK: /* 0x58 */
		drv_data->action = ACTION_RCV_DATA_STOP;
		drv_data->state = STATE_IDLE;
		break;

	case STATUS_MAST_WR_ADDR_NO_ACK: /* 0x20 */
	case STATUS_MAST_WR_NO_ACK: /* 30 */
	case STATUS_MAST_RD_ADDR_NO_ACK: /* 48 */
		/* Doesn't seem to be a device at other end */
		drv_data->action = ACTION_SEND_STOP;
		drv_data->state = STATE_IDLE;
		drv_data->rc = -ENXIO;
		break;

	default:
		dev_err(&drv_data->adapter.dev,
			"mv64xxx_i2c_fsm: Ctlr Error -- state: 0x%x, "
			"status: 0x%x, addr: 0x%x, flags: 0x%x\n",
			 drv_data->state, status, drv_data->msg->addr,
			 drv_data->msg->flags);
		drv_data->action = ACTION_SEND_STOP;
		mv64xxx_i2c_hw_init(drv_data);
		drv_data->rc = -EIO;
	}
}

static void mv64xxx_i2c_send_start(struct mv64xxx_i2c_data *drv_data)
{
	drv_data->msg = drv_data->msgs;
	drv_data->byte_posn = 0;
	drv_data->bytes_left = drv_data->msg->len;
	drv_data->rc = 0;

	mv64xxx_i2c_prepare_for_io(drv_data, drv_data->msgs);
	mv64xxx_write(drv_data, drv_data->cntl_bits | REG_CONTROL_START,
		drv_data->reg_offsets.control);

	udelay(2);
}

static void
mv64xxx_i2c_do_action(struct mv64xxx_i2c_data *drv_data)
{
	switch (drv_data->action) {
	case ACTION_SEND_RESTART:
		/* We should only get here if we have further messages */
		BUG_ON(drv_data->num_msgs == 0);

		drv_data->msgs++;
		drv_data->num_msgs--;
		mv64xxx_i2c_send_start(drv_data);

		if (drv_data->errata_delay)
			udelay(3);

		/*
		 * We're never at the start of the message here, and by this
		 * time it's already too late to do any protocol mangling.
		 * Thankfully, do not advertise support for that feature.
		 */
		drv_data->send_stop = drv_data->num_msgs == 1;
		break;

	case ACTION_CONTINUE:
		mv64xxx_write(drv_data, drv_data->cntl_bits,
			drv_data->reg_offsets.control);
		break;

	case ACTION_SEND_ADDR_1:
		udelay(2);
		mv64xxx_write(drv_data, drv_data->addr1,
			drv_data->reg_offsets.data);
		mv64xxx_write(drv_data, drv_data->cntl_bits,
			drv_data->reg_offsets.control);
		break;

	case ACTION_SEND_ADDR_2:
		mv64xxx_write(drv_data, drv_data->addr2,
			drv_data->reg_offsets.data);
		mv64xxx_write(drv_data, drv_data->cntl_bits,
			drv_data->reg_offsets.control);
		udelay(2);
		break;

	case ACTION_SEND_DATA:
		udelay(2);
		mv64xxx_write(drv_data, drv_data->msg->buf[drv_data->byte_posn++],
			drv_data->reg_offsets.data);
		mv64xxx_write(drv_data, drv_data->cntl_bits,
			drv_data->reg_offsets.control);
		break;

	case ACTION_RCV_DATA:
		drv_data->msg->buf[drv_data->byte_posn++] =
			mv64xxx_read(drv_data, drv_data->reg_offsets.data);
		mv64xxx_write(drv_data, drv_data->cntl_bits,
			drv_data->reg_offsets.control);
		break;

	case ACTION_RCV_DATA_STOP:
		drv_data->msg->buf[drv_data->byte_posn++] =
			mv64xxx_read(drv_data, drv_data->reg_offsets.data);
		drv_data->cntl_bits &= ~REG_CONTROL_INTEN;
		mv64xxx_write(drv_data, drv_data->cntl_bits | REG_CONTROL_STOP,
			drv_data->reg_offsets.control);
		drv_data->block = false;
		udelay(2);
		if (drv_data->errata_delay)
			udelay(3);

		break;

	case ACTION_INVALID:
	default:
		dev_err(&drv_data->adapter.dev,
			"mv64xxx_i2c_do_action: Invalid action: %d\n",
			drv_data->action);
		drv_data->rc = -EIO;

		/* FALLTHRU */
	case ACTION_SEND_STOP:
		drv_data->cntl_bits &= ~REG_CONTROL_INTEN;
		mv64xxx_write(drv_data, drv_data->cntl_bits
			| REG_CONTROL_STOP,
			drv_data->reg_offsets.control);
		drv_data->block = false;
		break;
	}
}

/*
 *****************************************************************************
 *
 *	I2C Msg Execution Routines
 *
 *****************************************************************************
 */
static void
mv64xxx_i2c_wait_for_completion(struct mv64xxx_i2c_data *drv_data)
{
	u32 status;
	do {
		if (mv64xxx_read(drv_data, drv_data->reg_offsets.control) &
							REG_CONTROL_IFLG) {
			status = mv64xxx_read(drv_data,
					      drv_data->reg_offsets.status);
			mv64xxx_i2c_fsm(drv_data, status);
			mv64xxx_i2c_do_action(drv_data);
		}
		if (drv_data->rc) {
			drv_data->state = STATE_IDLE;
			dev_err(&drv_data->adapter.dev, "I2C bus error\n");
			mv64xxx_i2c_hw_init(drv_data);
			drv_data->block = false;
		}
	} while (drv_data->block);
}

/*
 *****************************************************************************
 *
 *	I2C Core Support Routines (Interface to higher level I2C code)
 *
 *****************************************************************************
 */
static int
mv64xxx_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct mv64xxx_i2c_data *drv_data = container_of(adap, struct mv64xxx_i2c_data, adapter);
	int ret = num;

	BUG_ON(drv_data->msgs != NULL);

	drv_data->msgs = msgs;
	drv_data->num_msgs = num;
	drv_data->state = STATE_WAITING_FOR_START_COND;
	drv_data->send_stop = (num == 1);
	drv_data->block = true;
	mv64xxx_i2c_send_start(drv_data);
	mv64xxx_i2c_wait_for_completion(drv_data);

	if (drv_data->rc < 0)
		ret = drv_data->rc;

	drv_data->num_msgs = 0;
	drv_data->msgs = NULL;

	return ret;
}

/*
 *****************************************************************************
 *
 *	Driver Interface & Early Init Routines
 *
 *****************************************************************************
 */
static struct of_device_id mv64xxx_i2c_of_match_table[] = {
	{ .compatible = "marvell,mv64xxx-i2c", .data = &mv64xxx_i2c_regs_mv64xxx},
	{ .compatible = "marvell,mv78230-i2c", .data = &mv64xxx_i2c_regs_mv64xxx},
	{ .compatible = "marvell,mv78230-a0-i2c", .data = &mv64xxx_i2c_regs_mv64xxx},
	{}
};

static inline int
mv64xxx_calc_freq(const int tclk, const int n, const int m)
{
	return tclk / (10 * (m + 1) * (2 << n));
}

static bool
mv64xxx_find_baud_factors(const u32 req_freq, const u32 tclk, u32 *best_n,
			  u32 *best_m)
{
	int freq, delta, best_delta = INT_MAX;
	int m, n;

	for (n = 0; n <= 7; n++)
		for (m = 0; m <= 15; m++) {
			freq = mv64xxx_calc_freq(tclk, n, m);
			delta = req_freq - freq;
			if (delta >= 0 && delta < best_delta) {
				*best_m = m;
				*best_n = n;
				best_delta = delta;
			}
			if (best_delta == 0)
				return true;
		}
	if (best_delta == INT_MAX)
		return false;
	return true;
}

static int
mv64xxx_of_config(struct mv64xxx_i2c_data *drv_data,
		  struct device_d *pd)
{
	struct device_node *np = pd->device_node;
	u32 bus_freq, tclk;
	int rc = 0;
	u32 prop;
	struct mv64xxx_i2c_regs *mv64xxx_regs;
	int freq;

	if (IS_ERR(drv_data->clk)) {
		rc = -ENODEV;
		goto out;
	}
	tclk = clk_get_rate(drv_data->clk);

	if (of_property_read_u32(np, "clock-frequency", &bus_freq))
		bus_freq = 100000; /* 100kHz by default */

	if (!mv64xxx_find_baud_factors(bus_freq, tclk,
				       &drv_data->freq_n, &drv_data->freq_m)) {
		rc = -EINVAL;
		goto out;
	}

	freq = mv64xxx_calc_freq(tclk, drv_data->freq_n, drv_data->freq_m);
	dev_dbg(pd, "tclk=%d freq_n=%d freq_m=%d freq=%d\n",
			tclk, drv_data->freq_n, drv_data->freq_m, freq);

	if (of_property_read_u32(np, "reg-io-width", &prop)) {
		/* Use 32-bit registers by default */
		prop = 4;
	}

	switch (prop) {
	case 1:
		drv_data->write_reg = mv64xxx_writeb;
		drv_data->read_reg = mv64xxx_readb;
		break;
	case 4:
		drv_data->write_reg = mv64xxx_writel;
		drv_data->read_reg = mv64xxx_readl;
		break;
	default:
		dev_err(pd, "unsupported reg-io-width (%d)\n", prop);
		rc = -EINVAL;
		goto out;
	}

	dev_get_drvdata(pd, (const void **)&mv64xxx_regs);
	memcpy(&drv_data->reg_offsets, mv64xxx_regs,
		sizeof(drv_data->reg_offsets));

	/*
	 * For controllers embedded in new SoCs activate the errata fix.
	 */
	if (of_device_is_compatible(np, "marvell,mv78230-i2c")) {
		drv_data->errata_delay = true;
	}

	if (of_device_is_compatible(np, "marvell,mv78230-a0-i2c")) {
		drv_data->errata_delay = true;
	}

out:
	return rc;
}

static int
mv64xxx_i2c_probe(struct device_d *pd)
{
	struct resource *iores;
	struct mv64xxx_i2c_data		*drv_data;
	int	rc;

	if (!pd->device_node)
		return -ENODEV;

	drv_data = xzalloc(sizeof(*drv_data));

	iores = dev_request_mem_resource(pd, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	drv_data->reg_base = IOMEM(iores->start);

	drv_data->clk = clk_get(pd, NULL);
	if (IS_ERR(drv_data->clk))
		return PTR_ERR(drv_data->clk);

	clk_enable(drv_data->clk);

	rc = mv64xxx_of_config(drv_data, pd);
	if (rc)
		goto exit_clk;

	drv_data->adapter.master_xfer = mv64xxx_i2c_xfer;
	drv_data->adapter.dev.parent = pd;
	drv_data->adapter.nr = pd->id;
	drv_data->adapter.dev.device_node = pd->device_node;

	mv64xxx_i2c_hw_init(drv_data);

	rc = i2c_add_numbered_adapter(&drv_data->adapter);
	if (rc) {
		dev_err(pd, "Failed to add I2C adapter\n");
		goto exit_clk;
	}

	return 0;

exit_clk:
	clk_disable(drv_data->clk);

	return rc;
}

static struct driver_d mv64xxx_i2c_driver = {
	.probe	= mv64xxx_i2c_probe,
	.name = "mv64xxx_i2c",
	.of_compatible = DRV_OF_COMPAT(mv64xxx_i2c_of_match_table),
};
device_platform_driver(mv64xxx_i2c_driver);

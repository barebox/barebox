// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for I2C adapter in Rockchip RK3xxx SoC
 *
 * Max Schwarz <max.schwarz@online.de>
 * based on the patches by Rockchip Inc.
 */

#include <common.h>
#include <driver.h>
#include <i2c/i2c.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <mfd/syscon.h>

/* Register Map */
#define REG_CON			0x00 /* control register */
#define REG_CLKDIV		0x04 /* clock divisor register */
#define REG_MRXADDR		0x08 /* target address for REGISTER_TX */
#define REG_MRXRADDR		0x0c /* target register address for REGISTER_TX */
#define REG_MTXCNT		0x10 /* number of bytes to be transmitted */
#define REG_MRXCNT		0x14 /* number of bytes to be received */
#define REG_IEN			0x18 /* interrupt enable */
#define REG_IPD			0x1c /* interrupt pending */
#define REG_FCNT		0x20 /* finished count */

/* Data buffer offsets */
#define TXBUFFER_BASE		0x100
#define RXBUFFER_BASE		0x200

/* REG_CON bits */
#define REG_CON_EN		BIT(0)
enum {
	REG_CON_MOD_TX = 0,		/* transmit data */
	REG_CON_MOD_REGISTER_TX,	/* select register and restart */
	REG_CON_MOD_RX,			/* receive data */
	REG_CON_MOD_REGISTER_RX,	/* broken: transmits read addr AND writes
					 * register addr */
};
#define REG_CON_MOD(mod)	((mod) << 1)
#define REG_CON_MOD_MASK	(BIT(1) | BIT(2))
#define REG_CON_START		BIT(3)
#define REG_CON_STOP		BIT(4)
#define REG_CON_LASTACK		BIT(5) /* 1: send NACK after last received byte */
#define REG_CON_ACTACK		BIT(6) /* 1: stop if NACK is received */

#define REG_CON_TUNING_MASK GENMASK_ULL(15, 8)

#define REG_CON_SDA_CFG(cfg)	((cfg) << 8)
#define REG_CON_STA_CFG(cfg)	((cfg) << 12)
#define REG_CON_STO_CFG(cfg)	((cfg) << 14)

/* REG_MRXADDR bits */
#define REG_MRXADDR_VALID(x)	BIT(24 + (x)) /* [x*8+7:x*8] of MRX[R]ADDR valid */

/* REG_IEN/REG_IPD bits */
#define REG_INT_BTF		BIT(0) /* a byte was transmitted */
#define REG_INT_BRF		BIT(1) /* a byte was received */
#define REG_INT_MBTF		BIT(2) /* controller data transmit finished */
#define REG_INT_MBRF		BIT(3) /* controller data receive finished */
#define REG_INT_START		BIT(4) /* START condition generated */
#define REG_INT_STOP		BIT(5) /* STOP condition generated */
#define REG_INT_NAKRCV		BIT(6) /* NACK received */
#define REG_INT_ALL		0x7f

/* Constants */
#define WAIT_TIMEOUT		SECOND
#define DEFAULT_SCL_RATE	(100 * 1000) /* Hz */

/**
 * struct i2c_spec_values - I2C specification values for various modes
 * @min_hold_start_ns: min hold time (repeated) START condition
 * @min_low_ns: min LOW period of the SCL clock
 * @min_high_ns: min HIGH period of the SCL cloc
 * @min_setup_start_ns: min set-up time for a repeated START conditio
 * @max_data_hold_ns: max data hold time
 * @min_data_setup_ns: min data set-up time
 * @min_setup_stop_ns: min set-up time for STOP condition
 * @min_hold_buffer_ns: min bus free time between a STOP and
 * START condition
 */
struct i2c_spec_values {
	unsigned long min_hold_start_ns;
	unsigned long min_low_ns;
	unsigned long min_high_ns;
	unsigned long min_setup_start_ns;
	unsigned long max_data_hold_ns;
	unsigned long min_data_setup_ns;
	unsigned long min_setup_stop_ns;
	unsigned long min_hold_buffer_ns;
};

static const struct i2c_spec_values standard_mode_spec = {
	.min_hold_start_ns = 4000,
	.min_low_ns = 4700,
	.min_high_ns = 4000,
	.min_setup_start_ns = 4700,
	.max_data_hold_ns = 3450,
	.min_data_setup_ns = 250,
	.min_setup_stop_ns = 4000,
	.min_hold_buffer_ns = 4700,
};

static const struct i2c_spec_values fast_mode_spec = {
	.min_hold_start_ns = 600,
	.min_low_ns = 1300,
	.min_high_ns = 600,
	.min_setup_start_ns = 600,
	.max_data_hold_ns = 900,
	.min_data_setup_ns = 100,
	.min_setup_stop_ns = 600,
	.min_hold_buffer_ns = 1300,
};

static const struct i2c_spec_values fast_mode_plus_spec = {
	.min_hold_start_ns = 260,
	.min_low_ns = 500,
	.min_high_ns = 260,
	.min_setup_start_ns = 260,
	.max_data_hold_ns = 400,
	.min_data_setup_ns = 50,
	.min_setup_stop_ns = 260,
	.min_hold_buffer_ns = 500,
};

/**
 * struct rk3x_i2c_calced_timings - calculated V1 timings
 * @div_low: Divider output for low
 * @div_high: Divider output for high
 * @tuning: Used to adjust setup/hold data time,
 * setup/hold start time and setup stop time for
 * v1's calc_timings, the tuning should all be 0
 * for old hardware anyone using v0's calc_timings.
 */
struct rk3x_i2c_calced_timings {
	unsigned long div_low;
	unsigned long div_high;
	unsigned int tuning;
};

enum rk3x_i2c_state {
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP
};

/**
 * struct rk3x_i2c_soc_data - SOC-specific data
 * @grf_offset: offset inside the grf regmap for setting the i2c type
 * @calc_timings: Callback function for i2c timing information calculated
 */
struct rk3x_i2c_soc_data {
	int grf_offset;
	int (*calc_timings)(unsigned long, struct i2c_timings *,
			    struct rk3x_i2c_calced_timings *);
};

/**
 * struct rk3x_i2c - private data of the controller
 * @adap: corresponding I2C adapter
 * @dev: device for this controller
 * @soc_data: related soc data struct
 * @regs: virtual memory area
 * @clk: function clk for rk3399 or function & Bus clks for others
 * @pclk: Bus clk for rk3399
 * @t: I2C known timing information
 * @msg: current i2c message
 * @addr: addr of i2c target device
 * @mode: mode of i2c transfer
 * @is_last_msg: flag determines whether it is the last msg in this transfer
 * @state: state of i2c transfer
 * @processed: byte length which has been send or received
 * @error: error code for i2c transfer
 */
struct rk3x_i2c {
	struct i2c_adapter adap;
	struct device *dev;
	const struct rk3x_i2c_soc_data *soc_data;

	/* Hardware resources */
	void __iomem *regs;
	struct clk *clk;
	struct clk *pclk;

	/* Settings */
	struct i2c_timings t;

	/* Current message */
	struct i2c_msg *msg;
	u8 addr;
	unsigned int mode;
	bool is_last_msg;

	/* I2C state machine */
	enum rk3x_i2c_state state;
	unsigned int processed;
	int error;
};

static inline void i2c_writel(struct rk3x_i2c *i2c, u32 value,
			      unsigned int offset)
{
	writel(value, i2c->regs + offset);
}

static inline u32 i2c_readl(struct rk3x_i2c *i2c, unsigned int offset)
{
	return readl(i2c->regs + offset);
}

/* Reset all interrupt pending bits */
static inline void rk3x_i2c_clean_ipd(struct rk3x_i2c *i2c)
{
	i2c_writel(i2c, REG_INT_ALL, REG_IPD);
}

/**
 * rk3x_i2c_start - Generate a START condition, which triggers a REG_INT_START interrupt.
 * @i2c: target controller data
 */
static void rk3x_i2c_start(struct rk3x_i2c *i2c)
{
	u32 val = i2c_readl(i2c, REG_CON) & REG_CON_TUNING_MASK;

	i2c_writel(i2c, REG_INT_START, REG_IEN);

	/* enable adapter with correct mode, send START condition */
	val |= REG_CON_EN | REG_CON_MOD(i2c->mode) | REG_CON_START;

	/* if we want to react to NACK, set ACTACK bit */
	if (!(i2c->msg->flags & I2C_M_IGNORE_NAK))
		val |= REG_CON_ACTACK;

	i2c_writel(i2c, val, REG_CON);
}

/**
 * rk3x_i2c_stop - Generate a STOP condition, which triggers a REG_INT_STOP interrupt.
 * @i2c: target controller data
 * @error: Error code to return in rk3x_i2c_xfer
 */
static void rk3x_i2c_stop(struct rk3x_i2c *i2c, int error)
{
	unsigned int ctrl;

	i2c->processed = 0;
	i2c->msg = NULL;
	i2c->error = error;

	if (i2c->is_last_msg) {
		/* Enable stop interrupt */
		i2c_writel(i2c, REG_INT_STOP, REG_IEN);

		i2c->state = STATE_STOP;

		ctrl = i2c_readl(i2c, REG_CON);
		ctrl |= REG_CON_STOP;
		i2c_writel(i2c, ctrl, REG_CON);
	} else {
		/* Signal rk3x_i2c_xfer to start the next message. */
		i2c->state = STATE_IDLE;

		/*
		 * The HW is actually not capable of REPEATED START. But we can
		 * get the intended effect by resetting its internal state
		 * and issuing an ordinary START.
		 */
		ctrl = i2c_readl(i2c, REG_CON) & REG_CON_TUNING_MASK;
		i2c_writel(i2c, ctrl, REG_CON);
	}
}

/**
 * rk3x_i2c_prepare_read - Setup a read according to i2c->msg
 * @i2c: target controller data
 */
static void rk3x_i2c_prepare_read(struct rk3x_i2c *i2c)
{
	unsigned int len = i2c->msg->len - i2c->processed;
	u32 con;

	con = i2c_readl(i2c, REG_CON);

	/*
	 * The hw can read up to 32 bytes at a time. If we need more than one
	 * chunk, send an ACK after the last byte of the current chunk.
	 */
	if (len > 32) {
		len = 32;
		con &= ~REG_CON_LASTACK;
	} else {
		con |= REG_CON_LASTACK;
	}

	/* make sure we are in plain RX mode if we read a second chunk */
	if (i2c->processed != 0) {
		con &= ~REG_CON_MOD_MASK;
		con |= REG_CON_MOD(REG_CON_MOD_RX);
	}

	i2c_writel(i2c, con, REG_CON);
	i2c_writel(i2c, len, REG_MRXCNT);
}

/**
 * rk3x_i2c_fill_transmit_buf - Fill the transmit buffer with data from i2c->msg
 * @i2c: target controller data
 */
static void rk3x_i2c_fill_transmit_buf(struct rk3x_i2c *i2c)
{
	unsigned int i, j;
	u32 cnt = 0;
	u32 val;
	u8 byte;

	for (i = 0; i < 8; ++i) {
		val = 0;
		for (j = 0; j < 4; ++j) {
			if ((i2c->processed == i2c->msg->len) && (cnt != 0))
				break;

			if (i2c->processed == 0 && cnt == 0)
				byte = (i2c->addr & 0x7f) << 1;
			else
				byte = i2c->msg->buf[i2c->processed++];

			val |= byte << (j * 8);
			cnt++;
		}

		i2c_writel(i2c, val, TXBUFFER_BASE + 4 * i);

		if (i2c->processed == i2c->msg->len)
			break;
	}

	i2c_writel(i2c, cnt, REG_MTXCNT);
}

/* IRQ handlers for individual states */

static void rk3x_i2c_handle_start(struct rk3x_i2c *i2c, unsigned int ipd)
{
	if (!(ipd & REG_INT_START)) {
		rk3x_i2c_stop(i2c, -EIO);
		dev_warn(i2c->dev, "unexpected irq in START: 0x%x\n", ipd);
		rk3x_i2c_clean_ipd(i2c);
		return;
	}

	/* ack interrupt */
	i2c_writel(i2c, REG_INT_START, REG_IPD);

	/* disable start bit */
	i2c_writel(i2c, i2c_readl(i2c, REG_CON) & ~REG_CON_START, REG_CON);

	/* enable appropriate interrupts and transition */
	if (i2c->mode == REG_CON_MOD_TX) {
		i2c_writel(i2c, REG_INT_MBTF | REG_INT_NAKRCV, REG_IEN);
		i2c->state = STATE_WRITE;
		rk3x_i2c_fill_transmit_buf(i2c);
	} else {
		/* in any other case, we are going to be reading. */
		i2c_writel(i2c, REG_INT_MBRF | REG_INT_NAKRCV, REG_IEN);
		i2c->state = STATE_READ;
		rk3x_i2c_prepare_read(i2c);
	}
}

static void rk3x_i2c_handle_write(struct rk3x_i2c *i2c, unsigned int ipd)
{
	if (!(ipd & REG_INT_MBTF)) {
		rk3x_i2c_stop(i2c, -EIO);
		dev_err(i2c->dev, "unexpected irq in WRITE: 0x%x\n", ipd);
		rk3x_i2c_clean_ipd(i2c);
		return;
	}

	/* ack interrupt */
	i2c_writel(i2c, REG_INT_MBTF, REG_IPD);

	/* are we finished? */
	if (i2c->processed == i2c->msg->len)
		rk3x_i2c_stop(i2c, i2c->error);
	else
		rk3x_i2c_fill_transmit_buf(i2c);
}

static void rk3x_i2c_handle_read(struct rk3x_i2c *i2c, unsigned int ipd)
{
	unsigned int i, len = i2c->msg->len - i2c->processed;
	u32 val = 0;
	u8 byte;

	/* we only care for MBRF here. */
	if (!(ipd & REG_INT_MBRF))
		return;

	/* ack interrupt (read also produces a spurious START flag, clear it too) */
	i2c_writel(i2c, REG_INT_MBRF | REG_INT_START, REG_IPD);

	/* Can only handle a maximum of 32 bytes at a time */
	if (len > 32)
		len = 32;

	/* read the data from receive buffer */
	for (i = 0; i < len; ++i) {
		if (i % 4 == 0)
			val = i2c_readl(i2c, RXBUFFER_BASE + (i / 4) * 4);

		byte = (val >> ((i % 4) * 8)) & 0xff;
		i2c->msg->buf[i2c->processed++] = byte;
	}

	/* are we finished? */
	if (i2c->processed == i2c->msg->len)
		rk3x_i2c_stop(i2c, i2c->error);
	else
		rk3x_i2c_prepare_read(i2c);
}

static void rk3x_i2c_handle_stop(struct rk3x_i2c *i2c, unsigned int ipd)
{
	unsigned int con;

	if (!(ipd & REG_INT_STOP)) {
		rk3x_i2c_stop(i2c, -EIO);
		dev_err(i2c->dev, "unexpected irq in STOP: 0x%x\n", ipd);
		rk3x_i2c_clean_ipd(i2c);
		return;
	}

	/* ack interrupt */
	i2c_writel(i2c, REG_INT_STOP, REG_IPD);

	/* disable STOP bit */
	con = i2c_readl(i2c, REG_CON);
	con &= ~REG_CON_STOP;
	i2c_writel(i2c, con, REG_CON);

	i2c->state = STATE_IDLE;
}

static void rk3x_i2c_irq(struct rk3x_i2c *i2c)
{
	unsigned int ipd = i2c_readl(i2c, REG_IPD);

	dev_dbg(i2c->dev, "IRQ: state %d, ipd: %x\n", i2c->state, ipd);

	/* Clean interrupt bits we don't care about */
	ipd &= ~(REG_INT_BRF | REG_INT_BTF);

	if (ipd & REG_INT_NAKRCV) {
		/*
		 * We got a NACK in the last operation. Depending on whether
		 * IGNORE_NAK is set, we have to stop the operation and report
		 * an error.
		 */
		i2c_writel(i2c, REG_INT_NAKRCV, REG_IPD);

		ipd &= ~REG_INT_NAKRCV;

		if (!(i2c->msg->flags & I2C_M_IGNORE_NAK))
			rk3x_i2c_stop(i2c, -ENXIO);
	}

	/* is there anything left to handle? */
	if ((ipd & REG_INT_ALL) == 0)
		return;

	switch (i2c->state) {
	case STATE_START:
		rk3x_i2c_handle_start(i2c, ipd);
		break;
	case STATE_WRITE:
		rk3x_i2c_handle_write(i2c, ipd);
		break;
	case STATE_READ:
		rk3x_i2c_handle_read(i2c, ipd);
		break;
	case STATE_STOP:
		rk3x_i2c_handle_stop(i2c, ipd);
		break;
	default:
		break;
	}
}

/**
 * rk3x_i2c_get_spec - Get timing values of I2C specification
 * @speed: Desired SCL frequency
 *
 * Return: Matched i2c_spec_values.
 */
static const struct i2c_spec_values *rk3x_i2c_get_spec(unsigned int speed)
{
	if (speed <= I2C_MAX_STANDARD_MODE_FREQ)
		return &standard_mode_spec;
	else if (speed <= I2C_MAX_FAST_MODE_FREQ)
		return &fast_mode_spec;
	else
		return &fast_mode_plus_spec;
}

/**
 * rk3x_i2c_v0_calc_timings - Calculate divider values for desired SCL frequency
 * @clk_rate: I2C input clock rate
 * @t: Known I2C timing information
 * @t_calc: Caculated rk3x private timings that would be written into regs
 *
 * Return: %0 on success, -%EINVAL if the goal SCL rate is too slow. In that case
 * a best-effort divider value is returned in divs. If the target rate is
 * too high, we silently use the highest possible rate.
 */
static int rk3x_i2c_v0_calc_timings(unsigned long clk_rate,
				    struct i2c_timings *t,
				    struct rk3x_i2c_calced_timings *t_calc)
{
	unsigned long min_low_ns, min_high_ns;
	unsigned long max_low_ns, min_total_ns;

	unsigned long clk_rate_khz, scl_rate_khz;

	unsigned long min_low_div, min_high_div;
	unsigned long max_low_div;

	unsigned long min_div_for_hold, min_total_div;
	unsigned long extra_div, extra_low_div, ideal_low_div;

	unsigned long data_hold_buffer_ns = 50;
	const struct i2c_spec_values *spec;
	int ret = 0;

	/* Only support standard-mode and fast-mode */
	if (WARN_ON(t->bus_freq_hz > I2C_MAX_FAST_MODE_FREQ))
		t->bus_freq_hz = I2C_MAX_FAST_MODE_FREQ;

	/* prevent scl_rate_khz from becoming 0 */
	if (WARN_ON(t->bus_freq_hz < 1000))
		t->bus_freq_hz = 1000;

	/*
	 * min_low_ns:  The minimum number of ns we need to hold low to
	 *		meet I2C specification, should include fall time.
	 * min_high_ns: The minimum number of ns we need to hold high to
	 *		meet I2C specification, should include rise time.
	 * max_low_ns:  The maximum number of ns we can hold low to meet
	 *		I2C specification.
	 *
	 * Note: max_low_ns should be (maximum data hold time * 2 - buffer)
	 *	 This is because the i2c host on Rockchip holds the data line
	 *	 for half the low time.
	 */
	spec = rk3x_i2c_get_spec(t->bus_freq_hz);
	min_high_ns = t->scl_rise_ns + spec->min_high_ns;

	/*
	 * Timings for repeated start:
	 * - controller appears to drop SDA at .875x (7/8) programmed clk high.
	 * - controller appears to keep SCL high for 2x programmed clk high.
	 *
	 * We need to account for those rules in picking our "high" time so
	 * we meet tSU;STA and tHD;STA times.
	 */
	min_high_ns = max(min_high_ns, DIV_ROUND_UP(
		(t->scl_rise_ns + spec->min_setup_start_ns) * 1000, 875));
	min_high_ns = max(min_high_ns, DIV_ROUND_UP(
		(t->scl_rise_ns + spec->min_setup_start_ns + t->sda_fall_ns +
		spec->min_high_ns), 2));

	min_low_ns = t->scl_fall_ns + spec->min_low_ns;
	max_low_ns =  spec->max_data_hold_ns * 2 - data_hold_buffer_ns;
	min_total_ns = min_low_ns + min_high_ns;

	/* Adjust to avoid overflow */
	clk_rate_khz = DIV_ROUND_UP(clk_rate, 1000);
	scl_rate_khz = t->bus_freq_hz / 1000;

	/*
	 * We need the total div to be >= this number
	 * so we don't clock too fast.
	 */
	min_total_div = DIV_ROUND_UP(clk_rate_khz, scl_rate_khz * 8);

	/* These are the min dividers needed for min hold times. */
	min_low_div = DIV_ROUND_UP(clk_rate_khz * min_low_ns, 8 * 1000000);
	min_high_div = DIV_ROUND_UP(clk_rate_khz * min_high_ns, 8 * 1000000);
	min_div_for_hold = (min_low_div + min_high_div);

	/*
	 * This is the maximum divider so we don't go over the maximum.
	 * We don't round up here (we round down) since this is a maximum.
	 */
	max_low_div = clk_rate_khz * max_low_ns / (8 * 1000000);

	if (min_low_div > max_low_div) {
		WARN_ONCE(true,
			  "Conflicting, min_low_div %lu, max_low_div %lu\n",
			  min_low_div, max_low_div);
		max_low_div = min_low_div;
	}

	if (min_div_for_hold > min_total_div) {
		/*
		 * Time needed to meet hold requirements is important.
		 * Just use that.
		 */
		t_calc->div_low = min_low_div;
		t_calc->div_high = min_high_div;
	} else {
		/*
		 * We've got to distribute some time among the low and high
		 * so we don't run too fast.
		 */
		extra_div = min_total_div - min_div_for_hold;

		/*
		 * We'll try to split things up perfectly evenly,
		 * biasing slightly towards having a higher div
		 * for low (spend more time low).
		 */
		ideal_low_div = DIV_ROUND_UP(clk_rate_khz * min_low_ns,
					     scl_rate_khz * 8 * min_total_ns);

		/* Don't allow it to go over the maximum */
		if (ideal_low_div > max_low_div)
			ideal_low_div = max_low_div;

		/*
		 * Handle when the ideal low div is going to take up
		 * more than we have.
		 */
		if (ideal_low_div > min_low_div + extra_div)
			ideal_low_div = min_low_div + extra_div;

		/* Give low the "ideal" and give high whatever extra is left */
		extra_low_div = ideal_low_div - min_low_div;
		t_calc->div_low = ideal_low_div;
		t_calc->div_high = min_high_div + (extra_div - extra_low_div);
	}

	/*
	 * Adjust to the fact that the hardware has an implicit "+1".
	 * NOTE: Above calculations always produce div_low > 0 and div_high > 0.
	 */
	t_calc->div_low--;
	t_calc->div_high--;

	/* Give the tuning value 0, that would not update con register */
	t_calc->tuning = 0;
	/* Maximum divider supported by hw is 0xffff */
	if (t_calc->div_low > 0xffff) {
		t_calc->div_low = 0xffff;
		ret = -EINVAL;
	}

	if (t_calc->div_high > 0xffff) {
		t_calc->div_high = 0xffff;
		ret = -EINVAL;
	}

	return ret;
}

/**
 * rk3x_i2c_v1_calc_timings - Calculate timing values for desired SCL frequency
 * @clk_rate: I2C input clock rate
 * @t: Known I2C timing information
 * @t_calc: Caculated rk3x private timings that would be written into regs
 *
 * Return: %0 on success, -%EINVAL if the goal SCL rate is too slow. In that case
 * a best-effort divider value is returned in divs. If the target rate is
 * too high, we silently use the highest possible rate.
 * The following formulas are v1's method to calculate timings.
 *
 * l = divl + 1;
 * h = divh + 1;
 * s = sda_update_config + 1;
 * u = start_setup_config + 1;
 * p = stop_setup_config + 1;
 * T = Tclk_i2c;
 *
 * tHigh = 8 * h * T;
 * tLow = 8 * l * T;
 *
 * tHD;sda = (l * s + 1) * T;
 * tSU;sda = [(8 - s) * l + 1] * T;
 * tI2C = 8 * (l + h) * T;
 *
 * tSU;sta = (8h * u + 1) * T;
 * tHD;sta = [8h * (u + 1) - 1] * T;
 * tSU;sto = (8h * p + 1) * T;
 */
static int rk3x_i2c_v1_calc_timings(unsigned long clk_rate,
				    struct i2c_timings *t,
				    struct rk3x_i2c_calced_timings *t_calc)
{
	unsigned long min_low_ns, min_high_ns;
	unsigned long min_setup_start_ns, min_setup_data_ns;
	unsigned long min_setup_stop_ns, max_hold_data_ns;

	unsigned long clk_rate_khz, scl_rate_khz;

	unsigned long min_low_div, min_high_div;

	unsigned long min_div_for_hold, min_total_div;
	unsigned long extra_div, extra_low_div;
	unsigned long sda_update_cfg, stp_sta_cfg, stp_sto_cfg;

	const struct i2c_spec_values *spec;
	int ret = 0;

	/* Support standard-mode, fast-mode and fast-mode plus */
	if (WARN_ON(t->bus_freq_hz > I2C_MAX_FAST_MODE_PLUS_FREQ))
		t->bus_freq_hz = I2C_MAX_FAST_MODE_PLUS_FREQ;

	/* prevent scl_rate_khz from becoming 0 */
	if (WARN_ON(t->bus_freq_hz < 1000))
		t->bus_freq_hz = 1000;

	/*
	 * min_low_ns: The minimum number of ns we need to hold low to
	 *	       meet I2C specification, should include fall time.
	 * min_high_ns: The minimum number of ns we need to hold high to
	 *	        meet I2C specification, should include rise time.
	 */
	spec = rk3x_i2c_get_spec(t->bus_freq_hz);

	/* calculate min-divh and min-divl */
	clk_rate_khz = DIV_ROUND_UP(clk_rate, 1000);
	scl_rate_khz = t->bus_freq_hz / 1000;
	min_total_div = DIV_ROUND_UP(clk_rate_khz, scl_rate_khz * 8);

	min_high_ns = t->scl_rise_ns + spec->min_high_ns;
	min_high_div = DIV_ROUND_UP(clk_rate_khz * min_high_ns, 8 * 1000000);

	min_low_ns = t->scl_fall_ns + spec->min_low_ns;
	min_low_div = DIV_ROUND_UP(clk_rate_khz * min_low_ns, 8 * 1000000);

	/*
	 * Final divh and divl must be greater than 0, otherwise the
	 * hardware would not output the i2c clk.
	 */
	min_high_div = (min_high_div < 1) ? 2 : min_high_div;
	min_low_div = (min_low_div < 1) ? 2 : min_low_div;

	/* These are the min dividers needed for min hold times. */
	min_div_for_hold = (min_low_div + min_high_div);

	/*
	 * This is the maximum divider so we don't go over the maximum.
	 * We don't round up here (we round down) since this is a maximum.
	 */
	if (min_div_for_hold >= min_total_div) {
		/*
		 * Time needed to meet hold requirements is important.
		 * Just use that.
		 */
		t_calc->div_low = min_low_div;
		t_calc->div_high = min_high_div;
	} else {
		/*
		 * We've got to distribute some time among the low and high
		 * so we don't run too fast.
		 * We'll try to split things up by the scale of min_low_div and
		 * min_high_div, biasing slightly towards having a higher div
		 * for low (spend more time low).
		 */
		extra_div = min_total_div - min_div_for_hold;
		extra_low_div = DIV_ROUND_UP(min_low_div * extra_div,
					     min_div_for_hold);

		t_calc->div_low = min_low_div + extra_low_div;
		t_calc->div_high = min_high_div + (extra_div - extra_low_div);
	}

	/*
	 * calculate sda data hold count by the rules, data_upd_st:3
	 * is a appropriate value to reduce calculated times.
	 */
	for (sda_update_cfg = 3; sda_update_cfg > 0; sda_update_cfg--) {
		max_hold_data_ns =  DIV_ROUND_UP((sda_update_cfg
						 * (t_calc->div_low) + 1)
						 * 1000000, clk_rate_khz);
		min_setup_data_ns =  DIV_ROUND_UP(((8 - sda_update_cfg)
						 * (t_calc->div_low) + 1)
						 * 1000000, clk_rate_khz);
		if ((max_hold_data_ns < spec->max_data_hold_ns) &&
		    (min_setup_data_ns > spec->min_data_setup_ns))
			break;
	}

	/* calculate setup start config */
	min_setup_start_ns = t->scl_rise_ns + spec->min_setup_start_ns;
	stp_sta_cfg = DIV_ROUND_UP(clk_rate_khz * min_setup_start_ns
			   - 1000000, 8 * 1000000 * (t_calc->div_high));

	/* calculate setup stop config */
	min_setup_stop_ns = t->scl_rise_ns + spec->min_setup_stop_ns;
	stp_sto_cfg = DIV_ROUND_UP(clk_rate_khz * min_setup_stop_ns
			   - 1000000, 8 * 1000000 * (t_calc->div_high));

	t_calc->tuning = REG_CON_SDA_CFG(--sda_update_cfg) |
			 REG_CON_STA_CFG(--stp_sta_cfg) |
			 REG_CON_STO_CFG(--stp_sto_cfg);

	t_calc->div_low--;
	t_calc->div_high--;

	/* Maximum divider supported by hw is 0xffff */
	if (t_calc->div_low > 0xffff) {
		t_calc->div_low = 0xffff;
		ret = -EINVAL;
	}

	if (t_calc->div_high > 0xffff) {
		t_calc->div_high = 0xffff;
		ret = -EINVAL;
	}

	return ret;
}

static void rk3x_i2c_adapt_div(struct rk3x_i2c *i2c, unsigned long clk_rate)
{
	struct i2c_timings *t = &i2c->t;
	struct rk3x_i2c_calced_timings calc;
	u64 t_low_ns, t_high_ns;
	u32 val;
	int ret;

	ret = i2c->soc_data->calc_timings(clk_rate, t, &calc);
	WARN_ONCE(ret != 0, "Could not reach SCL freq %u", t->bus_freq_hz);

	clk_enable(i2c->pclk);

	val = i2c_readl(i2c, REG_CON);
	val &= ~REG_CON_TUNING_MASK;
	val |= calc.tuning;
	i2c_writel(i2c, val, REG_CON);
	i2c_writel(i2c, (calc.div_high << 16) | (calc.div_low & 0xffff),
		   REG_CLKDIV);

	clk_disable(i2c->pclk);

	t_low_ns = div_u64(((u64)calc.div_low + 1) * 8 * 1000000000, clk_rate);
	t_high_ns = div_u64(((u64)calc.div_high + 1) * 8 * 1000000000,
			    clk_rate);
	dev_dbg(i2c->dev,
		"CLK %lukhz, Req %uns, Act low %lluns high %lluns\n",
		clk_rate / 1000,
		1000000000 / t->bus_freq_hz,
		t_low_ns, t_high_ns);
}

/**
 * rk3x_i2c_setup - Setup I2C registers for an I2C operation specified by msgs, num.
 * @i2c: target controller data
 * @msgs: I2C msgs to process
 * @num: Number of msgs
 *
 * Return: Number of I2C msgs processed or negative in case of error
 */
static int rk3x_i2c_setup(struct rk3x_i2c *i2c, struct i2c_msg *msgs, int num)
{
	u32 addr = (msgs[0].addr & 0x7f) << 1;
	int ret = 0;

	/*
	 * The I2C adapter can issue a small (len < 4) write packet before
	 * reading. This speeds up SMBus-style register reads.
	 * The MRXADDR/MRXRADDR hold the target address and the target register
	 * address in this case.
	 */

	if (num >= 2 && msgs[0].len < 4 &&
	    !(msgs[0].flags & I2C_M_RD) && (msgs[1].flags & I2C_M_RD)) {
		u32 reg_addr = 0;
		int i;

		dev_dbg(i2c->dev, "Combined write/read from addr 0x%x\n",
			addr >> 1);

		/* Fill MRXRADDR with the register address(es) */
		for (i = 0; i < msgs[0].len; ++i) {
			reg_addr |= msgs[0].buf[i] << (i * 8);
			reg_addr |= REG_MRXADDR_VALID(i);
		}

		/* msgs[0] is handled by hw. */
		i2c->msg = &msgs[1];

		i2c->mode = REG_CON_MOD_REGISTER_TX;

		i2c_writel(i2c, addr | REG_MRXADDR_VALID(0), REG_MRXADDR);
		i2c_writel(i2c, reg_addr, REG_MRXRADDR);

		ret = 2;
	} else {
		/*
		 * We'll have to do it the boring way and process the msgs
		 * one-by-one.
		 */

		if (msgs[0].flags & I2C_M_RD) {
			addr |= 1; /* set read bit */

			/*
			 * We have to transmit the target addr first. Use
			 * MOD_REGISTER_TX for that purpose.
			 */
			i2c->mode = REG_CON_MOD_REGISTER_TX;
			i2c_writel(i2c, addr | REG_MRXADDR_VALID(0),
				   REG_MRXADDR);
			i2c_writel(i2c, 0, REG_MRXRADDR);
		} else {
			i2c->mode = REG_CON_MOD_TX;
		}

		i2c->msg = &msgs[0];

		ret = 1;
	}

	i2c->addr = msgs[0].addr;
	i2c->state = STATE_START;
	i2c->processed = 0;
	i2c->error = 0;

	rk3x_i2c_clean_ipd(i2c);

	return ret;
}

static int rk3x_i2c_wait_xfer_poll(struct rk3x_i2c *i2c)
{
	uint64_t start = get_time_ns();

	while (!is_timeout(start, WAIT_TIMEOUT)) {
		rk3x_i2c_irq(i2c);
		if (i2c->state == STATE_IDLE)
			return 0;
	}

	return -ETIMEDOUT;
}

static int rk3x_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct rk3x_i2c *i2c = (struct rk3x_i2c *)adap->algo_data;
	u32 val;
	int i, ret = 0;

	clk_enable(i2c->clk);
	clk_enable(i2c->pclk);

	i2c->is_last_msg = false;

	/*
	 * Process msgs. We can handle more than one message at once (see
	 * rk3x_i2c_setup()).
	 */
	for (i = 0; i < num; i += ret) {
		int xfer_ret;

		ret = rk3x_i2c_setup(i2c, msgs + i, num - i);
		if (ret < 0) {
			dev_err(i2c->dev, "rk3x_i2c_setup() failed\n");
			break;
		}

		if (i + ret >= num)
			i2c->is_last_msg = true;

		rk3x_i2c_start(i2c);

		xfer_ret = rk3x_i2c_wait_xfer_poll(i2c);
		if (xfer_ret) {
			/* Force a STOP condition without interrupt */
			i2c_writel(i2c, 0, REG_IEN);
			val = i2c_readl(i2c, REG_CON) & REG_CON_TUNING_MASK;
			val |= REG_CON_EN | REG_CON_STOP;
			i2c_writel(i2c, val, REG_CON);

			i2c->state = STATE_IDLE;

			ret = xfer_ret;
			break;
		}

		if (i2c->error) {
			ret = i2c->error;
			break;
		}
	}

	clk_disable(i2c->pclk);
	clk_disable(i2c->clk);

	return ret < 0 ? ret : num;
}

static const struct rk3x_i2c_soc_data rv1108_soc_data = {
	.grf_offset = -1,
	.calc_timings = rk3x_i2c_v1_calc_timings,
};

static const struct rk3x_i2c_soc_data rv1126_soc_data = {
	.grf_offset = 0x118,
	.calc_timings = rk3x_i2c_v1_calc_timings,
};

static const struct rk3x_i2c_soc_data rk3066_soc_data = {
	.grf_offset = 0x154,
	.calc_timings = rk3x_i2c_v0_calc_timings,
};

static const struct rk3x_i2c_soc_data rk3188_soc_data = {
	.grf_offset = 0x0a4,
	.calc_timings = rk3x_i2c_v0_calc_timings,
};

static const struct rk3x_i2c_soc_data rk3228_soc_data = {
	.grf_offset = -1,
	.calc_timings = rk3x_i2c_v0_calc_timings,
};

static const struct rk3x_i2c_soc_data rk3288_soc_data = {
	.grf_offset = -1,
	.calc_timings = rk3x_i2c_v0_calc_timings,
};

static const struct rk3x_i2c_soc_data rk3399_soc_data = {
	.grf_offset = -1,
	.calc_timings = rk3x_i2c_v1_calc_timings,
};

static const struct of_device_id rk3x_i2c_match[] = {
	{
		.compatible = "rockchip,rv1108-i2c",
		.data = &rv1108_soc_data
	},
	{
		.compatible = "rockchip,rv1126-i2c",
		.data = &rv1126_soc_data
	},
	{
		.compatible = "rockchip,rk3066-i2c",
		.data = &rk3066_soc_data
	},
	{
		.compatible = "rockchip,rk3188-i2c",
		.data = &rk3188_soc_data
	},
	{
		.compatible = "rockchip,rk3228-i2c",
		.data = &rk3228_soc_data
	},
	{
		.compatible = "rockchip,rk3288-i2c",
		.data = &rk3288_soc_data
	},
	{
		.compatible = "rockchip,rk3399-i2c",
		.data = &rk3399_soc_data
	},
	{},
};
MODULE_DEVICE_TABLE(of, rk3x_i2c_match);

static int rk3x_i2c_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	const struct rk3x_i2c_soc_data *data;
	struct resource *iores;
	unsigned long clk_rate;
	struct rk3x_i2c *i2c;
	int ret;

	i2c = xzalloc(sizeof(*i2c));
	i2c->dev = dev;

	data = device_get_match_data(dev);
	if (!data)
		return dev_err_probe(dev, -EINVAL, "Failed to retrieve driver data\n");

	i2c->soc_data = data;

	/* use common interface to get I2C timing properties */
	i2c_parse_fw_timings(dev, &i2c->t, true);

	i2c->adap.master_xfer = rk3x_i2c_xfer;
	i2c->adap.retries = 3;
	i2c->adap.dev.of_node = np;
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = dev;
	i2c->adap.nr = of_alias_get_id(np, "i2c");

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	dev->priv = i2c;
	i2c->regs = IOMEM(iores->start);

	/*
	 * Switch to new interface if the SoC also offers the old one.
	 * The control bit is located in the GRF register space.
	 */
	if (i2c->soc_data->grf_offset >= 0) {
		struct regmap *grf;
		u32 value;

		grf = syscon_regmap_lookup_by_phandle(np, "rockchip,grf");
		if (IS_ERR(grf))
			return dev_err_probe(dev, PTR_ERR(grf),
					     "rk3x-i2c needs 'rockchip,grf' property\n");

		if (i2c->adap.nr < 0)
			return dev_err_probe(dev, -EINVAL,
					     "rk3x-i2c needs i2cX alias\n");

		/* rv1126 i2c2 uses non-sequential write mask 20, value 4 */
		if (i2c->soc_data == &rv1126_soc_data && i2c->adap.nr == 2)
			value = BIT(20) | BIT(4);
		else
			/* 27+i: write mask, 11+i: value */
			value = BIT(27 + i2c->adap.nr) | BIT(11 + i2c->adap.nr);

		ret = regmap_write(grf, i2c->soc_data->grf_offset, value);
		if (ret)
			return dev_err_probe(dev, ret, "Could not write to GRF\n");
	}

	if (i2c->soc_data->calc_timings == rk3x_i2c_v0_calc_timings) {
		/* Only one clock to use for bus clock and peripheral clock */
		i2c->clk = clk_get(dev, NULL);
		i2c->pclk = i2c->clk;
	} else {
		i2c->clk = clk_get(dev, "i2c");
		i2c->pclk = clk_get(dev, "pclk");
	}

	if (IS_ERR(i2c->clk))
		return dev_err_probe(dev, PTR_ERR(i2c->clk),
				     "Can't get bus clk\n");

	if (IS_ERR(i2c->pclk))
		return dev_err_probe(dev, PTR_ERR(i2c->pclk),
				     "Can't get periph clk\n");

	ret = clk_enable(i2c->clk);
	if (ret < 0)
		return dev_err_probe(dev, ret,  "Can't enable bus clk\n");

	clk_rate = clk_get_rate(i2c->clk);
	rk3x_i2c_adapt_div(i2c, clk_rate);

	clk_disable(i2c->clk);

	return i2c_add_numbered_adapter(&i2c->adap);
}

static struct driver rk3x_i2c_driver = {
	.name  = "rk3x-i2c",
	.of_compatible = rk3x_i2c_match,
	.probe   = rk3x_i2c_probe,
};
coredevice_platform_driver(rk3x_i2c_driver);

MODULE_DESCRIPTION("Rockchip RK3xxx I2C Bus driver");
MODULE_AUTHOR("Max Schwarz <max.schwarz@online.de>");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2017 STMicroelectronics
 * Copyright 2019 Ahmad Fatoum, Pengutronix
 */

#define pr_fmt(fmt) "i2c-stm32: " fmt

#include <common.h>
#include <i2c/i2c.h>
#include <init.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/reset.h>
#include <mfd/syscon.h>

/* STM32 I2C registers */
struct __packed stm32_i2c_regs {
	u32 cr1;	/* I2C control register 1 */
	u32 cr2;	/* I2C control register 2 */
	u32 oar1;	/* I2C own address 1 register */
	u32 oar2;	/* I2C own address 2 register */
	u32 timingr;	/* I2C timing register */
	u32 timeoutr;	/* I2C timeout register */
	u32 isr;	/* I2C interrupt and status register */
	u32 icr;	/* I2C interrupt clear register */
	u32 pecr;	/* I2C packet error checking register */
	u32 rxdr;	/* I2C receive data register */
	u32 txdr;	/* I2C transmit data register */
};

#define STM32_I2C_CR1				0x00
#define STM32_I2C_CR2				0x04
#define STM32_I2C_TIMINGR			0x10
#define STM32_I2C_ISR				0x18
#define STM32_I2C_ICR				0x1C
#define STM32_I2C_RXDR				0x24
#define STM32_I2C_TXDR				0x28

/* STM32 I2C control 1 */
#define STM32_I2C_CR1_ANFOFF			BIT(12)
#define STM32_I2C_CR1_DNF_MASK			GENMASK(11, 8)
#define STM32_I2C_CR1_DNF(n)			(((n) & 0xf) << 8)
#define STM32_I2C_CR1_ERRIE			BIT(7)
#define STM32_I2C_CR1_TCIE			BIT(6)
#define STM32_I2C_CR1_STOPIE			BIT(5)
#define STM32_I2C_CR1_NACKIE			BIT(4)
#define STM32_I2C_CR1_ADDRIE			BIT(3)
#define STM32_I2C_CR1_RXIE			BIT(2)
#define STM32_I2C_CR1_TXIE			BIT(1)
#define STM32_I2C_CR1_PE			BIT(0)

/* STM32 I2C control 2 */
#define STM32_I2C_CR2_RELOAD			BIT(24)
#define STM32_I2C_CR2_NBYTES_MASK		GENMASK(23, 16)
#define STM32_I2C_CR2_NBYTES(n)			((n & 0xff) << 16)
#define STM32_I2C_CR2_NACK			BIT(15)
#define STM32_I2C_CR2_STOP			BIT(14)
#define STM32_I2C_CR2_START			BIT(13)
#define STM32_I2C_CR2_HEAD10R			BIT(12)
#define STM32_I2C_CR2_ADD10			BIT(11)
#define STM32_I2C_CR2_RD_WRN			BIT(10)
#define STM32_I2C_CR2_SADD10_MASK		GENMASK(9, 0)
#define STM32_I2C_CR2_SADD10(n)			(n & STM32_I2C_CR2_SADD10_MASK)
#define STM32_I2C_CR2_SADD7_MASK		GENMASK(7, 1)
#define STM32_I2C_CR2_SADD7(n)			((n & 0x7f) << 1)
#define STM32_I2C_CR2_RESET_MASK		(STM32_I2C_CR2_HEAD10R \
						| STM32_I2C_CR2_NBYTES_MASK \
						| STM32_I2C_CR2_SADD7_MASK \
						| STM32_I2C_CR2_RELOAD \
						| STM32_I2C_CR2_RD_WRN)

/* STM32 I2C Interrupt Status */
#define STM32_I2C_ISR_BUSY			BIT(15)
#define STM32_I2C_ISR_ARLO			BIT(9)
#define STM32_I2C_ISR_BERR			BIT(8)
#define STM32_I2C_ISR_TCR			BIT(7)
#define STM32_I2C_ISR_TC			BIT(6)
#define STM32_I2C_ISR_STOPF			BIT(5)
#define STM32_I2C_ISR_NACKF			BIT(4)
#define STM32_I2C_ISR_ADDR			BIT(3)
#define STM32_I2C_ISR_RXNE			BIT(2)
#define STM32_I2C_ISR_TXIS			BIT(1)
#define STM32_I2C_ISR_TXE			BIT(0)
#define STM32_I2C_ISR_ERRORS			(STM32_I2C_ISR_BERR \
						| STM32_I2C_ISR_ARLO)

/* STM32 I2C Interrupt Clear */
#define STM32_I2C_ICR_ARLOCF			BIT(9)
#define STM32_I2C_ICR_BERRCF			BIT(8)
#define STM32_I2C_ICR_STOPCF			BIT(5)
#define STM32_I2C_ICR_NACKCF			BIT(4)

/* STM32 I2C Timing */
#define STM32_I2C_TIMINGR_PRESC(n)		((n & 0xf) << 28)
#define STM32_I2C_TIMINGR_SCLDEL(n)		((n & 0xf) << 20)
#define STM32_I2C_TIMINGR_SDADEL(n)		((n & 0xf) << 16)
#define STM32_I2C_TIMINGR_SCLH(n)		((n & 0xff) << 8)
#define STM32_I2C_TIMINGR_SCLL(n)		(n & 0xff)

#define STM32_I2C_MAX_LEN			0xff

#define STM32_I2C_DNF_MAX			15

#define STM32_I2C_ANALOG_FILTER_DELAY_MIN	50	/* ns */
#define STM32_I2C_ANALOG_FILTER_DELAY_MAX	260	/* ns */

#define STM32_I2C_RISE_TIME_DEFAULT		25	/* ns */
#define STM32_I2C_FALL_TIME_DEFAULT		10	/* ns */

#define STM32_PRESC_MAX				BIT(4)
#define STM32_SCLDEL_MAX			BIT(4)
#define STM32_SDADEL_MAX			BIT(4)
#define STM32_SCLH_MAX				BIT(8)
#define STM32_SCLL_MAX				BIT(8)

#define STANDARD_RATE				100000
#define FAST_RATE				400000
#define FAST_PLUS_RATE				1000000

enum stm32_i2c_speed {
	IC_SPEED_MODE_STANDARD,
	IC_SPEED_MODE_FAST,
	IC_SPEED_MODE_FAST_PLUS,
};

/**
 * struct stm32_i2c_spec - private i2c specification timing
 * @rate: I2C bus speed (Hz)
 * @rate_min: 80% of I2C bus speed (Hz)
 * @rate_max: 120% of I2C bus speed (Hz)
 * @fall_max: Max fall time of both SDA and SCL signals (ns)
 * @rise_max: Max rise time of both SDA and SCL signals (ns)
 * @hddat_min: Min data hold time (ns)
 * @vddat_max: Max data valid time (ns)
 * @sudat_min: Min data setup time (ns)
 * @l_min: Min low period of the SCL clock (ns)
 * @h_min: Min high period of the SCL clock (ns)
 */

struct stm32_i2c_spec {
	u32 rate;
	u32 rate_min;
	u32 rate_max;
	u32 fall_max;
	u32 rise_max;
	u32 hddat_min;
	u32 vddat_max;
	u32 sudat_min;
	u32 l_min;
	u32 h_min;
};

/**
 * struct stm32_i2c_setup - private I2C timing setup parameters
 * @clock_src: I2C clock source frequency (Hz)
 * @timings: I2C timing information
 * @analog_filter: Analog filter delay (On/Off)
 */
struct stm32_i2c_setup {
	u32 clock_src;
	struct i2c_timings timings;
	u8 dnf;
	bool analog_filter;
};

/**
 * struct stm32_i2c_data - driver data for I2C configuration by compatible
 * @fmp_clr_offset: Fast Mode Plus clear register offset from set register
 */
struct stm32_i2c_data {
	u32 fmp_clr_offset;
};

/**
 * struct stm32_i2c_timings - private I2C output parameters
 * @prec: Prescaler value
 * @scldel: Data setup time
 * @sdadel: Data hold time
 * @sclh: SCL high period (master mode)
 * @sclh: SCL low period (master mode)
 */
struct stm32_i2c_timings {
	struct list_head node;
	u8 presc;
	u8 scldel;
	u8 sdadel;
	u8 sclh;
	u8 scll;
};

/**
 * struct stm32_i2c_priv - private data of the controller
 * @regs: I2C registers address
 * @clk: hw i2c clock
 * @setup: I2C timing setup parameters
 * @speed: I2C clock frequency of the controller. Standard, Fast or Fast+
 * @regmap: holds SYSCFG phandle for Fast Mode Plus bit
 * @regmap_sreg: register address for setting Fast Mode Plus bits
 * @regmap_creg: register address for clearing Fast Mode Plus bits
 * @regmap_mask: mask for Fast Mode Plus bits
 */
struct stm32_i2c_priv {
	struct stm32_i2c_regs __iomem *regs;
	struct clk *clk;
	struct i2c_adapter adapter;
	struct stm32_i2c_setup setup;
	u32 speed;
	struct regmap *regmap;
	u32 regmap_sreg;
	u32 regmap_creg;
	u32 regmap_mask;
};

static const struct stm32_i2c_spec i2c_specs[] = {
	/* Standard speed - 100 KHz */
	[IC_SPEED_MODE_STANDARD] = {
		.rate = STANDARD_RATE,
		.rate_min = 8000,
		.rate_max = 120000,
		.fall_max = 300,
		.rise_max = 1000,
		.hddat_min = 0,
		.vddat_max = 3450,
		.sudat_min = 250,
		.l_min = 4700,
		.h_min = 4000,
	},
	/* Fast speed - 400 KHz */
	[IC_SPEED_MODE_FAST] = {
		.rate = FAST_RATE,
		.rate_min = 320000,
		.rate_max = 480000,
		.fall_max = 300,
		.rise_max = 300,
		.hddat_min = 0,
		.vddat_max = 900,
		.sudat_min = 100,
		.l_min = 1300,
		.h_min = 600,
	},
	/* Fast Plus Speed - 1 MHz */
	[IC_SPEED_MODE_FAST_PLUS] = {
		.rate = FAST_PLUS_RATE,
		.rate_min = 800000,
		.rate_max = 1200000,
		.fall_max = 100,
		.rise_max = 120,
		.hddat_min = 0,
		.vddat_max = 450,
		.sudat_min = 50,
		.l_min = 500,
		.h_min = 260,
	},
};

static const struct stm32_i2c_data stm32f7_data = {
	.fmp_clr_offset = 0x00,
};

static const struct stm32_i2c_data stm32mp15_data = {
	.fmp_clr_offset = 0x40,
};

static const struct stm32_i2c_data stm32mp13_data = {
	.fmp_clr_offset = 0x4,
};

static inline int stm32_i2c_check_device_busy(struct stm32_i2c_priv *priv)
{
	u32 status = readl(&priv->regs->isr);

	if (status & STM32_I2C_ISR_BUSY)
		return -EBUSY;

	return 0;
}

static void stm32_i2c_message_start(struct stm32_i2c_priv *i2c_priv,
				    struct i2c_msg *msg)
{
	struct stm32_i2c_regs *regs = i2c_priv->regs;
	u32 cr2 = readl(&regs->cr2);

	/* Set transfer direction */
	cr2 &= ~STM32_I2C_CR2_RD_WRN;
	if (msg->flags & I2C_M_RD)
		cr2 |= STM32_I2C_CR2_RD_WRN;

	/* Set slave address */
	cr2 &= ~(STM32_I2C_CR2_HEAD10R | STM32_I2C_CR2_ADD10);
	if (msg->flags & I2C_M_TEN) {
		cr2 &= ~STM32_I2C_CR2_SADD10_MASK;
		cr2 |= STM32_I2C_CR2_SADD10(msg->addr);
		cr2 |= STM32_I2C_CR2_ADD10;
	} else {
		cr2 &= ~STM32_I2C_CR2_SADD7_MASK;
		cr2 |= STM32_I2C_CR2_SADD7(msg->addr);
	}

	/* Set nb bytes to transfer and reload (if needed) */
	cr2 &= ~(STM32_I2C_CR2_NBYTES_MASK | STM32_I2C_CR2_RELOAD);
	if (msg->len > STM32_I2C_MAX_LEN) {
		cr2 |= STM32_I2C_CR2_NBYTES(STM32_I2C_MAX_LEN);
		cr2 |= STM32_I2C_CR2_RELOAD;
	} else {
		cr2 |= STM32_I2C_CR2_NBYTES(msg->len);
	}

	/* Write configurations register */
	writel(cr2, &regs->cr2);

	/* START/ReSTART generation */
	setbits_le32(&regs->cr2, STM32_I2C_CR2_START);
}

/*
 * RELOAD mode must be selected if total number of data bytes to be
 * sent is greater than MAX_LEN
 */

static void stm32_i2c_handle_reload(struct stm32_i2c_priv *i2c_priv,
				    struct i2c_msg *msg)
{
	struct stm32_i2c_regs *regs = i2c_priv->regs;
	u32 cr2 = readl(&regs->cr2);

	cr2 &= ~STM32_I2C_CR2_NBYTES_MASK;

	if (msg->len > STM32_I2C_MAX_LEN) {
		cr2 |= STM32_I2C_CR2_NBYTES(STM32_I2C_MAX_LEN);
	} else {
		cr2 &= ~STM32_I2C_CR2_RELOAD;
		cr2 |= STM32_I2C_CR2_NBYTES(msg->len);
	}

	writel(cr2, &regs->cr2);
}

static int stm32_i2c_wait_flags(struct stm32_i2c_priv *i2c_priv,
				u32 flags, u32 *status)
{
	struct stm32_i2c_regs *regs = i2c_priv->regs;

	return readl_poll_timeout(&regs->isr, *status,
				  *status & flags, USEC_PER_SEC);
}

static int stm32_i2c_check_end_of_message(struct stm32_i2c_priv *i2c_priv)
{
	struct stm32_i2c_regs *regs = i2c_priv->regs;
	u32 mask = STM32_I2C_ISR_ERRORS | STM32_I2C_ISR_NACKF |
		   STM32_I2C_ISR_STOPF;
	struct device *dev = &i2c_priv->adapter.dev;
	u32 status;
	int ret;

	ret = stm32_i2c_wait_flags(i2c_priv, mask, &status);
	if (ret)
		return ret;

	if (status & STM32_I2C_ISR_BERR) {
		dev_dbg(dev, "Bus error\n");

		/* Clear BERR flag */
		setbits_le32(&regs->icr, STM32_I2C_ICR_BERRCF);

		return -EIO;
	}

	if (status & STM32_I2C_ISR_ARLO) {
		dev_dbg(dev, "Arbitration lost\n");

		/* Clear ARLO flag */
		setbits_le32(&regs->icr, STM32_I2C_ICR_ARLOCF);

		return -EAGAIN;
	}

	if (status & STM32_I2C_ISR_NACKF) {
		dev_dbg(dev, "Receive NACK\n");

		/* Clear NACK flag */
		setbits_le32(&regs->icr, STM32_I2C_ICR_NACKCF);

		/* Wait until STOPF flag is set */
		mask = STM32_I2C_ISR_STOPF;
		ret = stm32_i2c_wait_flags(i2c_priv, mask, &status);
		if (ret)
			return ret;

		ret = -EIO;
	}

	if (status & STM32_I2C_ISR_STOPF) {
		/* Clear STOP flag */
		setbits_le32(&regs->icr, STM32_I2C_ICR_STOPCF);

		/* Clear control register 2 */
		clrbits_le32(&regs->cr2, STM32_I2C_CR2_RESET_MASK);
	}

	return ret;
}

static int stm32_i2c_message_xfer(struct stm32_i2c_priv *i2c_priv,
				  struct i2c_msg *msg, bool stop)
{
	struct stm32_i2c_regs *regs = i2c_priv->regs;
	u32 status;
	u32 mask = msg->flags & I2C_M_RD ? STM32_I2C_ISR_RXNE :
		   STM32_I2C_ISR_TXIS | STM32_I2C_ISR_NACKF;
	int bytes_to_rw = msg->len > STM32_I2C_MAX_LEN ?
			  STM32_I2C_MAX_LEN : msg->len;
	int ret = 0;

	/* Add errors */
	mask |= STM32_I2C_ISR_ERRORS;

	stm32_i2c_message_start(i2c_priv, msg);

	while (msg->len) {
		/*
		 * Wait until TXIS/NACKF/BERR/ARLO flags or
		 * RXNE/BERR/ARLO flags are set
		 */
		ret = stm32_i2c_wait_flags(i2c_priv, mask, &status);
		if (ret)
			break;

		if (status & (STM32_I2C_ISR_NACKF | STM32_I2C_ISR_ERRORS))
			break;

		if (status & STM32_I2C_ISR_RXNE) {
			*msg->buf++ = readb(&regs->rxdr);
			msg->len--;
			bytes_to_rw--;
		}

		if (status & STM32_I2C_ISR_TXIS) {
			writeb(*msg->buf++, &regs->txdr);
			msg->len--;
			bytes_to_rw--;
		}

		if (!bytes_to_rw && msg->len) {
			/* Wait until TCR flag is set */
			mask = STM32_I2C_ISR_TCR;
			ret = stm32_i2c_wait_flags(i2c_priv, mask, &status);
			if (ret)
				break;

			bytes_to_rw = msg->len > STM32_I2C_MAX_LEN ?
				      STM32_I2C_MAX_LEN : msg->len;
			mask = msg->flags & I2C_M_RD ? STM32_I2C_ISR_RXNE :
			       STM32_I2C_ISR_TXIS | STM32_I2C_ISR_NACKF;

			stm32_i2c_handle_reload(i2c_priv, msg);
		} else if (!bytes_to_rw) {
			/* Wait until TC flag is set */
			mask = STM32_I2C_ISR_TC;
			ret = stm32_i2c_wait_flags(i2c_priv, mask, &status);
			if (ret)
				break;

			if (!stop)
				/* Message sent, new message has to be sent */
				return 0;
		}
	}

	/* End of transfer, send stop condition if appropriate */
	if (!ret && !(status & (STM32_I2C_ISR_NACKF | STM32_I2C_ISR_ERRORS)))
		setbits_le32(&regs->cr2, STM32_I2C_CR2_STOP);

	return stm32_i2c_check_end_of_message(i2c_priv);
}

static int stm32_i2c_xfer(struct i2c_adapter *adapter,
			  struct i2c_msg *msg, int nmsgs)
{
	struct stm32_i2c_priv *i2c_priv =
		container_of(adapter, struct stm32_i2c_priv, adapter);
	int i, ret;

	ret = stm32_i2c_check_device_busy(i2c_priv);
	if (ret)
		return ret;

	for (i = nmsgs; i > 0; i--, msg++) {
		ret = stm32_i2c_message_xfer(i2c_priv, msg, i == 1);
		if (ret)
			return ret;
	}

	return nmsgs;
}

static int stm32_i2c_compute_solutions(u32 i2cclk,
				       struct stm32_i2c_setup *setup,
				       const struct stm32_i2c_spec *specs,
				       struct list_head *solutions)
{
	struct stm32_i2c_timings *v;
	u32 p_prev = STM32_PRESC_MAX;
	u32 af_delay_min, af_delay_max;
	u16 p, l, a;
	int sdadel_min, sdadel_max, scldel_min;
	int ret = 0;

	af_delay_min = setup->analog_filter ?
		       STM32_I2C_ANALOG_FILTER_DELAY_MIN : 0;
	af_delay_max = setup->analog_filter ?
		       STM32_I2C_ANALOG_FILTER_DELAY_MAX : 0;

	sdadel_min = specs->hddat_min + setup->timings.scl_fall_ns -
		     af_delay_min - (setup->dnf + 3) * i2cclk;

	sdadel_max = specs->vddat_max - setup->timings.scl_rise_ns -
		     af_delay_max - (setup->dnf + 4) * i2cclk;

	scldel_min = setup->timings.scl_rise_ns + specs->sudat_min;

	if (sdadel_min < 0)
		sdadel_min = 0;
	if (sdadel_max < 0)
		sdadel_max = 0;

	pr_debug("SDADEL(min/max): %i/%i, SCLDEL(Min): %i\n",
		 sdadel_min, sdadel_max, scldel_min);

	/* Compute possible values for PRESC, SCLDEL and SDADEL */
	for (p = 0; p < STM32_PRESC_MAX; p++) {
		for (l = 0; l < STM32_SCLDEL_MAX; l++) {
			int scldel = (l + 1) * (p + 1) * i2cclk;

			if (scldel < scldel_min)
				continue;

			for (a = 0; a < STM32_SDADEL_MAX; a++) {
				int sdadel = (a * (p + 1) + 1) * i2cclk;

				if (((sdadel >= sdadel_min) &&
				     (sdadel <= sdadel_max)) &&
				    (p != p_prev)) {
					v = calloc(1, sizeof(*v));
					if (!v)
						return -ENOMEM;

					v->presc = p;
					v->scldel = l;
					v->sdadel = a;
					p_prev = p;

					list_add_tail(&v->node, solutions);
					break;
				}
			}

			if (p_prev == p)
				break;
		}
	}

	if (list_empty(solutions)) {
		pr_err("no Prescaler solution\n");
		ret = -EPERM;
	}

	return ret;
}

static int stm32_i2c_choose_solution(u32 i2cclk,
				     struct stm32_i2c_setup *setup,
				     const struct stm32_i2c_spec *specs,
				     struct list_head *solutions,
				     struct stm32_i2c_timings *s)
{
	struct stm32_i2c_timings *v;
	u32 i2cbus = DIV_ROUND_CLOSEST(NSEC_PER_SEC,
				       setup->timings.bus_freq_hz);
	u32 clk_error_prev = i2cbus;
	u32 clk_min, clk_max;
	u32 af_delay_min;
	u32 dnf_delay;
	u32 tsync;
	u16 l, h;
	bool sol_found = false;
	int ret = 0;

	af_delay_min = setup->analog_filter ?
		       STM32_I2C_ANALOG_FILTER_DELAY_MIN : 0;

	dnf_delay = setup->dnf * i2cclk;

	tsync = af_delay_min + dnf_delay + (2 * i2cclk);
	clk_max = NSEC_PER_SEC / specs->rate_min;
	clk_min = NSEC_PER_SEC / specs->rate_max;

	/*
	 * Among Prescaler possibilities discovered above figures out SCL Low
	 * and High Period. Provided:
	 * - SCL Low Period has to be higher than Low Period of the SCL Clock
	 *   defined by I2C Specification. I2C Clock has to be lower than
	 *   (SCL Low Period - Analog/Digital filters) / 4.
	 * - SCL High Period has to be lower than High Period of the SCL Clock
	 *   defined by I2C Specification
	 * - I2C Clock has to be lower than SCL High Period
	 */
	list_for_each_entry(v, solutions, node) {
		u32 prescaler = (v->presc + 1) * i2cclk;

		for (l = 0; l < STM32_SCLL_MAX; l++) {
			u32 tscl_l = (l + 1) * prescaler + tsync;

			if (tscl_l < specs->l_min ||
			    (i2cclk >=
			     ((tscl_l - af_delay_min - dnf_delay) / 4))) {
				continue;
			}

			for (h = 0; h < STM32_SCLH_MAX; h++) {
				u32 tscl_h = (h + 1) * prescaler + tsync;
				u32 tscl = tscl_l + tscl_h +
					   setup->timings.scl_rise_ns + setup->timings.scl_fall_ns;

				if ((tscl >= clk_min) && (tscl <= clk_max) &&
				    (tscl_h >= specs->h_min) &&
				    (i2cclk < tscl_h)) {
					u32 clk_error;

					if (tscl > i2cbus)
						clk_error = tscl - i2cbus;
					else
						clk_error = i2cbus - tscl;

					if (clk_error < clk_error_prev) {
						clk_error_prev = clk_error;
						v->scll = l;
						v->sclh = h;
						sol_found = true;
						memcpy(s, v, sizeof(*s));
					}
				}
			}
		}
	}

	if (!sol_found) {
		pr_err("no solution at all\n");
		ret = -EPERM;
	}

	return ret;
}

static const struct stm32_i2c_spec *get_specs(u32 rate)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(i2c_specs); i++)
		if (rate <= i2c_specs[i].rate)
			return &i2c_specs[i];

	/* NOT REACHED */
	return ERR_PTR(-EINVAL);
}

static int stm32_i2c_compute_timing(struct stm32_i2c_priv *i2c_priv,
				    struct stm32_i2c_timings *output)
{
	struct device *dev = &i2c_priv->adapter.dev;
	struct stm32_i2c_setup *setup = &i2c_priv->setup;
	const struct stm32_i2c_spec *specs;
	struct stm32_i2c_timings *v, *_v;
	struct list_head solutions;
	u32 i2cclk = DIV_ROUND_CLOSEST(NSEC_PER_SEC, setup->clock_src);
	int ret;

	specs = get_specs(setup->timings.bus_freq_hz);
	if (specs == ERR_PTR(-EINVAL)) {
		dev_err(dev, "speed out of bound {%d}\n",
			setup->timings.bus_freq_hz);
		return -EINVAL;
	}

	if (setup->timings.scl_rise_ns > specs->rise_max ||
	    setup->timings.scl_fall_ns > specs->fall_max) {
		dev_err(dev, "timings out of bound Rise{%d>%d}/Fall{%d>%d}\n",
			setup->timings.scl_rise_ns, specs->rise_max,
			setup->timings.scl_fall_ns, specs->fall_max);
		return -EINVAL;
	}

	/*  Analog and Digital Filters */
	setup->dnf = DIV_ROUND_CLOSEST(i2c_priv->setup.timings.digital_filter_width_ns, i2cclk);
	if (setup->dnf > STM32_I2C_DNF_MAX) {
		dev_err(dev, "DNF out of bound %d/%d\n",
			setup->dnf, STM32_I2C_DNF_MAX);
		return -EINVAL;
	}

	INIT_LIST_HEAD(&solutions);
	ret = stm32_i2c_compute_solutions(i2cclk, setup, specs, &solutions);
	if (ret)
		goto exit;

	ret = stm32_i2c_choose_solution(i2cclk, setup, specs, &solutions, output);
	if (ret)
		goto exit;

	dev_dbg(dev, "Presc: %i, scldel: %i, sdadel: %i, scll: %i, sclh: %i\n",
		output->presc,
		output->scldel, output->sdadel,
		output->scll, output->sclh);

exit:
	/* Release list and memory */
	list_for_each_entry_safe(v, _v, &solutions, node) {
		list_del(&v->node);
		free(v);
	}

	return ret;
}

static u32 get_lower_rate(u32 rate)
{
	int i;

	for (i = ARRAY_SIZE(i2c_specs) - 1; i >= 0; i--)
		if (rate > i2c_specs[i].rate)
			return i2c_specs[i].rate;

	return i2c_specs[0].rate;
}

static int stm32_i2c_setup_timing(struct stm32_i2c_priv *i2c_priv,
				  struct stm32_i2c_timings *timing)
{
	struct device *dev = &i2c_priv->adapter.dev;
	struct stm32_i2c_setup *setup = &i2c_priv->setup;
	int ret = 0;

	setup->timings.bus_freq_hz = i2c_priv->speed;
	setup->clock_src = clk_get_rate(i2c_priv->clk);

	if (!setup->clock_src) {
		dev_err(dev, "clock rate is 0\n");
		return -EINVAL;
	}

	do {
		ret = stm32_i2c_compute_timing(i2c_priv, timing);
		if (ret) {
			dev_dbg(dev, "failed to compute I2C timings.\n");
			if (setup->timings.bus_freq_hz > STANDARD_RATE) {
				setup->timings.bus_freq_hz =
					get_lower_rate(setup->timings.bus_freq_hz);
				dev_dbg(dev, "downgrade I2C Speed Freq to (%i)\n",
					setup->timings.bus_freq_hz);
			} else {
				break;
			}
		}
	} while (ret);

	if (ret) {
		dev_err(dev, "impossible to compute I2C timings.\n");
		return ret;
	}

	dev_dbg(dev, "I2C Freq(%i), Clk Source(%i)\n",
		setup->timings.bus_freq_hz, setup->clock_src);
	dev_dbg(dev, "I2C Rise(%i) and Fall(%i) Time\n",
		setup->timings.scl_rise_ns, setup->timings.scl_fall_ns);
	dev_dbg(dev, "I2C Analog Filter(%s), DNF(%i)\n",
		setup->analog_filter ? "On" : "Off", setup->dnf);

	i2c_priv->speed = setup->timings.bus_freq_hz;

	return 0;
}

static int stm32_i2c_write_fm_plus_bits(struct stm32_i2c_priv *i2c_priv)
{
	int ret;
	bool enable = i2c_priv->speed > FAST_RATE;

	/* Optional */
	if (IS_ERR_OR_NULL(i2c_priv->regmap))
		return 0;

	if (i2c_priv->regmap_sreg == i2c_priv->regmap_creg)
		ret = regmap_update_bits(i2c_priv->regmap,
					 i2c_priv->regmap_sreg,
					 i2c_priv->regmap_mask,
					 enable ? i2c_priv->regmap_mask : 0);
	else
		ret = regmap_write(i2c_priv->regmap,
				   enable ? i2c_priv->regmap_sreg :
					    i2c_priv->regmap_creg,
				   i2c_priv->regmap_mask);

	return ret;
}

static int stm32_i2c_hw_config(struct stm32_i2c_priv *i2c_priv)
{
	struct stm32_i2c_regs *regs = i2c_priv->regs;
	struct stm32_i2c_timings t;
	int ret;
	u32 timing = 0;

	ret = stm32_i2c_setup_timing(i2c_priv, &t);
	if (ret)
		return ret;

	/* Disable I2C */
	clrbits_le32(&regs->cr1, STM32_I2C_CR1_PE);

	/* Setup Fast mode plus if necessary */
	ret = stm32_i2c_write_fm_plus_bits(i2c_priv);
	if (ret)
		return ret;

	/* Timing settings */
	timing |= STM32_I2C_TIMINGR_PRESC(t.presc);
	timing |= STM32_I2C_TIMINGR_SCLDEL(t.scldel);
	timing |= STM32_I2C_TIMINGR_SDADEL(t.sdadel);
	timing |= STM32_I2C_TIMINGR_SCLH(t.sclh);
	timing |= STM32_I2C_TIMINGR_SCLL(t.scll);
	writel(timing, &regs->timingr);

	/* Enable I2C */
	if (i2c_priv->setup.analog_filter)
		clrbits_le32(&regs->cr1, STM32_I2C_CR1_ANFOFF);
	else
		setbits_le32(&regs->cr1, STM32_I2C_CR1_ANFOFF);

	/* Program the Digital Filter */
	clrsetbits_le32(&regs->cr1, STM32_I2C_CR1_DNF_MASK,
			STM32_I2C_CR1_DNF(i2c_priv->setup.dnf));

	setbits_le32(&regs->cr1, STM32_I2C_CR1_PE);

	return 0;
}

static int stm32_i2c_set_bus_speed(struct stm32_i2c_priv *i2c_priv, unsigned int speed)
{
	struct device *dev = &i2c_priv->adapter.dev;

	if (speed > FAST_PLUS_RATE) {
		dev_dbg(dev, "Speed %d not supported\n", speed);
		return -EINVAL;
	}

	i2c_priv->speed = speed;

	return stm32_i2c_hw_config(i2c_priv);
}

static int stm32_of_to_plat(struct device *dev, struct stm32_i2c_priv *i2c_priv)
{
	const struct stm32_i2c_data *data;
	int ret;

	data = device_get_match_data(dev);
	if (!data)
		return -ENODEV;

	if (of_property_read_u32(dev->of_node, "i2c-digital-filter-width-ns",
				 &i2c_priv->setup.timings.digital_filter_width_ns))
		i2c_priv->setup.timings.digital_filter_width_ns = 0;
	if (!of_property_read_bool(dev->of_node, "i2c-digital-filter"))
		i2c_priv->setup.timings.digital_filter_width_ns = 0;

	i2c_priv->setup.analog_filter =
		of_property_read_bool(dev->of_node, "i2c-analog-filter");

	/* Optional */
	i2c_priv->regmap = syscon_regmap_lookup_by_phandle(dev->of_node,
							   "st,syscfg-fmp");
	if (!IS_ERR(i2c_priv->regmap)) {
		u32 fmp[3];

		ret = of_property_read_u32_array(dev->of_node, "st,syscfg-fmp", fmp, 3);
		if (ret)
			return ret;

		i2c_priv->regmap_sreg = fmp[1];
		i2c_priv->regmap_creg = fmp[1] + data->fmp_clr_offset;
		i2c_priv->regmap_mask = fmp[2];
	}

	return 0;
}

static int __init stm32_i2c_probe(struct device *dev)
{
	struct resource *iores;
	struct stm32_i2c_priv *stm32_i2c;
	struct i2c_platform_data *pdata;
	struct i2c_timings *timings;
	int ret;

	stm32_i2c = xzalloc(sizeof(*stm32_i2c));

	stm32_i2c->clk = clk_get(dev, NULL);
	if (IS_ERR(stm32_i2c->clk))
		return PTR_ERR(stm32_i2c->clk);
	clk_enable(stm32_i2c->clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	stm32_i2c->regs = IOMEM(iores->start);

	ret = device_reset_us(dev, 2);
	if (ret)
		return ret;

	ret = stm32_of_to_plat(dev, stm32_i2c);
	if (ret)
		return ret;

	timings = &stm32_i2c->setup.timings;

	/* We've our own defaults, so don't use the i2c_parse_fw_timings ones */
	i2c_parse_fw_timings(dev, timings, false);

	if (!timings->bus_freq_hz)
		timings->bus_freq_hz = STANDARD_RATE;
	if (!timings->scl_rise_ns)
		timings->scl_rise_ns = STM32_I2C_RISE_TIME_DEFAULT;
	if (!timings->scl_fall_ns)
		timings->scl_fall_ns = STM32_I2C_FALL_TIME_DEFAULT;

	/* Setup stm32_i2c driver structure */
	stm32_i2c->adapter.master_xfer = stm32_i2c_xfer;
	stm32_i2c->adapter.nr = -1;
	stm32_i2c->adapter.dev.parent = dev;
	stm32_i2c->adapter.dev.of_node = dev->of_node;

	pdata = dev->platform_data;
	if (pdata && pdata->bitrate)
		timings->bus_freq_hz = pdata->bitrate;

	ret = stm32_i2c_set_bus_speed(stm32_i2c, timings->bus_freq_hz);
	if (ret)
		return ret;

	return i2c_add_numbered_adapter(&stm32_i2c->adapter);
}

static __maybe_unused struct of_device_id stm32_i2c_dt_ids[] = {
	{ .compatible = "st,stm32f7-i2c", .data = &stm32f7_data },
	{ .compatible = "st,stm32mp15-i2c", .data = &stm32mp15_data },
	{ .compatible = "st,stm32mp13-i2c", .data = &stm32mp13_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stm32_i2c_dt_ids);

static struct driver stm32_i2c_driver = {
	.probe	= stm32_i2c_probe,
	.name	= "stm32f7-i2c",
	.of_compatible = DRV_OF_COMPAT(stm32_i2c_dt_ids),
};
coredevice_platform_driver(stm32_i2c_driver);

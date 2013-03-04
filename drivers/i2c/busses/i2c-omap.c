/*
 * TI OMAP I2C master mode driver
 *
 * Copyright (C) 2003 MontaVista Software, Inc.
 * Copyright (C) 2005 Nokia Corporation
 * Copyright (C) 2004 - 2007 Texas Instruments.
 *
 * Originally written by MontaVista Software, Inc.
 * Additional contributions by:
 *	Tony Lindgren <tony@atomide.com>
 *	Imre Deak <imre.deak@nokia.com>
 *	Juha Yrjölä <juha.yrjola@solidboot.com>
 *	Syed Khasim <x0khasim@ti.com>
 *	Nishant Menon <nm@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


/* #include <linux/delay.h> */


#include <clock.h>
#include <common.h>
#include <driver.h>
#include <gpio.h>
#include <init.h>
#include <malloc.h>
#include <types.h>
#include <xfuncs.h>

#include <linux/err.h>

#include <io.h>
#include <i2c/i2c.h>
#include <mach/generic.h>
#include <mach/omap3-clock.h>

#define OMAP_I2C_SIZE		0x3f
#define OMAP1_I2C_BASE		0xfffb3800
#define OMAP2_I2C_BASE1		0x48070000
#define OMAP2_I2C_BASE2		0x48072000
#define OMAP2_I2C_BASE3		0x48060000

/* This will be the driver name */
#define DRIVER_NAME "i2c-omap"

/* I2C Interrupt Enable Register (OMAP_I2C_IE): */
#define OMAP_I2C_IE_XDR		(1 << 14)	/* TX Buffer drain int enable */
#define OMAP_I2C_IE_RDR		(1 << 13)	/* RX Buffer drain int enable */
#define OMAP_I2C_IE_XRDY	(1 << 4)	/* TX data ready int enable */
#define OMAP_I2C_IE_RRDY	(1 << 3)	/* RX data ready int enable */
#define OMAP_I2C_IE_ARDY	(1 << 2)	/* Access ready int enable */
#define OMAP_I2C_IE_NACK	(1 << 1)	/* No ack interrupt enable */
#define OMAP_I2C_IE_AL		(1 << 0)	/* Arbitration lost int ena */

/* I2C Status Register (OMAP_I2C_STAT): */
#define OMAP_I2C_STAT_XDR	(1 << 14)	/* TX Buffer draining */
#define OMAP_I2C_STAT_RDR	(1 << 13)	/* RX Buffer draining */
#define OMAP_I2C_STAT_BB	(1 << 12)	/* Bus busy */
#define OMAP_I2C_STAT_ROVR	(1 << 11)	/* Receive overrun */
#define OMAP_I2C_STAT_XUDF	(1 << 10)	/* Transmit underflow */
#define OMAP_I2C_STAT_AAS	(1 << 9)	/* Address as slave */
#define OMAP_I2C_STAT_AD0	(1 << 8)	/* Address zero */
#define OMAP_I2C_STAT_XRDY	(1 << 4)	/* Transmit data ready */
#define OMAP_I2C_STAT_RRDY	(1 << 3)	/* Receive data ready */
#define OMAP_I2C_STAT_ARDY	(1 << 2)	/* Register access ready */
#define OMAP_I2C_STAT_NACK	(1 << 1)	/* No ack interrupt enable */
#define OMAP_I2C_STAT_AL	(1 << 0)	/* Arbitration lost int ena */

/* I2C WE wakeup enable register */
#define OMAP_I2C_WE_XDR_WE	(1 << 14)	/* TX drain wakup */
#define OMAP_I2C_WE_RDR_WE	(1 << 13)	/* RX drain wakeup */
#define OMAP_I2C_WE_AAS_WE	(1 << 9)	/* Address as slave wakeup*/
#define OMAP_I2C_WE_BF_WE	(1 << 8)	/* Bus free wakeup */
#define OMAP_I2C_WE_STC_WE	(1 << 6)	/* Start condition wakeup */
#define OMAP_I2C_WE_GC_WE	(1 << 5)	/* General call wakeup */
#define OMAP_I2C_WE_DRDY_WE	(1 << 3)	/* TX/RX data ready wakeup */
#define OMAP_I2C_WE_ARDY_WE	(1 << 2)	/* Reg access ready wakeup */
#define OMAP_I2C_WE_NACK_WE	(1 << 1)	/* No acknowledgment wakeup */
#define OMAP_I2C_WE_AL_WE	(1 << 0)	/* Arbitration lost wakeup */

#define OMAP_I2C_WE_ALL		(OMAP_I2C_WE_XDR_WE | OMAP_I2C_WE_RDR_WE | \
				OMAP_I2C_WE_AAS_WE | OMAP_I2C_WE_BF_WE | \
				OMAP_I2C_WE_STC_WE | OMAP_I2C_WE_GC_WE | \
				OMAP_I2C_WE_DRDY_WE | OMAP_I2C_WE_ARDY_WE | \
				OMAP_I2C_WE_NACK_WE | OMAP_I2C_WE_AL_WE)

/* I2C Buffer Configuration Register (OMAP_I2C_BUF): */
#define OMAP_I2C_BUF_RDMA_EN	(1 << 15)	/* RX DMA channel enable */
#define OMAP_I2C_BUF_RXFIF_CLR	(1 << 14)	/* RX FIFO Clear */
#define OMAP_I2C_BUF_XDMA_EN	(1 << 7)	/* TX DMA channel enable */
#define OMAP_I2C_BUF_TXFIF_CLR	(1 << 6)	/* TX FIFO Clear */

/* I2C Configuration Register (OMAP_I2C_CON): */
#define OMAP_I2C_CON_EN		(1 << 15)	/* I2C module enable */
#define OMAP_I2C_CON_BE		(1 << 14)	/* Big endian mode */
#define OMAP_I2C_CON_OPMODE_HS	(1 << 12)	/* High Speed support */
#define OMAP_I2C_CON_STB	(1 << 11)	/* Start byte mode (master) */
#define OMAP_I2C_CON_MST	(1 << 10)	/* Master/slave mode */
#define OMAP_I2C_CON_TRX	(1 << 9)	/* TX/RX mode (master only) */
#define OMAP_I2C_CON_XA		(1 << 8)	/* Expand address */
#define OMAP_I2C_CON_RM		(1 << 2)	/* Repeat mode (master only) */
#define OMAP_I2C_CON_STP	(1 << 1)	/* Stop cond (master only) */
#define OMAP_I2C_CON_STT	(1 << 0)	/* Start condition (master) */

/* I2C SCL time value when Master */
#define OMAP_I2C_SCLL_HSSCLL	8
#define OMAP_I2C_SCLH_HSSCLH	8

/* I2C System Test Register (OMAP_I2C_SYSTEST): */
#ifdef DEBUG
#define OMAP_I2C_SYSTEST_ST_EN		(1 << 15)	/* System test enable */
#define OMAP_I2C_SYSTEST_FREE		(1 << 14)	/* Free running mode */
#define OMAP_I2C_SYSTEST_TMODE_MASK	(3 << 12)	/* Test mode select */
#define OMAP_I2C_SYSTEST_TMODE_SHIFT	(12)		/* Test mode select */
#define OMAP_I2C_SYSTEST_SCL_I		(1 << 3)	/* SCL line sense in */
#define OMAP_I2C_SYSTEST_SCL_O		(1 << 2)	/* SCL line drive out */
#define OMAP_I2C_SYSTEST_SDA_I		(1 << 1)	/* SDA line sense in */
#define OMAP_I2C_SYSTEST_SDA_O		(1 << 0)	/* SDA line drive out */
#endif

/* OCP_SYSSTATUS bit definitions */
#define SYSS_RESETDONE_MASK		(1 << 0)

/* OCP_SYSCONFIG bit definitions */
#define SYSC_CLOCKACTIVITY_MASK		(0x3 << 8)
#define SYSC_SIDLEMODE_MASK		(0x3 << 3)
#define SYSC_ENAWAKEUP_MASK		(1 << 2)
#define SYSC_SOFTRESET_MASK		(1 << 1)
#define SYSC_AUTOIDLE_MASK		(1 << 0)

#define SYSC_IDLEMODE_SMART		0x2
#define SYSC_CLOCKACTIVITY_FCLK		0x2


struct omap_i2c_struct {
	void 			*base;
	u8			*regs;
	u8			reg_shift;
	struct resource		*ioarea;
	u32			speed;		/* Speed of bus in Khz */
	u16			cmd_err;
	u8			*buf;
	size_t			buf_len;
	struct i2c_adapter	adapter;
	u8			fifo_size;	/* use as flag and value
						 * fifo_size==0 implies no fifo
						 * if set, should be trsh+1
						 */
	u8			rev;
	unsigned		b_hw:1;		/* bad h/w fixes */
	u16			iestate;	/* Saved interrupt register */
	u16			pscstate;
	u16			scllstate;
	u16			sclhstate;
	u16			bufstate;
	u16			syscstate;
	u16			westate;
};
#define to_omap_i2c_struct(a)	container_of(a, struct omap_i2c_struct, adapter)

enum {
	OMAP_I2C_REV_REG = 0,
	OMAP_I2C_IE_REG,
	OMAP_I2C_STAT_REG,
	OMAP_I2C_IV_REG,
	OMAP_I2C_WE_REG,
	OMAP_I2C_SYSS_REG,
	OMAP_I2C_BUF_REG,
	OMAP_I2C_CNT_REG,
	OMAP_I2C_DATA_REG,
	OMAP_I2C_SYSC_REG,
	OMAP_I2C_CON_REG,
	OMAP_I2C_OA_REG,
	OMAP_I2C_SA_REG,
	OMAP_I2C_PSC_REG,
	OMAP_I2C_SCLL_REG,
	OMAP_I2C_SCLH_REG,
	OMAP_I2C_SYSTEST_REG,
	OMAP_I2C_BUFSTAT_REG,
	OMAP_I2C_REVNB_LO,
	OMAP_I2C_REVNB_HI,
	OMAP_I2C_IRQSTATUS_RAW,
	OMAP_I2C_IRQENABLE_SET,
	OMAP_I2C_IRQENABLE_CLR,
};

static const u8 reg_map[] = {
	[OMAP_I2C_REV_REG] = 0x00,
	[OMAP_I2C_IE_REG] = 0x01,
	[OMAP_I2C_STAT_REG] = 0x02,
	[OMAP_I2C_IV_REG] = 0x03,
	[OMAP_I2C_WE_REG] = 0x03,
	[OMAP_I2C_SYSS_REG] = 0x04,
	[OMAP_I2C_BUF_REG] = 0x05,
	[OMAP_I2C_CNT_REG] = 0x06,
	[OMAP_I2C_DATA_REG] = 0x07,
	[OMAP_I2C_SYSC_REG] = 0x08,
	[OMAP_I2C_CON_REG] = 0x09,
	[OMAP_I2C_OA_REG] = 0x0a,
	[OMAP_I2C_SA_REG] = 0x0b,
	[OMAP_I2C_PSC_REG] = 0x0c,
	[OMAP_I2C_SCLL_REG] = 0x0d,
	[OMAP_I2C_SCLH_REG] = 0x0e,
	[OMAP_I2C_SYSTEST_REG] = 0x0f,
	[OMAP_I2C_BUFSTAT_REG] = 0x10,
};

static const u8 omap4_reg_map[] = {
	[OMAP_I2C_REV_REG] = 0x04,
	[OMAP_I2C_IE_REG] = 0x2c,
	[OMAP_I2C_STAT_REG] = 0x28,
	[OMAP_I2C_IV_REG] = 0x34,
	[OMAP_I2C_WE_REG] = 0x34,
	[OMAP_I2C_SYSS_REG] = 0x90,
	[OMAP_I2C_BUF_REG] = 0x94,
	[OMAP_I2C_CNT_REG] = 0x98,
	[OMAP_I2C_DATA_REG] = 0x9c,
	[OMAP_I2C_SYSC_REG] = 0x10,
	[OMAP_I2C_CON_REG] = 0xa4,
	[OMAP_I2C_OA_REG] = 0xa8,
	[OMAP_I2C_SA_REG] = 0xac,
	[OMAP_I2C_PSC_REG] = 0xb0,
	[OMAP_I2C_SCLL_REG] = 0xb4,
	[OMAP_I2C_SCLH_REG] = 0xb8,
	[OMAP_I2C_SYSTEST_REG] = 0xbc,
	[OMAP_I2C_BUFSTAT_REG] = 0xc0,
	[OMAP_I2C_REVNB_LO] = 0x00,
	[OMAP_I2C_REVNB_HI] = 0x04,
	[OMAP_I2C_IRQSTATUS_RAW] = 0x24,
	[OMAP_I2C_IRQENABLE_SET] = 0x2c,
	[OMAP_I2C_IRQENABLE_CLR] = 0x30,
};

static inline void omap_i2c_write_reg(struct omap_i2c_struct *i2c_omap,
				      int reg, u16 val)
{
	__raw_writew(val, i2c_omap->base +
			(i2c_omap->regs[reg] << i2c_omap->reg_shift));
}

static inline u16 omap_i2c_read_reg(struct omap_i2c_struct *i2c_omap, int reg)
{
	return __raw_readw(i2c_omap->base +
			(i2c_omap->regs[reg] << i2c_omap->reg_shift));
}

static void omap_i2c_unidle(struct omap_i2c_struct *i2c_omap)
{
	if (cpu_is_omap34xx()) {
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, 0);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_PSC_REG, i2c_omap->pscstate);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_SCLL_REG, i2c_omap->scllstate);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_SCLH_REG, i2c_omap->sclhstate);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_BUF_REG, i2c_omap->bufstate);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_SYSC_REG, i2c_omap->syscstate);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_WE_REG, i2c_omap->westate);
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);
	}

	/*
	 * Don't write to this register if the IE state is 0 as it can
	 * cause deadlock.
	 */
	if (i2c_omap->iestate)
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_IE_REG, i2c_omap->iestate);
}

static void omap_i2c_idle(struct omap_i2c_struct *i2c_omap)
{
	u16 iv;

	i2c_omap->iestate = omap_i2c_read_reg(i2c_omap, OMAP_I2C_IE_REG);

	/* Barebox driver don't need to clear interrupts here */

	/* omap_i2c_write_reg(i2c_omap, OMAP_I2C_IE_REG, 0); */
	if (i2c_omap->rev < OMAP_I2C_REV_2) {
		iv = omap_i2c_read_reg(i2c_omap, OMAP_I2C_IV_REG); /* Read clears */
	} else {
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_STAT_REG, i2c_omap->iestate);

		/* Flush posted write before the i2c_omap->idle store occurs */
		omap_i2c_read_reg(i2c_omap, OMAP_I2C_STAT_REG);
	}
}

static int omap_i2c_init(struct omap_i2c_struct *i2c_omap)
{
	u16 psc = 0, scll = 0, sclh = 0, buf = 0;
	u16 fsscll = 0, fssclh = 0, hsscll = 0, hssclh = 0;
	uint64_t start;

	unsigned long fclk_rate = 12000000;
	unsigned long internal_clk = 0;

	if (i2c_omap->rev >= OMAP_I2C_REV_2) {
		/* Disable I2C controller before soft reset */
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG,
			omap_i2c_read_reg(i2c_omap, OMAP_I2C_CON_REG) &
				~(OMAP_I2C_CON_EN));

		omap_i2c_write_reg(i2c_omap, OMAP_I2C_SYSC_REG, SYSC_SOFTRESET_MASK);
		/* For some reason we need to set the EN bit before the
		 * reset done bit gets set. */
		start = get_time_ns();

		omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);
		while (!(omap_i2c_read_reg(i2c_omap, OMAP_I2C_SYSS_REG) &
			 SYSS_RESETDONE_MASK)) {
			if (is_timeout(start, MSECOND)) {
				dev_warn(&i2c_omap->adapter.dev, "timeout waiting "
						"for controller reset\n");
				return -ETIMEDOUT;
			}
			mdelay(1);
		}

		/* SYSC register is cleared by the reset; rewrite it */
		if (i2c_omap->rev == OMAP_I2C_REV_ON_2430) {

			omap_i2c_write_reg(i2c_omap, OMAP_I2C_SYSC_REG,
					   SYSC_AUTOIDLE_MASK);

		} else if (i2c_omap->rev >= OMAP_I2C_REV_ON_3430) {
			i2c_omap->syscstate = SYSC_AUTOIDLE_MASK;
			i2c_omap->syscstate |= SYSC_ENAWAKEUP_MASK;
			i2c_omap->syscstate |= (SYSC_IDLEMODE_SMART <<
			      __ffs(SYSC_SIDLEMODE_MASK));
			i2c_omap->syscstate |= (SYSC_CLOCKACTIVITY_FCLK <<
			      __ffs(SYSC_CLOCKACTIVITY_MASK));

			omap_i2c_write_reg(i2c_omap, OMAP_I2C_SYSC_REG,
							i2c_omap->syscstate);
			/*
			 * Enabling all wakup sources to stop I2C freezing on
			 * WFI instruction.
			 * REVISIT: Some wkup sources might not be needed.
			 */
			i2c_omap->westate = OMAP_I2C_WE_ALL;
			omap_i2c_write_reg(i2c_omap, OMAP_I2C_WE_REG, i2c_omap->westate);
		}
	}
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, 0);

	/* omap1 handling is missing here */

	if (cpu_is_omap2430() || cpu_is_omap34xx() || cpu_is_omap4xxx()) {

		/*
		 * HSI2C controller internal clk rate should be 19.2 Mhz for
		 * HS and for all modes on 2430. On 34xx we can use lower rate
		 * to get longer filter period for better noise suppression.
		 * The filter is iclk (fclk for HS) period.
		 */
		if (i2c_omap->speed > 400 || cpu_is_omap2430())
			internal_clk = 19200;
		else if (i2c_omap->speed > 100)
			internal_clk = 9600;
		else
			internal_clk = 4000;
		fclk_rate = 96000000 / 1000;

		/* Compute prescaler divisor */
		psc = fclk_rate / internal_clk;
		psc = psc - 1;

		/* If configured for High Speed */
		if (i2c_omap->speed > 400) {
			unsigned long scl;

			/* For first phase of HS mode */
			scl = internal_clk / 400;
			fsscll = scl - (scl / 3) - 7;
			fssclh = (scl / 3) - 5;

			/* For second phase of HS mode */
			scl = fclk_rate / i2c_omap->speed;
			hsscll = scl - (scl / 3) - 7;
			hssclh = (scl / 3) - 5;
		} else if (i2c_omap->speed > 100) {
			unsigned long scl;

			/* Fast mode */
			scl = internal_clk / i2c_omap->speed;
			fsscll = scl - (scl / 3) - 7;
			fssclh = (scl / 3) - 5;
		} else {
			/* Standard mode */
			fsscll = internal_clk / (i2c_omap->speed * 2) - 7;
			fssclh = internal_clk / (i2c_omap->speed * 2) - 5;
		}
		scll = (hsscll << OMAP_I2C_SCLL_HSSCLL) | fsscll;
		sclh = (hssclh << OMAP_I2C_SCLH_HSSCLH) | fssclh;
	} else {
		/* Program desired operating rate */
		fclk_rate /= (psc + 1) * 1000;
		if (psc > 2)
			psc = 2;
		scll = fclk_rate / (i2c_omap->speed * 2) - 7 + psc;
		sclh = fclk_rate / (i2c_omap->speed * 2) - 7 + psc;
	}

	/* Setup clock prescaler to obtain approx 12MHz I2C module clock: */
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_PSC_REG, psc);

	/* SCL low and high time values */
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_SCLL_REG, scll);
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_SCLH_REG, sclh);

	if (i2c_omap->fifo_size) {
		/* Note: setup required fifo size - 1. RTRSH and XTRSH */
		buf = (i2c_omap->fifo_size - 1) << 8 | OMAP_I2C_BUF_RXFIF_CLR |
			(i2c_omap->fifo_size - 1) | OMAP_I2C_BUF_TXFIF_CLR;
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_BUF_REG, buf);
	}

	/* Take the I2C module out of reset: */
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);

	/* Enable interrupts */
	i2c_omap->iestate = (OMAP_I2C_IE_XRDY | OMAP_I2C_IE_RRDY |
			OMAP_I2C_IE_ARDY | OMAP_I2C_IE_NACK |
			OMAP_I2C_IE_AL)  | ((i2c_omap->fifo_size) ?
				(OMAP_I2C_IE_RDR | OMAP_I2C_IE_XDR) : 0);
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_IE_REG, i2c_omap->iestate);
	if (cpu_is_omap34xx()) {
		i2c_omap->pscstate = psc;
		i2c_omap->scllstate = scll;
		i2c_omap->sclhstate = sclh;
		i2c_omap->bufstate = buf;
	}
	return 0;
}

/*
 * Waiting on Bus Busy
 */
static int omap_i2c_wait_for_bb(struct i2c_adapter *adapter)
{
	uint64_t start;
	struct omap_i2c_struct *i2c_omap = to_omap_i2c_struct(adapter);

	start = get_time_ns();
	while (omap_i2c_read_reg(i2c_omap, OMAP_I2C_STAT_REG) & OMAP_I2C_STAT_BB) {
		if (is_timeout(start, MSECOND)) {
			dev_warn(&adapter->dev, "timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static inline void
omap_i2c_ack_stat(struct omap_i2c_struct *i2c_omap, u16 stat)
{
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_STAT_REG, stat);
}

static int
omap_i2c_isr(struct omap_i2c_struct *dev)
{
	u16 bits;
	u16 stat, w;
	int err, count = 0;

	bits = omap_i2c_read_reg(dev, OMAP_I2C_IE_REG);
	while ((stat = (omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG))) & bits) {
		dev_dbg(&dev->adapter.dev, "IRQ (ISR = 0x%04x)\n", stat);
		if (count++ == 100) {
			dev_warn(&dev->adapter.dev, "Too much work in one IRQ\n");
			break;
		}

		err = 0;
complete:
		/*
		 * Ack the stat in one go, but [R/X]DR and [R/X]RDY should be
		 * acked after the data operation is complete.
		 * Ref: TRM SWPU114Q Figure 18-31
		 */
		omap_i2c_write_reg(dev, OMAP_I2C_STAT_REG, stat &
				~(OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR |
				OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR));

		if (stat & OMAP_I2C_STAT_NACK) {
			err |= OMAP_I2C_STAT_NACK;
			omap_i2c_write_reg(dev, OMAP_I2C_CON_REG,
					   OMAP_I2C_CON_STP);
		}
		if (stat & OMAP_I2C_STAT_AL) {
			dev_err(&dev->adapter.dev, "Arbitration lost\n");
			err |= OMAP_I2C_STAT_AL;
		}
		if (stat & (OMAP_I2C_STAT_ARDY | OMAP_I2C_STAT_NACK |
					OMAP_I2C_STAT_AL)) {
			omap_i2c_ack_stat(dev, stat &
				(OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR |
				OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR));
			return 0;
		}
		if (stat & (OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR)) {
			u8 num_bytes = 1;
			if (dev->fifo_size) {
				if (stat & OMAP_I2C_STAT_RRDY)
					num_bytes = dev->fifo_size;
				else    /* read RXSTAT on RDR interrupt */
					num_bytes = (omap_i2c_read_reg(dev,
							OMAP_I2C_BUFSTAT_REG)
							>> 8) & 0x3F;
			}
			while (num_bytes) {
				num_bytes--;
				w = omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
				if (dev->buf_len) {
					*dev->buf++ = w;
					dev->buf_len--;
					/* Data reg from 2430 is 8 bit wide */
					if (!cpu_is_omap2430() &&
							!cpu_is_omap34xx() &&
							!cpu_is_omap4xxx()) {
						if (dev->buf_len) {
							*dev->buf++ = w >> 8;
							dev->buf_len--;
						}
					}
				} else {
					if (stat & OMAP_I2C_STAT_RRDY)
						dev_err(&dev->adapter.dev,
							"RRDY IRQ while no data"
								" requested\n");
					if (stat & OMAP_I2C_STAT_RDR)
						dev_err(&dev->adapter.dev,
							"RDR IRQ while no data"
								" requested\n");
					break;
				}
			}
			omap_i2c_ack_stat(dev,
				stat & (OMAP_I2C_STAT_RRDY | OMAP_I2C_STAT_RDR));
			continue;
		}
		if (stat & (OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR)) {
			u8 num_bytes = 1;
			if (dev->fifo_size) {
				if (stat & OMAP_I2C_STAT_XRDY)
					num_bytes = dev->fifo_size;
				else    /* read TXSTAT on XDR interrupt */
					num_bytes = omap_i2c_read_reg(dev,
							OMAP_I2C_BUFSTAT_REG)
							& 0x3F;
			}
			while (num_bytes) {
				num_bytes--;
				w = 0;
				if (dev->buf_len) {
					w = *dev->buf++;
					dev->buf_len--;
					/* Data reg from  2430 is 8 bit wide */
					if (!cpu_is_omap2430() &&
							!cpu_is_omap34xx() &&
							!cpu_is_omap4xxx()) {
						if (dev->buf_len) {
							w |= *dev->buf++ << 8;
							dev->buf_len--;
						}
					}
				} else {
					if (stat & OMAP_I2C_STAT_XRDY)
						dev_err(&dev->adapter.dev,
							"XRDY IRQ while no "
							"data to send\n");
					if (stat & OMAP_I2C_STAT_XDR)
						dev_err(&dev->adapter.dev,
							"XDR IRQ while no "
							"data to send\n");
					break;
				}

				/*
				 * OMAP3430 Errata 1.153: When an XRDY/XDR
				 * is hit, wait for XUDF before writing data
				 * to DATA_REG. Otherwise some data bytes can
				 * be lost while transferring them from the
				 * memory to the I2C interface.
				 */

				if (dev->rev <= OMAP_I2C_REV_ON_3430) {
						while (!(stat & OMAP_I2C_STAT_XUDF)) {
							if (stat & (OMAP_I2C_STAT_NACK | OMAP_I2C_STAT_AL)) {
								omap_i2c_ack_stat(dev, stat & (OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR));
								err |= OMAP_I2C_STAT_XUDF;
								goto complete;
							}
							stat = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);
						}
				}

				omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, w);
			}
			omap_i2c_ack_stat(dev,
				stat & (OMAP_I2C_STAT_XRDY | OMAP_I2C_STAT_XDR));
			continue;
		}
		if (stat & OMAP_I2C_STAT_ROVR) {
			dev_err(&dev->adapter.dev, "Receive overrun\n");
			dev->cmd_err |= OMAP_I2C_STAT_ROVR;
		}
		if (stat & OMAP_I2C_STAT_XUDF) {
			dev_err(&dev->adapter.dev, "Transmit underflow\n");
			dev->cmd_err |= OMAP_I2C_STAT_XUDF;
		}
	}

	return -EBUSY;
}


/*
 * Low level master read/write transaction.
 */
static int omap_i2c_xfer_msg(struct i2c_adapter *adapter,
			     struct i2c_msg *msg, int stop)
{
	struct omap_i2c_struct *i2c_omap = to_omap_i2c_struct(adapter);
	uint64_t start;
	u16 con;
	u16 w;
	int ret = 0;


	dev_dbg(&adapter->dev, "addr: 0x%04x, len: %d, flags: 0x%x, stop: %d\n",
		msg->addr, msg->len, msg->flags, stop);

	if (msg->len == 0)
		return -EINVAL;

	omap_i2c_write_reg(i2c_omap, OMAP_I2C_SA_REG, msg->addr);

	/* REVISIT: Could the STB bit of I2C_CON be used with probing? */
	i2c_omap->buf = msg->buf;
	i2c_omap->buf_len = msg->len;

	omap_i2c_write_reg(i2c_omap, OMAP_I2C_CNT_REG, i2c_omap->buf_len);

	/* Clear the FIFO Buffers */
	w = omap_i2c_read_reg(i2c_omap, OMAP_I2C_BUF_REG);
	w |= OMAP_I2C_BUF_RXFIF_CLR | OMAP_I2C_BUF_TXFIF_CLR;
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_BUF_REG, w);

	i2c_omap->cmd_err = 0;

	w = OMAP_I2C_CON_EN | OMAP_I2C_CON_MST | OMAP_I2C_CON_STT;

	/* High speed configuration */
	if (i2c_omap->speed > 400)
		w |= OMAP_I2C_CON_OPMODE_HS;

	if (msg->flags & I2C_M_TEN)
		w |= OMAP_I2C_CON_XA;
	if (!(msg->flags & I2C_M_RD))
		w |= OMAP_I2C_CON_TRX;

	if (!i2c_omap->b_hw && stop)
		w |= OMAP_I2C_CON_STP;

	omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, w);

	/*
	 * Don't write stt and stp together on some hardware.
	 */
	if (i2c_omap->b_hw && stop) {
		start = get_time_ns();
		con = omap_i2c_read_reg(i2c_omap, OMAP_I2C_CON_REG);
		while (con & OMAP_I2C_CON_STT) {
			con = omap_i2c_read_reg(i2c_omap, OMAP_I2C_CON_REG);

			/* Let the user know if i2c is in a bad state */
			if (is_timeout(start, MSECOND)) {
				dev_err(&adapter->dev, "controller timed out "
				"waiting for start condition to finish\n");
				return -ETIMEDOUT;
			}
		}

		w |= OMAP_I2C_CON_STP;
		w &= ~OMAP_I2C_CON_STT;
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, w);
	}

	/*
	 * REVISIT: We should abort the transfer on signals, but the bus goes
	 * into arbitration and we're currently unable to recover from it.
	 */
	start = get_time_ns();
	ret = omap_i2c_isr(i2c_omap);
	while (ret){
		ret = omap_i2c_isr(i2c_omap);
		if (is_timeout(start, MSECOND)) {
				dev_err(&adapter->dev,
				"timed out on polling for "
				"open i2c message handling\n");
				return -ETIMEDOUT;
			}
	}

	i2c_omap->buf_len = 0;
	if (likely(!i2c_omap->cmd_err))
		return 0;

	/* We have an error */
	if (i2c_omap->cmd_err & (OMAP_I2C_STAT_AL | OMAP_I2C_STAT_ROVR |
			    OMAP_I2C_STAT_XUDF)) {
		omap_i2c_init(i2c_omap);
		return -EIO;
	}

	if (i2c_omap->cmd_err & OMAP_I2C_STAT_NACK) {
		if (msg->flags & I2C_M_IGNORE_NAK)
			return 0;
		if (stop) {
			w = omap_i2c_read_reg(i2c_omap, OMAP_I2C_CON_REG);
			w |= OMAP_I2C_CON_STP;
			omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, w);
		}
		return -EREMOTEIO;
	}
	return -EIO;
}


/*
 * Prepare controller for a transaction and call omap_i2c_xfer_msg
 * to do the work during IRQ processing.
 */
static int
omap_i2c_xfer(struct i2c_adapter *adapter, struct i2c_msg msgs[], int num)
{
	struct omap_i2c_struct *i2c_omap = to_omap_i2c_struct(adapter);
	int i;
	int r;

	omap_i2c_unidle(i2c_omap);

	r = omap_i2c_wait_for_bb(adapter);
	if (r < 0)
		goto out;

	for (i = 0; i < num; i++) {
		r = omap_i2c_xfer_msg(adapter, &msgs[i], (i == (num - 1)));
		if (r != 0)
			break;
	}

	if (r == 0)
		r = num;
out:
	omap_i2c_idle(i2c_omap);
	return r;
}

static int __init
i2c_omap_probe(struct device_d *pdev)
{
	struct omap_i2c_struct	*i2c_omap;
	/* struct i2c_platform_data *pdata; */
	int r;
	u32 speed = 0;

	i2c_omap = kzalloc(sizeof(struct omap_i2c_struct), GFP_KERNEL);
	if (!i2c_omap) {
		r = -ENOMEM;
		goto err_free_mem;
	}

	if (cpu_is_omap4xxx()) {
		i2c_omap->regs = (u8 *)omap4_reg_map;
		i2c_omap->reg_shift = 0;
	} else {
		i2c_omap->regs = (u8 *)reg_map;
		i2c_omap->reg_shift = 2;
	}

	if (pdev->platform_data != NULL)
		speed = *(u32 *)pdev->platform_data;
	else
		speed = 100;	/* Defualt speed */

	i2c_omap->speed = speed;
	i2c_omap->base = dev_request_mem_region(pdev, 0);
	printf ("I2C probe\n");
	omap_i2c_unidle(i2c_omap);

	i2c_omap->rev = omap_i2c_read_reg(i2c_omap, OMAP_I2C_REV_REG) & 0xff;
	/* i2c_omap->base = OMAP2_I2C_BASE3; */

	if (cpu_is_omap2430() || cpu_is_omap34xx() || cpu_is_omap4xxx()) {
		u16 s;

		/* Set up the fifo size - Get total size */
		s = (omap_i2c_read_reg(i2c_omap, OMAP_I2C_BUFSTAT_REG) >> 14) & 0x3;
		i2c_omap->fifo_size = 0x8 << s;

		/*
		 * Set up notification threshold as half the total available
		 * size. This is to ensure that we can handle the status on int
		 * call back latencies.
		 */

		i2c_omap->fifo_size = (i2c_omap->fifo_size / 2);

		if (i2c_omap->rev >= OMAP_I2C_REV_ON_4430)
			i2c_omap->b_hw = 0; /* Disable hardware fixes */
		else
			i2c_omap->b_hw = 1; /* Enable hardware fixes */
	}

	/* reset ASAP, clearing any IRQs */
	omap_i2c_init(i2c_omap);

	dev_info(pdev, "bus %d rev%d.%d at %d kHz\n",
		 pdev->id, i2c_omap->rev >> 4, i2c_omap->rev & 0xf, i2c_omap->speed);

	omap_i2c_idle(i2c_omap);

	i2c_omap->adapter.master_xfer	= omap_i2c_xfer,
	i2c_omap->adapter.nr = pdev->id;
	i2c_omap->adapter.dev.parent = pdev;

	/* i2c device drivers may be active on return from add_adapter() */
	r = i2c_add_numbered_adapter(&i2c_omap->adapter);
	if (r) {
		dev_err(pdev, "failure adding adapter\n");
		goto err_unuse_clocks;
	}

	return 0;

err_unuse_clocks:
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, 0);
	omap_i2c_idle(i2c_omap);

err_free_mem:
	kfree(i2c_omap);

	return r;
}

static struct driver_d omap_i2c_driver = {
	.probe		= i2c_omap_probe,
	.name		= DRIVER_NAME,
};
device_platform_driver(omap_i2c_driver);

MODULE_AUTHOR("MontaVista Software, Inc. (and others)");
MODULE_DESCRIPTION("TI OMAP I2C bus adapter");
MODULE_LICENSE("GPL");

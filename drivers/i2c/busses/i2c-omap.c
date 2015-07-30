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

/* Errata definitions */
#define I2C_OMAP_ERRATA_I207		(1 << 0)
#define I2C_OMAP_ERRATA_I462		(1 << 1)

/* i2c driver flags from kernel */
#define OMAP_I2C_FLAG_NO_FIFO			BIT(0)
#define OMAP_I2C_FLAG_16BIT_DATA_REG		BIT(2)
#define OMAP_I2C_FLAG_BUS_SHIFT_NONE		0
#define OMAP_I2C_FLAG_BUS_SHIFT_1		BIT(7)
#define OMAP_I2C_FLAG_BUS_SHIFT_2		BIT(8)
#define OMAP_I2C_FLAG_BUS_SHIFT__SHIFT		7

/* timeout waiting for the controller to respond */
#define OMAP_I2C_TIMEOUT		(1000 * MSECOND)	/* ms */

struct omap_i2c_struct {
	void 			*base;
	u8			reg_shift;
	struct omap_i2c_driver_data	*data;
	struct resource		*ioarea;
	u32			speed;		/* Speed of bus in Khz */
	u16			scheme;
	u16			cmd_err;
	u8			*buf;
	u8			*regs;
	size_t			buf_len;
	struct i2c_adapter	adapter;
	u8			threshold;
	u8			fifo_size;	/* use as flag and value
						 * fifo_size==0 implies no fifo
						 * if set, should be trsh+1
						 */
	u32			rev;
	unsigned		b_hw:1;		/* bad h/w fixes */
	unsigned		receiver:1;	/* true for receiver mode */
	u16			iestate;	/* Saved interrupt register */
	u16			pscstate;
	u16			scllstate;
	u16			sclhstate;
	u16			bufstate;
	u16			syscstate;
	u16			westate;
	u16			errata;
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
	/* only on OMAP4430 */
	OMAP_I2C_IP_V2_REVNB_LO,
	OMAP_I2C_IP_V2_REVNB_HI,
	OMAP_I2C_IP_V2_IRQSTATUS_RAW,
	OMAP_I2C_IP_V2_IRQENABLE_SET,
	OMAP_I2C_IP_V2_IRQENABLE_CLR,
};

static const u8 reg_map_ip_v1[] = {
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

static const u8 reg_map_ip_v2[] = {
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
	[OMAP_I2C_IP_V2_REVNB_LO] = 0x00,
	[OMAP_I2C_IP_V2_REVNB_HI] = 0x04,
	[OMAP_I2C_IP_V2_IRQSTATUS_RAW] = 0x24,
	[OMAP_I2C_IP_V2_IRQENABLE_SET] = 0x2c,
	[OMAP_I2C_IP_V2_IRQENABLE_CLR] = 0x30,
};

struct omap_i2c_driver_data {
	u32		flags;
	u32		fclk_rate;
};

static struct omap_i2c_driver_data omap3_data = {
	.flags =	OMAP_I2C_FLAG_BUS_SHIFT_2,
	.fclk_rate =	96000,
};

static struct omap_i2c_driver_data omap4_data = {
	.flags =	OMAP_I2C_FLAG_BUS_SHIFT_NONE,
	.fclk_rate =	96000,
};

static struct omap_i2c_driver_data am33xx_data = {
	.flags =	OMAP_I2C_FLAG_BUS_SHIFT_NONE,
	.fclk_rate =	48000,
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

static void __omap_i2c_init(struct omap_i2c_struct *dev)
{

	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, 0);

	/* Setup clock prescaler to obtain approx 12MHz I2C module clock: */
	omap_i2c_write_reg(dev, OMAP_I2C_PSC_REG, dev->pscstate);

	/* SCL low and high time values */
	omap_i2c_write_reg(dev, OMAP_I2C_SCLL_REG, dev->scllstate);
	omap_i2c_write_reg(dev, OMAP_I2C_SCLH_REG, dev->sclhstate);
	if (dev->rev >= OMAP_I2C_REV_ON_3430_3530)
		omap_i2c_write_reg(dev, OMAP_I2C_WE_REG, dev->westate);

	/* Take the I2C module out of reset: */
	omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);

	/*
	 * Don't write to this register if the IE state is 0 as it can
	 * cause deadlock.
	 */
	if (dev->iestate)
		omap_i2c_write_reg(dev, OMAP_I2C_IE_REG, dev->iestate);
}

static int omap_i2c_reset(struct omap_i2c_struct *dev)
{
	uint64_t start;
	u16 sysc;

	if (dev->rev >= OMAP_I2C_OMAP1_REV_2) {
		sysc = omap_i2c_read_reg(dev, OMAP_I2C_SYSC_REG);

		/* Disable I2C controller before soft reset */
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG,
			omap_i2c_read_reg(dev, OMAP_I2C_CON_REG) &
				~(OMAP_I2C_CON_EN));

		omap_i2c_write_reg(dev, OMAP_I2C_SYSC_REG, SYSC_SOFTRESET_MASK);
		/* For some reason we need to set the EN bit before the
		 * reset done bit gets set. */
		omap_i2c_write_reg(dev, OMAP_I2C_CON_REG, OMAP_I2C_CON_EN);
		start = get_time_ns();
		while (!(omap_i2c_read_reg(dev, OMAP_I2C_SYSS_REG) &
			 SYSS_RESETDONE_MASK)) {
			if (is_timeout(start, OMAP_I2C_TIMEOUT)) {
				dev_warn(&dev->adapter.dev, "timeout waiting "
						"for controller reset\n");
				return -ETIMEDOUT;
			}
			mdelay(1000);
		}

		/* SYSC register is cleared by the reset; rewrite it */
		omap_i2c_write_reg(dev, OMAP_I2C_SYSC_REG, sysc);

	}
	return 0;
}

static int omap_i2c_init(struct omap_i2c_struct *i2c_omap)
{
	u16 psc = 0, scll = 0, sclh = 0, buf = 0;
	u16 fsscll = 0, fssclh = 0, hsscll = 0, hssclh = 0;
	uint64_t start;
	unsigned long internal_clk = 0;
	struct omap_i2c_driver_data *i2c_data = i2c_omap->data;

	if (i2c_omap->rev >= OMAP_I2C_OMAP1_REV_2) {
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

		} else if (i2c_omap->rev >= OMAP_I2C_REV_ON_3430_3530) {
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

	/*
	 * HSI2C controller internal clk rate should be 19.2 Mhz for
	 * HS and for all modes on 2430. On 34xx we can use lower rate
	 * to get longer filter period for better noise suppression.
	 * The filter is iclk (fclk for HS) period.
	 */
	if (i2c_omap->speed > 400)
		internal_clk = 19200;
	else if (i2c_omap->speed > 100)
		internal_clk = 9600;
	else
		internal_clk = 4000;

	/* Compute prescaler divisor */
	psc = i2c_data->fclk_rate / internal_clk;
	psc = psc - 1;

	/* If configured for High Speed */
	if (i2c_omap->speed > 400) {
		unsigned long scl;

		/* For first phase of HS mode */
		scl = internal_clk / 400;
		fsscll = scl - (scl / 3) - 7;
		fssclh = (scl / 3) - 5;

		/* For second phase of HS mode */
		scl = i2c_data->fclk_rate / i2c_omap->speed;
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

	i2c_omap->pscstate = psc;
	i2c_omap->scllstate = scll;
	i2c_omap->sclhstate = sclh;
	i2c_omap->bufstate = buf;

	__omap_i2c_init(i2c_omap);

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
omap_i2c_complete_cmd(struct omap_i2c_struct *dev, u16 err)
{
	dev->cmd_err |= err;
}

static inline void
omap_i2c_ack_stat(struct omap_i2c_struct *i2c_omap, u16 stat)
{
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_STAT_REG, stat);
}

static int errata_omap3_i462(struct omap_i2c_struct *dev)
{
	unsigned long timeout = 10000;
	u16 stat;

	do {
		stat = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);
		if (stat & OMAP_I2C_STAT_XUDF)
			break;

		if (stat & (OMAP_I2C_STAT_NACK | OMAP_I2C_STAT_AL)) {
			omap_i2c_ack_stat(dev, (OMAP_I2C_STAT_XRDY |
							OMAP_I2C_STAT_XDR));
			if (stat & OMAP_I2C_STAT_NACK) {
				dev->cmd_err |= OMAP_I2C_STAT_NACK;
				omap_i2c_ack_stat(dev, OMAP_I2C_STAT_NACK);
			}

			if (stat & OMAP_I2C_STAT_AL) {
				dev_err(&dev->adapter.dev, "Arbitration lost\n");
				dev->cmd_err |= OMAP_I2C_STAT_AL;
				omap_i2c_ack_stat(dev, OMAP_I2C_STAT_AL);
			}

			return -EIO;
		}
	} while (--timeout);

	if (!timeout) {
		dev_err(&dev->adapter.dev, "timeout waiting on XUDF bit\n");
		return 0;
	}

	return 0;
}

static void omap_i2c_receive_data(struct omap_i2c_struct *dev, u8 num_bytes,
		bool is_rdr)
{
	u16		w;

	while (num_bytes--) {
		w = omap_i2c_read_reg(dev, OMAP_I2C_DATA_REG);
		*dev->buf++ = w;
		dev->buf_len--;

		/*
		 * Data reg in 2430, omap3 and
		 * omap4 is 8 bit wide
		 */
		if (dev->data->flags & OMAP_I2C_FLAG_16BIT_DATA_REG) {
			*dev->buf++ = w >> 8;
			dev->buf_len--;
		}
	}
}

static inline void i2c_omap_errata_i207(struct omap_i2c_struct *dev, u16 stat)
{
	/*
	 * I2C Errata(Errata Nos. OMAP2: 1.67, OMAP3: 1.8)
	 * Not applicable for OMAP4.
	 * Under certain rare conditions, RDR could be set again
	 * when the bus is busy, then ignore the interrupt and
	 * clear the interrupt.
	 */
	if (stat & OMAP_I2C_STAT_RDR) {
		/* Step 1: If RDR is set, clear it */
		omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RDR);

		/* Step 2: */
		if (!(omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG)
						& OMAP_I2C_STAT_BB)) {

			/* Step 3: */
			if (omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG)
						& OMAP_I2C_STAT_RDR) {
				omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RDR);
				dev_dbg(&dev->adapter.dev, "RDR when bus is busy.\n");
			}

		}
	}
}

static int omap_i2c_transmit_data(struct omap_i2c_struct *dev, u8 num_bytes,
		bool is_xdr)
{
	u16		w;

	while (num_bytes--) {
		w = *dev->buf++;
		dev->buf_len--;

		/*
		 * Data reg in 2430, omap3 and
		 * omap4 is 8 bit wide
		 */
		if (dev->data->flags & OMAP_I2C_FLAG_16BIT_DATA_REG) {
			w |= *dev->buf++ << 8;
			dev->buf_len--;
		}

		if (dev->errata & I2C_OMAP_ERRATA_I462) {
			int ret;

			ret = errata_omap3_i462(dev);
			if (ret < 0)
				return ret;
		}

		omap_i2c_write_reg(dev, OMAP_I2C_DATA_REG, w);
	}

	return 0;
}

static int
omap_i2c_isr(struct omap_i2c_struct *dev)
{
	u16 bits;
	u16 stat;
	int err = 0, count = 0;

	do {
		bits = omap_i2c_read_reg(dev, OMAP_I2C_IE_REG);
		stat = omap_i2c_read_reg(dev, OMAP_I2C_STAT_REG);
		stat &= bits;

		/* If we're in receiver mode, ignore XDR/XRDY */
		if (dev->receiver)
			stat &= ~(OMAP_I2C_STAT_XDR | OMAP_I2C_STAT_XRDY);
		else
			stat &= ~(OMAP_I2C_STAT_RDR | OMAP_I2C_STAT_RRDY);

		if (!stat) {
			/* my work here is done */
			goto out;
		}

		dev_dbg(&dev->adapter.dev, "IRQ (ISR = 0x%04x)\n", stat);
		if (count++ == 100) {
			dev_warn(&dev->adapter.dev, "Too much work in one IRQ\n");
			break;
		}

		if (stat & OMAP_I2C_STAT_NACK) {
			err |= OMAP_I2C_STAT_NACK;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_NACK);
			break;
		}

		if (stat & OMAP_I2C_STAT_AL) {
			dev_err(&dev->adapter.dev, "Arbitration lost\n");
			err |= OMAP_I2C_STAT_AL;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_AL);
			break;
		}

		/*
		 * ProDB0017052: Clear ARDY bit twice
		 */
		if (stat & OMAP_I2C_STAT_ARDY)
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ARDY);


		if (stat & (OMAP_I2C_STAT_ARDY | OMAP_I2C_STAT_NACK |
					OMAP_I2C_STAT_AL)) {
			omap_i2c_ack_stat(dev, (OMAP_I2C_STAT_RRDY |
						OMAP_I2C_STAT_RDR |
						OMAP_I2C_STAT_XRDY |
						OMAP_I2C_STAT_XDR |
						OMAP_I2C_STAT_ARDY));
			break;
		}

		if (stat & OMAP_I2C_STAT_RDR) {
			u8 num_bytes = 1;

			if (dev->fifo_size)
				num_bytes = dev->buf_len;

			omap_i2c_receive_data(dev, num_bytes, true);

			if (dev->errata & I2C_OMAP_ERRATA_I207)
				i2c_omap_errata_i207(dev, stat);

			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RDR);
			continue;
		}

		if (stat & OMAP_I2C_STAT_RRDY) {
			u8 num_bytes = 1;

			if (dev->threshold)
				num_bytes = dev->threshold;

			omap_i2c_receive_data(dev, num_bytes, false);
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_RRDY);
			continue;
		}

		if (stat & OMAP_I2C_STAT_XDR) {
			u8 num_bytes = 1;
			int ret;

			if (dev->fifo_size)
				num_bytes = dev->buf_len;

			ret = omap_i2c_transmit_data(dev, num_bytes, true);
			if (ret < 0)
				break;

			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XDR);
			continue;
		}

		if (stat & OMAP_I2C_STAT_XRDY) {
			u8 num_bytes = 1;
			int ret;

			if (dev->threshold)
				num_bytes = dev->threshold;

			ret = omap_i2c_transmit_data(dev, num_bytes, false);
			if (ret < 0)
				break;

			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XRDY);
			continue;
		}

		if (stat & OMAP_I2C_STAT_ROVR) {
			dev_err(&dev->adapter.dev, "Receive overrun\n");
			err |= OMAP_I2C_STAT_ROVR;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_ROVR);
			break;
		}

		if (stat & OMAP_I2C_STAT_XUDF) {
			dev_err(&dev->adapter.dev, "Transmit underflow\n");
			err |= OMAP_I2C_STAT_XUDF;
			omap_i2c_ack_stat(dev, OMAP_I2C_STAT_XUDF);
			break;
		}
	} while (stat);

	omap_i2c_complete_cmd(dev, err);
	return 0;

out:
	return -EBUSY;
}

static void omap_i2c_resize_fifo(struct omap_i2c_struct *dev, u8 size,
								bool is_rx)
{
	u16		buf;

	if (dev->data->flags & OMAP_I2C_FLAG_NO_FIFO)
		return;

	/*
	 * Set up notification threshold based on message size. We're doing
	 * this to try and avoid draining feature as much as possible. Whenever
	 * we have big messages to transfer (bigger than our total fifo size)
	 * then we might use draining feature to transfer the remaining bytes.
	 */

	dev->threshold = clamp(size, (u8) 1, dev->fifo_size);

	buf = omap_i2c_read_reg(dev, OMAP_I2C_BUF_REG);

	if (is_rx) {
		/* Clear RX Threshold */
		buf &= ~(0x3f << 8);
		buf |= ((dev->threshold - 1) << 8) | OMAP_I2C_BUF_RXFIF_CLR;
	} else {
		/* Clear TX Threshold */
		buf &= ~0x3f;
		buf |= (dev->threshold - 1) | OMAP_I2C_BUF_TXFIF_CLR;
	}

	omap_i2c_write_reg(dev, OMAP_I2C_BUF_REG, buf);

	if (dev->rev < OMAP_I2C_REV_ON_3630)
		dev->b_hw = 1; /* Enable hardware fixes */
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

	i2c_omap->receiver = !!(msg->flags & I2C_M_RD);
	omap_i2c_resize_fifo(i2c_omap, msg->len, i2c_omap->receiver);

	omap_i2c_write_reg(i2c_omap, OMAP_I2C_SA_REG, msg->addr);

	/* REVISIT: Could the STB bit of I2C_CON be used with probing? */
	i2c_omap->buf = msg->buf;
	i2c_omap->buf_len = msg->len;

	/* make sure writes to dev->buf_len are ordered */
	barrier();

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

	if (msg->flags & I2C_M_STOP)
		stop = 1;
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
		if (is_timeout(start, OMAP_I2C_TIMEOUT)) {
				dev_err(&adapter->dev,
				"timed out on polling for "
				"open i2c message handling\n");
				omap_i2c_reset(i2c_omap);
				__omap_i2c_init(i2c_omap);
				return -ETIMEDOUT;
			}
	}

	if (likely(!i2c_omap->cmd_err))
		return 0;

	/* We have an error */
	if (i2c_omap->cmd_err & (OMAP_I2C_STAT_AL | OMAP_I2C_STAT_ROVR |
			    OMAP_I2C_STAT_XUDF)) {
		omap_i2c_reset(i2c_omap);
		__omap_i2c_init(i2c_omap);
		return -EIO;
	}

	if (i2c_omap->cmd_err & OMAP_I2C_STAT_NACK) {
		if (msg->flags & I2C_M_IGNORE_NAK)
			return 0;

		w = omap_i2c_read_reg(i2c_omap, OMAP_I2C_CON_REG);
		w |= OMAP_I2C_CON_STP;
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, w);
		return -EREMOTEIO;
	}
	return -EIO;
}

static void omap_i2c_unidle(struct omap_i2c_struct *i2c_omap)
{
		__omap_i2c_init(i2c_omap);
}

static void omap_i2c_idle(struct omap_i2c_struct *i2c_omap)
{
	u16 iv;

	i2c_omap->iestate = omap_i2c_read_reg(i2c_omap, OMAP_I2C_IE_REG);

	/* Barebox driver don't need to clear interrupts here */

	/* omap_i2c_write_reg(i2c_omap, OMAP_I2C_IE_REG, 0); */
	if (i2c_omap->rev < OMAP_I2C_OMAP1_REV_2) {
		/* Read clears */
		iv = omap_i2c_read_reg(i2c_omap, OMAP_I2C_IV_REG);
	} else {
		omap_i2c_write_reg(i2c_omap, OMAP_I2C_STAT_REG,
							i2c_omap->iestate);

		/* Flush posted write before the i2c_omap->idle store occurs */
		omap_i2c_read_reg(i2c_omap, OMAP_I2C_STAT_REG);
	}
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

	omap_i2c_wait_for_bb(adapter);

out:
	omap_i2c_idle(i2c_omap);
	return r;
}

#define OMAP_I2C_SCHEME(rev)		((rev & 0xc000) >> 14)

#define OMAP_I2C_REV_SCHEME_0_MAJOR(rev) (rev >> 4)
#define OMAP_I2C_REV_SCHEME_0_MINOR(rev) (rev & 0xf)

#define OMAP_I2C_REV_SCHEME_1_MAJOR(rev) ((rev & 0x0700) >> 7)
#define OMAP_I2C_REV_SCHEME_1_MINOR(rev) (rev & 0x1f)
#define OMAP_I2C_SCHEME_0		0
#define OMAP_I2C_SCHEME_1		1

static int __init
i2c_omap_probe(struct device_d *pdev)
{
	struct omap_i2c_struct	*i2c_omap;
	struct omap_i2c_driver_data *i2c_data;
	int r;
	u32 speed = 0;
	u32 rev;
	u16 minor, major;

	i2c_omap = kzalloc(sizeof(struct omap_i2c_struct), GFP_KERNEL);
	if (!i2c_omap) {
		r = -ENOMEM;
		goto err_free_mem;
	}

	r = dev_get_drvdata(pdev, (const void **)&i2c_data);
	if (r)
		return r;

	if (of_machine_is_compatible("ti,am33xx"))
		i2c_data = &am33xx_data;
	if (of_machine_is_compatible("ti,omap4"))
		i2c_data = &omap4_data;

	i2c_omap->data = i2c_data;
	i2c_omap->reg_shift = (i2c_data->flags >>
					OMAP_I2C_FLAG_BUS_SHIFT__SHIFT) & 3;

	if (pdev->platform_data != NULL) {
		speed = *(u32 *)pdev->platform_data;
	} else {
		of_property_read_u32(pdev->device_node, "clock-frequency",
			&speed);
		/* convert DT freq value in Hz into kHz for speed */
		speed /= 1000;
	}

	if (!speed)
		speed = 100;	/* Default speed */

	i2c_omap->speed = speed;
	i2c_omap->base = dev_request_mem_region(pdev, 0);
	if (IS_ERR(i2c_omap->base))
		return PTR_ERR(i2c_omap->base);

	/*
	 * Read the Rev hi bit-[15:14] ie scheme this is 1 indicates ver2.
	 * On omap1/3/2 Offset 4 is IE Reg the bit [15:14] is 0 at reset.
	 * Also since the omap_i2c_read_reg uses reg_map_ip_* a
	 * raw_readw is done.
	 */
	rev = __raw_readw(i2c_omap->base + 0x04);

	i2c_omap->scheme = OMAP_I2C_SCHEME(rev);
	switch (i2c_omap->scheme) {
	case OMAP_I2C_SCHEME_0:
		i2c_omap->regs = (u8 *)reg_map_ip_v1;
		i2c_omap->rev = omap_i2c_read_reg(i2c_omap, OMAP_I2C_REV_REG);
		minor = OMAP_I2C_REV_SCHEME_0_MAJOR(i2c_omap->rev);
		major = OMAP_I2C_REV_SCHEME_0_MAJOR(i2c_omap->rev);
		break;
	case OMAP_I2C_SCHEME_1:
		/* FALLTHROUGH */
	default:
		i2c_omap->regs = (u8 *)reg_map_ip_v2;
		rev = (rev << 16) |
			omap_i2c_read_reg(i2c_omap, OMAP_I2C_IP_V2_REVNB_LO);
		minor = OMAP_I2C_REV_SCHEME_1_MINOR(rev);
		major = OMAP_I2C_REV_SCHEME_1_MAJOR(rev);
		i2c_omap->rev = rev;
	}

	i2c_omap->errata = 0;

	if (i2c_omap->rev >= OMAP_I2C_REV_ON_2430 &&
			i2c_omap->rev < OMAP_I2C_REV_ON_4430_PLUS)
		i2c_omap->errata |= I2C_OMAP_ERRATA_I207;

	if (i2c_omap->rev <= OMAP_I2C_REV_ON_3430_3530)
		i2c_omap->errata |= I2C_OMAP_ERRATA_I462;

	if (!(i2c_data->flags & OMAP_I2C_FLAG_NO_FIFO)) {
		u16 s;

		/* Set up the fifo size - Get total size */
		s = (omap_i2c_read_reg(i2c_omap, OMAP_I2C_BUFSTAT_REG) >> 14)
									 & 0x3;
		i2c_omap->fifo_size = 0x8 << s;

		/*
		 * Set up notification threshold as half the total available
		 * size. This is to ensure that we can handle the status on int
		 * call back latencies.
		 */

		i2c_omap->fifo_size = (i2c_omap->fifo_size / 2);

		if (i2c_omap->rev < OMAP_I2C_REV_ON_3630)
			i2c_omap->b_hw = 1; /* Enable hardware fixes */
	}

	/* reset ASAP, clearing any IRQs */
	omap_i2c_init(i2c_omap);

	omap_i2c_idle(i2c_omap);

	i2c_omap->adapter.master_xfer = omap_i2c_xfer,
	i2c_omap->adapter.nr = pdev->id;
	i2c_omap->adapter.dev.parent = pdev;
	i2c_omap->adapter.dev.device_node = pdev->device_node;

	/* i2c device drivers may be active on return from add_adapter() */
	r = i2c_add_numbered_adapter(&i2c_omap->adapter);
	if (r) {
		dev_err(pdev, "failure adding adapter\n");
		goto err_unuse_clocks;
	}

	dev_info(pdev, "bus %d rev%d.%d at %d kHz\n",
		 i2c_omap->adapter.nr, major, minor, i2c_omap->speed);

	return 0;

err_unuse_clocks:
	omap_i2c_write_reg(i2c_omap, OMAP_I2C_CON_REG, 0);
	omap_i2c_idle(i2c_omap);
err_free_mem:
	kfree(i2c_omap);

	return r;
}

static struct platform_device_id omap_i2c_ids[] = {
	 {
		.name = "i2c-omap3",
		.driver_data = (unsigned long)&omap3_data,
	}, {
		.name = "i2c-omap4",
		.driver_data = (unsigned long)&omap4_data,
	}, {
		.name = "i2c-am33xx",
		.driver_data = (unsigned long)&am33xx_data,
	}, {
		/* sentinel */
	},
};

static __maybe_unused struct of_device_id omap_i2c_dt_ids[] = {
	{
		.compatible = "ti,omap3-i2c",
		.data = &omap3_data,
	}, {
		.compatible = "ti,omap4-i2c",
	}, {
		/* sentinel */
	}
};

static struct driver_d omap_i2c_driver = {
	.probe		= i2c_omap_probe,
	.name		= DRIVER_NAME,
	.id_table	= omap_i2c_ids,
	.of_compatible = DRV_OF_COMPAT(omap_i2c_dt_ids),
};
device_platform_driver(omap_i2c_driver);

MODULE_AUTHOR("MontaVista Software, Inc. (and others)");
MODULE_DESCRIPTION("TI OMAP I2C bus adapter");
MODULE_LICENSE("GPL");

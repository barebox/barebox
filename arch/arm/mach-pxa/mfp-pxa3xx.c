/*
 * linux/arch/arm/plat-pxa/mfp.c
 *
 *   Multi-Function Pin Support
 *
 * Copyright (C) 2007 Marvell Internation Ltd.
 *
 * 2007-08-21: eric miao <eric.miao@marvell.com>
 *             initial version
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/hardware.h>
#include <mach/mfp-pxa3xx.h>
#include <plat/mfp.h>

#define MFPR_SIZE	(PAGE_SIZE)

/* MFPR register bit definitions */
#define MFPR_PULL_SEL		(0x1 << 15)
#define MFPR_PULLUP_EN		(0x1 << 14)
#define MFPR_PULLDOWN_EN	(0x1 << 13)
#define MFPR_SLEEP_SEL		(0x1 << 9)
#define MFPR_SLEEP_OE_N		(0x1 << 7)
#define MFPR_EDGE_CLEAR		(0x1 << 6)
#define MFPR_EDGE_FALL_EN	(0x1 << 5)
#define MFPR_EDGE_RISE_EN	(0x1 << 4)

#define MFPR_SLEEP_DATA(x)	((x) << 8)
#define MFPR_DRIVE(x)		(((x) & 0x7) << 10)
#define MFPR_AF_SEL(x)		(((x) & 0x7) << 0)

#define MFPR_EDGE_NONE		(0)
#define MFPR_EDGE_RISE		(MFPR_EDGE_RISE_EN)
#define MFPR_EDGE_FALL		(MFPR_EDGE_FALL_EN)
#define MFPR_EDGE_BOTH		(MFPR_EDGE_RISE | MFPR_EDGE_FALL)

/*
 * Table that determines the low power modes outputs, with actual settings
 * used in parentheses for don't-care values. Except for the float output,
 * the configured driven and pulled levels match, so if there is a need for
 * non-LPM pulled output, the same configuration could probably be used.
 *
 * Output value  sleep_oe_n  sleep_data  pullup_en  pulldown_en  pull_sel
 *                 (bit 7)    (bit 8)    (bit 14)     (bit 13)   (bit 15)
 *
 * Input            0          X(0)        X(0)        X(0)       0
 * Drive 0          0          0           0           X(1)       0
 * Drive 1          0          1           X(1)        0	  0
 * Pull hi (1)      1          X(1)        1           0	  0
 * Pull lo (0)      1          X(0)        0           1	  0
 * Z (float)        1          X(0)        0           0	  0
 */
#define MFPR_LPM_INPUT		(0)
#define MFPR_LPM_DRIVE_LOW	(MFPR_SLEEP_DATA(0) | MFPR_PULLDOWN_EN)
#define MFPR_LPM_DRIVE_HIGH	(MFPR_SLEEP_DATA(1) | MFPR_PULLUP_EN)
#define MFPR_LPM_PULL_LOW	(MFPR_LPM_DRIVE_LOW  | MFPR_SLEEP_OE_N)
#define MFPR_LPM_PULL_HIGH	(MFPR_LPM_DRIVE_HIGH | MFPR_SLEEP_OE_N)
#define MFPR_LPM_FLOAT		(MFPR_SLEEP_OE_N)
#define MFPR_LPM_MASK		(0xe080)

/*
 * The pullup and pulldown state of the MFP pin at run mode is by default
 * determined by the selected alternate function. In case that some buggy
 * devices need to override this default behavior,  the definitions below
 * indicates the setting of corresponding MFPR bits
 *
 * Definition       pull_sel  pullup_en  pulldown_en
 * MFPR_PULL_NONE       0         0        0
 * MFPR_PULL_LOW        1         0        1
 * MFPR_PULL_HIGH       1         1        0
 * MFPR_PULL_BOTH       1         1        1
 * MFPR_PULL_FLOAT	1         0        0
 */
#define MFPR_PULL_NONE		(0)
#define MFPR_PULL_LOW		(MFPR_PULL_SEL | MFPR_PULLDOWN_EN)
#define MFPR_PULL_BOTH		(MFPR_PULL_LOW | MFPR_PULLUP_EN)
#define MFPR_PULL_HIGH		(MFPR_PULL_SEL | MFPR_PULLUP_EN)
#define MFPR_PULL_FLOAT		(MFPR_PULL_SEL)

/* mfp_spin_lock is used to ensure that MFP register configuration
 * (most likely a read-modify-write operation) is atomic, and that
 * mfp_table[] is consistent
 */
static void __iomem *mfpr_mmio_base;

struct mfp_pin {
	unsigned long	config;		/* -1 for not configured */
	unsigned long	mfpr_off;	/* MFPRxx Register offset */
	unsigned long	mfpr_run;	/* Run-Mode Register Value */
	unsigned long	mfpr_lpm;	/* Low Power Mode Register Value */
};

static struct mfp_pin mfp_table[MFP_PIN_MAX];

/* mapping of MFP_LPM_* definitions to MFPR_LPM_* register bits */
static const unsigned long mfpr_lpm[] = {
	MFPR_LPM_INPUT,
	MFPR_LPM_DRIVE_LOW,
	MFPR_LPM_DRIVE_HIGH,
	MFPR_LPM_PULL_LOW,
	MFPR_LPM_PULL_HIGH,
	MFPR_LPM_FLOAT,
	MFPR_LPM_INPUT,
};

/* mapping of MFP_PULL_* definitions to MFPR_PULL_* register bits */
static const unsigned long mfpr_pull[] = {
	MFPR_PULL_NONE,
	MFPR_PULL_LOW,
	MFPR_PULL_HIGH,
	MFPR_PULL_BOTH,
	MFPR_PULL_FLOAT,
};

/* mapping of MFP_LPM_EDGE_* definitions to MFPR_EDGE_* register bits */
static const unsigned long mfpr_edge[] = {
	MFPR_EDGE_NONE,
	MFPR_EDGE_RISE,
	MFPR_EDGE_FALL,
	MFPR_EDGE_BOTH,
};

#define mfpr_readl(off)			\
	__raw_readl(mfpr_mmio_base + (off))

#define mfpr_writel(off, val)		\
	__raw_writel(val, mfpr_mmio_base + (off))

#define mfp_configured(p)	((p)->config != -1)

/*
 * perform a read-back of any valid MFPR register to make sure the
 * previous writings are finished
 */
static unsigned long mfpr_off_readback;
#define mfpr_sync()	(void)__raw_readl(mfpr_mmio_base + mfpr_off_readback)

static inline void __mfp_config_run(struct mfp_pin *p)
{
	if (mfp_configured(p))
		mfpr_writel(p->mfpr_off, p->mfpr_run);
}

static inline void __mfp_config_lpm(struct mfp_pin *p)
{
	if (mfp_configured(p)) {
		unsigned long mfpr_clr =
			(p->mfpr_run & ~MFPR_EDGE_BOTH) | MFPR_EDGE_CLEAR;

		if (mfpr_clr != p->mfpr_run)
			mfpr_writel(p->mfpr_off, mfpr_clr);
		if (p->mfpr_lpm != mfpr_clr)
			mfpr_writel(p->mfpr_off, p->mfpr_lpm);
	}
}

void mfp_config(unsigned long *mfp_cfgs, int num)
{
	int i;

	for (i = 0; i < num; i++, mfp_cfgs++) {
		unsigned long tmp, c = *mfp_cfgs;
		struct mfp_pin *p;
		int pin, af, drv, lpm, edge, pull;

		pin = MFP_PIN(c);
		BUG_ON(pin >= MFP_PIN_MAX);
		p = &mfp_table[pin];

		af  = MFP_AF(c);
		drv = MFP_DS(c);
		lpm = MFP_LPM_STATE(c);
		edge = MFP_LPM_EDGE(c);
		pull = MFP_PULL(c);

		/* run-mode pull settings will conflict with MFPR bits of
		 * low power mode state,  calculate mfpr_run and mfpr_lpm
		 * individually if pull != MFP_PULL_NONE
		 */
		tmp = MFPR_AF_SEL(af) | MFPR_DRIVE(drv);

		if (likely(pull == MFP_PULL_NONE)) {
			p->mfpr_run = tmp | mfpr_lpm[lpm] | mfpr_edge[edge];
			p->mfpr_lpm = p->mfpr_run;
		} else {
			p->mfpr_lpm = tmp | mfpr_lpm[lpm] | mfpr_edge[edge];
			p->mfpr_run = tmp | mfpr_pull[pull];
		}

		p->config = c; __mfp_config_run(p);
	}

	mfpr_sync();
}

unsigned long mfp_read(int mfp)
{
	unsigned long val;

	BUG_ON(mfp < 0 || mfp >= MFP_PIN_MAX);

	val = mfpr_readl(mfp_table[mfp].mfpr_off);
	return val;
}

void mfp_write(int mfp, unsigned long val)
{
	BUG_ON(mfp < 0 || mfp >= MFP_PIN_MAX);

	mfpr_writel(mfp_table[mfp].mfpr_off, val);
	mfpr_sync();
}

void __init mfp_init_base(void __iomem *mfpr_base)
{
	int i;

	/* initialize the table with default - unconfigured */
	for (i = 0; i < ARRAY_SIZE(mfp_table); i++)
		mfp_table[i].config = -1;

	mfpr_mmio_base = mfpr_base;
}

void __init mfp_init_addr(struct mfp_addr_map *map)
{
	struct mfp_addr_map *p;
	unsigned long offset;
	int i;

	/* mfp offset for readback */
	mfpr_off_readback = map[0].offset;

	for (p = map; p->start != MFP_PIN_INVALID; p++) {
		offset = p->offset;
		i = p->start;

		do {
			mfp_table[i].mfpr_off = offset;
			mfp_table[i].mfpr_run = 0;
			mfp_table[i].mfpr_lpm = 0;
			offset += 4; i++;
		} while ((i <= p->end) && (p->end != -1));
	}
}

void mfp_config_lpm(void)
{
	struct mfp_pin *p = &mfp_table[0];
	int pin;

	for (pin = 0; pin < ARRAY_SIZE(mfp_table); pin++, p++)
		__mfp_config_lpm(p);
}

void mfp_config_run(void)
{
	struct mfp_pin *p = &mfp_table[0];
	int pin;

	for (pin = 0; pin < ARRAY_SIZE(mfp_table); pin++, p++)
		__mfp_config_run(p);
}

static struct mfp_addr_map pxa300_mfp_addr_map[] __initdata = {

	MFP_ADDR_X(GPIO0,   GPIO2,   0x00b4),
	MFP_ADDR_X(GPIO3,   GPIO26,  0x027c),
	MFP_ADDR_X(GPIO27,  GPIO98,  0x0400),
	MFP_ADDR_X(GPIO99,  GPIO127, 0x0600),
	MFP_ADDR_X(GPIO0_2, GPIO1_2, 0x0674),
	MFP_ADDR_X(GPIO2_2, GPIO6_2, 0x02dc),

	MFP_ADDR(nBE0, 0x0204),
	MFP_ADDR(nBE1, 0x0208),

	MFP_ADDR(nLUA, 0x0244),
	MFP_ADDR(nLLA, 0x0254),

	MFP_ADDR(DF_CLE_nOE, 0x0240),
	MFP_ADDR(DF_nRE_nOE, 0x0200),
	MFP_ADDR(DF_ALE_nWE, 0x020C),
	MFP_ADDR(DF_INT_RnB, 0x00C8),
	MFP_ADDR(DF_nCS0, 0x0248),
	MFP_ADDR(DF_nCS1, 0x0278),
	MFP_ADDR(DF_nWE, 0x00CC),

	MFP_ADDR(DF_ADDR0, 0x0210),
	MFP_ADDR(DF_ADDR1, 0x0214),
	MFP_ADDR(DF_ADDR2, 0x0218),
	MFP_ADDR(DF_ADDR3, 0x021C),

	MFP_ADDR(DF_IO0, 0x0220),
	MFP_ADDR(DF_IO1, 0x0228),
	MFP_ADDR(DF_IO2, 0x0230),
	MFP_ADDR(DF_IO3, 0x0238),
	MFP_ADDR(DF_IO4, 0x0258),
	MFP_ADDR(DF_IO5, 0x0260),
	MFP_ADDR(DF_IO6, 0x0268),
	MFP_ADDR(DF_IO7, 0x0270),
	MFP_ADDR(DF_IO8, 0x0224),
	MFP_ADDR(DF_IO9, 0x022C),
	MFP_ADDR(DF_IO10, 0x0234),
	MFP_ADDR(DF_IO11, 0x023C),
	MFP_ADDR(DF_IO12, 0x025C),
	MFP_ADDR(DF_IO13, 0x0264),
	MFP_ADDR(DF_IO14, 0x026C),
	MFP_ADDR(DF_IO15, 0x0274),

	MFP_ADDR_END,
};

/* override pxa300 MFP register addresses */
static struct mfp_addr_map pxa310_mfp_addr_map[] __initdata = {
	MFP_ADDR_X(GPIO30,  GPIO98,   0x0418),
	MFP_ADDR_X(GPIO7_2, GPIO12_2, 0x052C),

	MFP_ADDR(ULPI_STP, 0x040C),
	MFP_ADDR(ULPI_NXT, 0x0410),
	MFP_ADDR(ULPI_DIR, 0x0414),

	MFP_ADDR_END,
};

void mfp_init(void)
{
	mfp_init_base((void __iomem *)MFPR_BASE);
	mfp_init_addr(pxa300_mfp_addr_map);
	if (cpu_is_pxa310())
		mfp_init_addr(pxa310_mfp_addr_map);
}

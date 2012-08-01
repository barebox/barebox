/*
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>
#include <common.h>
#include <init.h>
#include <clock.h>
#include <io.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>

#define S3C_TCFG0 (S3C_TIMER_BASE + 0x00)
# define S3C_TCFG0_T4MASK 0xff00
# define S3C_TCFG0_SET_PSCL234(x) ((x) << 8)
# define S3C_TCFG0_GET_PSCL234(x) (((x) >> 8) & 0xff)
#define S3C_TCFG1 (S3C_TIMER_BASE + 0x04)
# define S3C_TCFG1_T4MASK 0xf0000
# define S3C_TCFG1_SET_T4MUX(x) ((x) << 16)
# define S3C_TCFG1_GET_T4MUX(x) (((x) >> 16) & 0xf)
#define S3C_TCON (S3C_TIMER_BASE + 0x08)
# define S3C_TCON_T4MASK (7 << 20)
# define S3C_TCON_T4START (1 << 20)
# define S3C_TCON_T4MANUALUPD (1 << 21)
# define S3C_TCON_T4RELOAD (1 <<22)
#define S3C_TCNTB4 (S3C_TIMER_BASE + 0x3c)
#define S3C_TCNTO4 (S3C_TIMER_BASE + 0x40)

#ifdef CONFIG_ARCH_S3C24xx
# define TIMER_WIDTH 16
# define TIMER_SHIFT 10
# define PRE_MUX 3
# define PRE_MUX_ADD 1
static const uint32_t max = 0x0000ffff;
#else /* for S3C64xx and S5Pxx */
# define TIMER_WIDTH 32
# define TIMER_SHIFT 10
# define PRE_MUX 4
# define PRE_MUX_ADD 0
static const uint32_t max = ~0;
#endif

static void s3c_init_t4_clk_source(void)
{
	unsigned reg;

	reg = readl(S3C_TCON) & ~S3C_TCON_T4MASK; /* stop timer 4 */
	writel(reg, S3C_TCON);
	reg = readl(S3C_TCFG0) & ~S3C_TCFG0_T4MASK;
	reg |= S3C_TCFG0_SET_PSCL234(0); /* 0 means pre scaler is '256' */
	writel(reg, S3C_TCFG0);
	reg = readl(S3C_TCFG1) & ~S3C_TCFG1_T4MASK;
	reg |= S3C_TCFG1_SET_T4MUX(PRE_MUX); /* / 16 */
	writel(reg, S3C_TCFG1);
}

static unsigned s3c_get_t4_clk(void)
{
	unsigned clk = s3c_get_pclk();
	unsigned pre = S3C_TCFG0_GET_PSCL234(readl(S3C_TCFG0)) + 1;
	unsigned div = S3C_TCFG1_GET_T4MUX(readl(S3C_TCFG1)) + PRE_MUX_ADD;

	return clk / pre / (1 << div);
}

static void s3c_timer_init(void)
{
	unsigned tcon;

	tcon = readl(S3C_TCON) & ~S3C_TCON_T4MASK;

	writel(max, S3C_TCNTB4);	/* reload value */
	/* force a manual counter update */
	writel(tcon | S3C_TCON_T4MANUALUPD, S3C_TCON);
}

static void s3c_timer_start(void)
{
	unsigned tcon;

	tcon  = readl(S3C_TCON) & ~S3C_TCON_T4MANUALUPD;
	tcon |= S3C_TCON_T4START | S3C_TCON_T4RELOAD;
	writel(tcon, S3C_TCON);
}

static uint64_t s3c_clocksource_read(void)
{
	/* note: its a down counter */
	return max - readl(S3C_TCNTO4);
}

static struct clocksource cs = {
	.read = s3c_clocksource_read,
	.mask = CLOCKSOURCE_MASK(TIMER_WIDTH),
	.shift = TIMER_SHIFT,
};

static int s3c_clk_src_init(void)
{
	/* select its clock source first */
	s3c_init_t4_clk_source();

	s3c_timer_init();
	s3c_timer_start();

	cs.mult = clocksource_hz2mult(s3c_get_t4_clk(), cs.shift);
	init_clock(&cs);

	return 0;
}
core_initcall(s3c_clk_src_init);

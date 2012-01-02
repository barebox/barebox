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


static uint64_t s3c24xx_clocksource_read(void)
{
	/* note: its a down counter */
	return 0xFFFF - readw(S3C_TCNTO4);
}

static struct clocksource cs = {
	.read	= s3c24xx_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(16),
	.shift	= 10,
};

static int clocksource_init(void)
{
	uint32_t p_clk = s3c_get_pclk();

	writel(0x00000000, S3C_TCON);	/* stop all timers */
	writel(0x00ffffff, S3C_TCFG0);	/* PCLK / (255 + 1) for timer 4 */
	writel(0x00030000, S3C_TCFG1);	/* /16 */

	writew(0xffff, S3C_TCNTB4);	/* reload value is TOP */

	writel(0x00600000, S3C_TCON);	/* force a first reload */
	writel(0x00400000, S3C_TCON);
	writel(0x00500000, S3C_TCON);	/* enable timer 4 with auto reload */

	cs.mult = clocksource_hz2mult(p_clk / ((255 + 1) * 16), cs.shift);
	init_clock(&cs);

	return 0;
}
core_initcall(clocksource_init);

/**
 * @file
 * @brief Support DMTimer0 counter
 *
 * FileName: arch/arm/mach-omap/dmtimer0.c
 */
/*
 * This File is based on arch/arm/mach-omap/s32k_clksource.c
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * (C) Copyright 2012 Teresa Gámez, Phytec Messtechnik GmbH
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <clock.h>
#include <init.h>
#include <io.h>
#include <mach/am33xx-silicon.h>

#define CLK_RC32K	32768

#define TIDR			0x0
#define TIOCP_CFG		0x10
#define IRQ_EOI			0x20
#define IRQSTATUS_RAW		0x24
#define IRQSTATUS		0x28
#define IRQSTATUS_SET		0x2c
#define IRQSTATUS_CLR		0x30
#define IRQWAKEEN		0x34
#define TCLR			0x38
#define TCRR			0x3C
#define TLDR			0x40
#define TTGR			0x44
#define TWPS			0x48
#define TMAR			0x4C
#define TCAR1			0x50
#define TSICR			0x54
#define TCAR2			0x58

/**
 * @brief Provide a simple counter read
 *
 * @return DMTimer0 counter
 */
static uint64_t dmtimer0_read(void)
{
	return readl(AM33XX_DMTIMER0_BASE + TCRR);
}

static struct clocksource dmtimer0_cs = {
	.read	= dmtimer0_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

/**
 * @brief Initialize the Clock
 *
 * Enable dmtimer0.
 *
 * @return result of @ref init_clock
 */
static int dmtimer0_init(void)
{
	dmtimer0_cs.mult = clocksource_hz2mult(CLK_RC32K, dmtimer0_cs.shift);
	/* Enable counter */
	writel(0x3, AM33XX_DMTIMER0_BASE + TCLR);

	return init_clock(&dmtimer0_cs);
}

/* Run me at boot time */
core_initcall(dmtimer0_init);

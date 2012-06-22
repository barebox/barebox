/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <notifier.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <io.h>

#define GPT(x) __REG(IMX_TIM1_BASE + (x))
#define timer_base IOMEM(IMX_TIM1_BASE)

static uint64_t imx_clocksource_read(void)
{
	return readl(timer_base + GPT_TCN);
}

static struct clocksource cs = {
	.read	= imx_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int imx_clocksource_clock_change(struct notifier_block *nb, unsigned long event, void *data)
{
	cs.mult = clocksource_hz2mult(imx_get_gptclk(), cs.shift);
	return 0;
}

static struct notifier_block imx_clock_notifier = {
	.notifier_call = imx_clocksource_clock_change,
};

static int clocksource_init (void)
{
	int i;
	uint32_t val;

	/* setup GP Timer 1 */
	writel(TCTL_SWR, timer_base + GPT_TCTL);

#ifdef CONFIG_ARCH_IMX21
	PCCR1 |= PCCR1_GPT1_EN;
#endif
#ifdef CONFIG_ARCH_IMX27
	PCCR0 |= PCCR0_GPT1_EN;
	PCCR1 |= PCCR1_PERCLK1_EN;
#endif
#ifdef CONFIG_ARCH_IMX25
	writel(readl(IMX_CCM_BASE + CCM_CGCR1) | (1 << 19),
		IMX_CCM_BASE + CCM_CGCR1);
#endif

	for (i = 0; i < 100; i++)
		writel(0, timer_base + GPT_TCTL); /* We have no udelay by now */

	writel(0, timer_base + GPT_TPRER);
	val = readl(timer_base + GPT_TCTL);
	val |= TCTL_FRR | (1 << TCTL_CLKSOURCE) | TCTL_TEN; /* Freerun Mode, PERCLK1 input */
	writel(val, timer_base + GPT_TCTL);

	cs.mult = clocksource_hz2mult(imx_get_gptclk(), cs.shift);

	init_clock(&cs);

	clock_register_client(&imx_clock_notifier);

	return 0;
}

core_initcall(clocksource_init);

/*
 * Watchdog Registers
 */
#ifdef CONFIG_ARCH_IMX1
#define WDOG_WCR	0x00 /* Watchdog Control Register */
#define WDOG_WSR	0x04 /* Watchdog Service Register */
#define WDOG_WSTR	0x08 /* Watchdog Status Register  */
#define WDOG_WCR_WDE	(1 << 0)
#else
#define WDOG_WCR	0x00 /* Watchdog Control Register */
#define WDOG_WSR	0x02 /* Watchdog Service Register */
#define WDOG_WSTR	0x04 /* Watchdog Status Register  */
#define WDOG_WCR_WDE	(1 << 2)
#endif

/*
 * Reset the cpu by setting up the watchdog timer and let it time out
 */
void __noreturn reset_cpu (unsigned long addr)
{
	void __iomem *wdt = IOMEM(IMX_WDT_BASE);

	/* Disable watchdog and set Time-Out field to 0 */
	writew(0x0, wdt + WDOG_WCR);

	/* Write Service Sequence */
	writew(0x5555, wdt + WDOG_WSR);
	writew(0xaaaa, wdt + WDOG_WSR);

	/* Enable watchdog */
	writew(WDOG_WCR_WDE, wdt + WDOG_WCR);

	while (1);
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);

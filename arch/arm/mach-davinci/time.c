/*
 * DaVinci timer subsystem
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <clock.h>

#include <mach/time.h>

/* Timer register offsets */
#define PID12			0x0
#define TIM12			0x10
#define TIM34			0x14
#define PRD12			0x18
#define PRD34			0x1c
#define TCR			0x20
#define TGCR			0x24
#define WDTCR			0x28

/* Timer register bitfields */
#define TCR_ENAMODE_DISABLE          0x0
#define TCR_ENAMODE_ONESHOT          0x1
#define TCR_ENAMODE_PERIODIC         0x2
#define TCR_ENAMODE_MASK             0x3

#define TGCR_TIMMODE_SHIFT           2
#define TGCR_TIMMODE_64BIT_GP        0x0
#define TGCR_TIMMODE_32BIT_UNCHAINED 0x1
#define TGCR_TIMMODE_64BIT_WDOG      0x2
#define TGCR_TIMMODE_32BIT_CHAINED   0x3

#define TGCR_TIM12RS_SHIFT           0
#define TGCR_TIM34RS_SHIFT           1
#define TGCR_RESET                   0x0
#define TGCR_UNRESET                 0x1
#define TGCR_RESET_MASK              0x3

#define WDTCR_WDEN_SHIFT             14
#define WDTCR_WDEN_DISABLE           0x0
#define WDTCR_WDEN_ENABLE            0x1
#define WDTCR_WDKEY_SHIFT            16
#define WDTCR_WDKEY_SEQ0             0xa5c6
#define WDTCR_WDKEY_SEQ1             0xda7e

#define DAVINCI_TIMER_CLOCK 24000000

struct timer_s {
	void __iomem *base;
	unsigned long tim_off;
	unsigned long prd_off;
	unsigned long enamode_shift;
};

static struct timer_s timers[] = {
	{
		.base = IOMEM(DAVINCI_TIMER0_BASE),
		.enamode_shift = 6,
		.tim_off = TIM12,
		.prd_off = PRD12,
	},
	{
		.base = IOMEM(DAVINCI_TIMER0_BASE),
		.enamode_shift = 22,
		.tim_off = TIM34,
		.prd_off = PRD34,
	},
	{
		.base = IOMEM(DAVINCI_TIMER1_BASE),
		.enamode_shift = 6,
		.tim_off = TIM12,
		.prd_off = PRD12,
	},
	{
		.base = IOMEM(DAVINCI_TIMER1_BASE),
		.enamode_shift = 22,
		.tim_off = TIM34,
		.prd_off = PRD34,
	},
};

static struct timer_s *t = &timers[0];

static uint64_t davinci_cs_read(void)
{
	return (uint64_t)__raw_readl(t->base + t->tim_off);
}

static struct clocksource davinci_cs = {
	.read	= davinci_cs_read,
	.mask	= CLOCKSOURCE_MASK(32),
};

static int timer32_config(struct timer_s *t)
{
	u32 tcr;

	tcr = __raw_readl(t->base + TCR);

	/* disable timer */
	tcr &= ~(TCR_ENAMODE_MASK << t->enamode_shift);
	__raw_writel(tcr, t->base + TCR);

	/* reset counter to zero, set new period */
	__raw_writel(0, t->base + t->tim_off);
	__raw_writel(0xffffffff, t->base + t->prd_off);

	/* Set enable mode for periodic timer */
	tcr |= TCR_ENAMODE_PERIODIC << t->enamode_shift;

	__raw_writel(tcr, t->base + TCR);

	return 0;
}

/* Global init of 64-bit timer as a whole */
static void __init timer_init(void __iomem *base)
{
	u32 tgcr;

	/* Disabled, Internal clock source */
	__raw_writel(0, base + TCR);

	/* reset both timers, no pre-scaler for timer34 */
	tgcr = 0;
	__raw_writel(tgcr, base + TGCR);

	/* Set both timers to unchained 32-bit */
	tgcr = TGCR_TIMMODE_32BIT_UNCHAINED << TGCR_TIMMODE_SHIFT;
	__raw_writel(tgcr, base + TGCR);

	/* Unreset timers */
	tgcr |= (TGCR_UNRESET << TGCR_TIM12RS_SHIFT) |
		(TGCR_UNRESET << TGCR_TIM34RS_SHIFT);
	__raw_writel(tgcr, base + TGCR);

	/* Init both counters to zero */
	__raw_writel(0, base + TIM12);
	__raw_writel(0, base + TIM34);
}

static int clocksource_init(void)
{
	clocks_calc_mult_shift(&davinci_cs.mult, &davinci_cs.shift,
		DAVINCI_TIMER_CLOCK, NSEC_PER_SEC, 10);

	init_clock(&davinci_cs);

	timer_init(IOMEM(DAVINCI_TIMER0_BASE));
	timer_init(IOMEM(DAVINCI_TIMER1_BASE));

	timer32_config(t);

	return 0;
}
core_initcall(clocksource_init);

/* reset board using watchdog timer */
void __noreturn reset_cpu(ulong addr)
{
	u32 tgcr, wdtcr;
	void __iomem *base;

	base = IOMEM(DAVINCI_WDOG_BASE);

	/* disable, internal clock source */
	__raw_writel(0, base + TCR);

	/* reset timer, set mode to 64-bit watchdog, and unreset */
	tgcr = 0;
	__raw_writel(tgcr, base + TGCR);
	tgcr = TGCR_TIMMODE_64BIT_WDOG << TGCR_TIMMODE_SHIFT;
	tgcr |= (TGCR_UNRESET << TGCR_TIM12RS_SHIFT) |
		(TGCR_UNRESET << TGCR_TIM34RS_SHIFT);
	__raw_writel(tgcr, base + TGCR);

	/* clear counter and period regs */
	__raw_writel(0, base + TIM12);
	__raw_writel(0, base + TIM34);
	__raw_writel(0, base + PRD12);
	__raw_writel(0, base + PRD34);

	/* put watchdog in pre-active state */
	wdtcr = __raw_readl(base + WDTCR);
	wdtcr = (WDTCR_WDKEY_SEQ0 << WDTCR_WDKEY_SHIFT) |
		(WDTCR_WDEN_ENABLE << WDTCR_WDEN_SHIFT);
	__raw_writel(wdtcr, base + WDTCR);

	/* put watchdog in active state */
	wdtcr = (WDTCR_WDKEY_SEQ1 << WDTCR_WDKEY_SHIFT) |
		(WDTCR_WDEN_ENABLE << WDTCR_WDEN_SHIFT);
	__raw_writel(wdtcr, base + WDTCR);

	/* write an invalid value to the WDKEY field to trigger
	 * a watchdog reset */
	wdtcr = 0x00004000;
	__raw_writel(wdtcr, base + WDTCR);

	unreachable();
}
EXPORT_SYMBOL(reset_cpu);

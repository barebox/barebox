/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2007 Andrew Victor */
/* SPDX-FileCopyrightText: 2007 Atmel Corporation */

/*
 * arch/arm/mach-at91/include/mach/at91_rtt.h
 *
 * Real-time Timer (RTT) - System peripherals regsters.
 * Based on AT91SAM9261 datasheet revision D.
 */

#ifndef AT91_RTT_H
#define AT91_RTT_H

#include <io.h>

#define AT91_RTT_MR		0x00			/* Real-time Mode Register */
#define		AT91_RTT_RTPRES		(0xffff << 0)		/* Real-time Timer Prescaler Value */
#define		AT91_RTT_ALMIEN		(1 << 16)		/* Alarm Interrupt Enable */
#define		AT91_RTT_RTTINCIEN	(1 << 17)		/* Real Time Timer Increment Interrupt Enable */
#define		AT91_RTT_RTTRST		(1 << 18)		/* Real Time Timer Restart */

#define AT91_RTT_AR		0x04			/* Real-time Alarm Register */
#define		AT91_RTT_ALMV		(0xffffffff)		/* Alarm Value */

#define AT91_RTT_VR		0x08			/* Real-time Value Register */
#define		AT91_RTT_CRTV		(0xffffffff)		/* Current Real-time Value */

#define AT91_RTT_SR		0x0c			/* Real-time Status Register */
#define		AT91_RTT_ALMS		(1 << 0)		/* Real-time Alarm Status */
#define		AT91_RTT_RTTINC		(1 << 1)		/* Real-time Timer Increment */


/*
 * As the RTT is powered by the backup power so if the interrupt
 * is still on when the kernel start, the kernel will end up with
 * dead lock interrupt that it can not clear. Because the interrupt line is
 * shared with the basic timer (PIT) on AT91_ID_SYS.
 */
static inline void at91_rtt_irq_fixup(void *base)
{
	void __iomem *reg = base + AT91_RTT_MR;
	u32 mr = readl(reg);

	writel(mr & ~(AT91_RTT_ALMIEN | AT91_RTT_RTTINCIEN), reg);
}
#endif

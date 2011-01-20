/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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
#include <notifier.h>
#include <mach/imx-regs.h>
#include <asm/io.h>

#define HW_RTC_CTRL     0x000
# define BM_RTC_CTRL_WATCHDOGEN (1 << 4)
#define HW_RTC_CTRL_SET 0x004
#define HW_RTC_CTRL_CLR 0x008
#define HW_RTC_CTRL_TOG 0x00C

#define HW_RTC_WATCHDOG     0x050
#define HW_RTC_WATCHDOG_SET 0x054
#define HW_RTC_WATCHDOG_CLR 0x058
#define HW_RTC_WATCHDOG_TOG 0x05C

#define WDOG_COUNTER_RATE	1000 /* 1 kHz clock */

#define HW_RTC_PERSISTENT1     0x070
# define BV_RTC_PERSISTENT1_GENERAL__RTC_FORCE_UPDATER 0x80000000
#define HW_RTC_PERSISTENT1_SET 0x074
#define HW_RTC_PERSISTENT1_CLR 0x078
#define HW_RTC_PERSISTENT1_TOG 0x07C

/*
 * Reset the cpu by setting up the watchdog timer and let it time out
 *
 * TODO There is a much easier way to reset the CPU: Refer bit 2 in
 *       the HW_CLKCTRL_RESET register, data sheet page 106/4-30
 */
void __noreturn reset_cpu (unsigned long addr)
{
	writel(WDOG_COUNTER_RATE, IMX_WDT_BASE + HW_RTC_WATCHDOG);
	writel(BM_RTC_CTRL_WATCHDOGEN, IMX_WDT_BASE + HW_RTC_CTRL_SET);
	writel(BV_RTC_PERSISTENT1_GENERAL__RTC_FORCE_UPDATER, IMX_WDT_BASE + HW_RTC_PERSISTENT1);

	while (1)
		;
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);

/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* register definitions */
#define PMC_CNTRL			0x000
#define PMC_CNTRL_FUSE_OVERRIDE		(1 << 18)
#define PMC_CNTRL_INTR_POLARITY		(1 << 17)
#define PMC_CNTRL_CPUPWRREQ_OE		(1 << 16)
#define PMC_CNTRL_CPUPWRREQ_POLARITY	(1 << 15)
#define PMC_CNTRL_SIDE_EFFECT_LP0	(1 << 14)
#define PMC_CNTRL_AOINIT		(1 << 13)
#define PMC_CNTRL_PWRGATE_DIS		(1 << 12)
#define PMC_CNTRL_SYSCLK_OE		(1 << 11)
#define PMC_CNTRL_SYSCLK_POLARITY	(1 << 10)
#define PMC_CNTRL_PWRREQ_OE		(1 << 9)
#define PMC_CNTRL_PWRREQ_POLARITY	(1 << 8)
#define PMC_CNTRL_BLINK_EN		(1 << 7)
#define PMC_CNTRL_GLITCHDET_DIS		(1 << 6)
#define PMC_CNTRL_LATCHWAKE_EN		(1 << 5)
#define PMC_CNTRL_MAIN_RST		(1 << 4)
#define PMC_CNTRL_KBC_RST		(1 << 3)
#define PMC_CNTRL_RTC_RST		(1 << 2)
#define PMC_CNTRL_RTC_CLK_DIS		(1 << 1)
#define PMC_CNTRL_KBC_CLK_DIS		(1 << 0)

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2017 Pengutronix, Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on code from  Carlo Caione <carlo@carlocaione.org>
 */

#ifndef __BCM2835_WDT_H
#define __BCM2835_WDT_H

#define PM_RSTC				0x1c
#define PM_RSTS				0x20
#define PM_WDOG				0x24

#define PM_WDOG_RESET			0000000000
#define PM_PASSWORD			0x5a000000
#define PM_WDOG_TIME_SET		0x000fffff
#define PM_RSTC_WRCFG_CLR		0xffffffcf
#define PM_RSTC_WRCFG_SET		0x00000030
#define PM_RSTC_WRCFG_FULL_RESET	0x00000020
#define PM_RSTC_RESET			0x00000102

#define PM_RSTS_HADPOR_SET		0x00001000
#define PM_RSTS_HADSRH_SET		0x00000400
#define PM_RSTS_HADSRF_SET		0x00000200
#define PM_RSTS_HADSRQ_SET		0x00000100
#define PM_RSTS_HADWRH_SET		0x00000040
#define PM_RSTS_HADWRF_SET		0x00000020
#define PM_RSTS_HADWRQ_SET		0x00000010
#define PM_RSTS_HADDRH_SET		0x00000004
#define PM_RSTS_HADDRF_SET		0x00000002
#define PM_RSTS_HADDRQ_SET		0x00000001

#define PM_RSTS_HADDR_SET \
	(PM_RSTS_HADDRQ_SET | PM_RSTS_HADDRF_SET | PM_RSTS_HADDRH_SET)
#define PM_RSTS_HADWR_SET \
	(PM_RSTS_HADWRQ_SET | PM_RSTS_HADWRF_SET | PM_RSTS_HADWRH_SET)
#define PM_RSTS_HADSR_SET \
	(PM_RSTS_HADSRQ_SET | PM_RSTS_HADSRF_SET | PM_RSTS_HADSRH_SET)

/*
 * The Raspberry Pi firmware uses the RSTS register to know which partition
 * to boot from. The partition value is spread into bits 0, 2, 4, 6, 8, 10.
 * Partition 63 is a special partition used by the firmware to indicate halt.
 */
#define PM_RSTS_RASPBERRYPI_HALT	0x555

#endif

/*
 * (C) Copyright 2012 Juergen Beisert - <kernel@pengutronix.de>
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

#include <common.h>
#include <init.h>
#include <io.h>
#include <reset_source.h>
#include <mach/imx-regs.h>

#ifdef CONFIG_ARCH_IMX1
#  define IMX_RESET_SRC_WDOG (1 << 1)
#  define IMX_RESET_SRC_HRDRESET (1 << 0)
/* let the compiler sort out useless code on this arch */
#  define IMX_RESET_SRC_WARMSTART 0
#  define IMX_RESET_SRC_COLDSTART 0
#else
  /* WRSR checked for i.MX25, i.MX27, i.MX31, i.MX35 and i.MX51 */
# define WDOG_WRSR 0x04
  /* valid for i.MX25, i.MX27, i.MX31, i.MX35, i.MX51 */
#  define IMX_RESET_SRC_WARMSTART (1 << 0)
  /* valid for i.MX25, i.MX27, i.MX31, i.MX35, i.MX51 */
#  define IMX_RESET_SRC_WDOG (1 << 1)
  /* valid for i.MX27, i.MX31, always '0' on i.MX25, i.MX35, i.MX51 */
#  define IMX_RESET_SRC_HRDRESET (1 << 3)
  /* valid for i.MX27, i.MX31, always '0' on i.MX25, i.MX35, i.MX51 */
#  define IMX_RESET_SRC_COLDSTART (1 << 4)
#endif

static unsigned read_detection_register(void)
{
#ifdef CONFIG_ARCH_IMX1
	return readl(IMX_SYSCTRL_BASE);
#else
	return readw(IMX_WDT_BASE + WDOG_WRSR);
#endif
}

static int imx_detect_reset_source(void)
{
	unsigned reg = read_detection_register();

	if (reg & IMX_RESET_SRC_COLDSTART) {
		set_reset_source(RESET_POR);
		return 0;
	}

	if (reg & (IMX_RESET_SRC_HRDRESET | IMX_RESET_SRC_WARMSTART)) {
		set_reset_source(RESET_RST);
		return 0;
	}

	if (reg & IMX_RESET_SRC_WDOG) {
		set_reset_source(RESET_WDG);
		return 0;
	}

	/* else keep the default 'unknown' state */
	return 0;
}

device_initcall(imx_detect_reset_source);

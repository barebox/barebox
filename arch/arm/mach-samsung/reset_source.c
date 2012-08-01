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
#include <mach/s3c-iomap.h>

/* S3C2440 relevant */
#define S3C2440_GSTATUS2 0xb4
# define S3C2440_GSTATUS2_PWRST (1 << 0)
# define S3C2440_GSTATUS2_SLEEPRST (1 << 1)
# define S3C2440_GSTATUS2_WDRST (1 << 2)

static int s3c_detect_reset_source(void)
{
	u32 reg = readl(S3C_GPIO_BASE + S3C2440_GSTATUS2);

	if (reg & S3C2440_GSTATUS2_PWRST) {
		set_reset_source(RESET_POR);
		writel(S3C2440_GSTATUS2_PWRST,
					S3C_GPIO_BASE + S3C2440_GSTATUS2);
		return 0;
	}

	if (reg & S3C2440_GSTATUS2_SLEEPRST) {
		set_reset_source(RESET_WKE);
		writel(S3C2440_GSTATUS2_SLEEPRST,
					S3C_GPIO_BASE + S3C2440_GSTATUS2);
		return 0;
	}

	if (reg & S3C2440_GSTATUS2_WDRST) {
		set_reset_source(RESET_WDG);
		writel(S3C2440_GSTATUS2_WDRST,
					S3C_GPIO_BASE + S3C2440_GSTATUS2);
		return 0;
	}

	/* else keep the default 'unknown' state */
	return 0;
}

device_initcall(s3c_detect_reset_source);

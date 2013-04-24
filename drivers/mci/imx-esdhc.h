/*
 * FSL SD/MMC Defines
 *-------------------------------------------------------------------
 *
 * Copyright 2007-2008,2010 Freescale Semiconductor, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 *
 *-------------------------------------------------------------------
 *
 */

#ifndef  __FSL_ESDHC_H__
#define	__FSL_ESDHC_H__

#include <errno.h>
#include <asm/byteorder.h>

#define SYSCTL_INITA		0x08000000
#define SYSCTL_TIMEOUT_MASK	0x000f0000
#define SYSCTL_CLOCK_MASK	0x0000fff0
#define SYSCTL_RSTA		0x01000000
#define SYSCTL_CKEN		0x00000008
#define SYSCTL_PEREN		0x00000004
#define SYSCTL_HCKEN		0x00000002
#define SYSCTL_IPGEN		0x00000001

#define CMD_ERR		(IRQSTAT_CIE | IRQSTAT_CEBE | IRQSTAT_CCE)
#define DATA_ERR	(IRQSTAT_DEBE | IRQSTAT_DCE | IRQSTAT_DTOE)

#define PROCTL_INIT		0x00000020
#define PROCTL_DTW_4		0x00000002
#define PROCTL_DTW_8		0x00000004

#define WML_WRITE	0x00010000
#define WML_RD_WML_MASK	0xff
#define WML_WR_WML_MASK	0xff0000

#define BLKATTR_CNT(x)	((x & 0xffff) << 16)
#define BLKATTR_SIZE(x)	(x & 0x1fff)
#define MAX_BLK_CNT	0x7fff	/* so malloc will have enough room with 32M */

#define ESDHC_HOSTCAPBLT_VS18	0x04000000
#define ESDHC_HOSTCAPBLT_VS30	0x02000000
#define ESDHC_HOSTCAPBLT_VS33	0x01000000
#define ESDHC_HOSTCAPBLT_SRS	0x00800000
#define ESDHC_HOSTCAPBLT_DMAS	0x00400000
#define ESDHC_HOSTCAPBLT_HSS	0x00200000

struct fsl_esdhc_cfg {
	u32	esdhc_base;
	u32	no_snoop;
};

#define esdhc_read32(a)			readl(a)
#define esdhc_write32(a, v)		writel(v,a)
#define esdhc_clrsetbits32(a, c, s)	writel((readl(a) & ~(c)) | (s), (a))
#define esdhc_clrbits32(a, c)		writel(readl(a) & ~(c), (a))
#define esdhc_setbits32(a, s)		writel(readl(a) | (s), (a))

#endif  /* __FSL_ESDHC_H__ */

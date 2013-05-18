/*
 * (c) 2009 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
 */

#ifndef __MACH_IMX_IIM_H
#define __MACH_IMX_IIM_H

#include <errno.h>
#include <net.h>

#define IIM_STAT	0x0000
#define IIM_STATM	0x0004
#define IIM_ERR		0x0008
#define IIM_EMASK	0x000C
#define IIM_FCTL	0x0010
#define IIM_UA		0x0014
#define IIM_LA		0x0018
#define IIM_SDAT	0x001C
#define IIM_PREV	0x0020
#define IIM_SREV	0x0024
#define IIM_PREG_P	0x0028
#define IIM_SCS0	0x002C
#define IIM_SCS1	0x0030
#define IIM_SCS2	0x0034
#define IIM_SCS3	0x0038

#ifdef CONFIG_IMX_IIM
int imx_iim_read(unsigned int bank, int offset, void *buf, int count);
#else
static inline int imx_iim_read(unsigned int bank, int offset, void *buf,
		int count)
{
	return -EINVAL;
}
#endif /* CONFIG_IMX_IIM */

static inline int imx51_iim_register_fec_ethaddr(void)
{
	int ret;
	u8 buf[6];

	ret = imx_iim_read(1, 9, buf, 6);
	if (ret != 6)
		return -EINVAL;

	eth_register_ethaddr(0, buf);

	return 0;
}

static inline int imx53_iim_register_fec_ethaddr(void)
{
	return imx51_iim_register_fec_ethaddr();
}

static inline int imx25_iim_register_fec_ethaddr(void)
{
	int ret;
	u8 buf[6];

	ret = imx_iim_read(0, 26, buf, 6);
	if (ret != 6)
		return -EINVAL;

	eth_register_ethaddr(0, buf);

	return 0;
}

#endif /* __MACH_IMX_IIM_H */

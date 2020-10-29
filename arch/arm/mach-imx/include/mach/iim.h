/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix */

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

#define IIM_BANK_MASK_WIDTH	3
#define IIM_BANK_MASK_SHIFT	0
#define IIM_BANK(n)		(((n) & ((1 << IIM_BANK_MASK_WIDTH) - 1)) << IIM_BANK_MASK_SHIFT)

#define IIM_BYTE_MASK_WIDTH	5
#define IIM_BYTE_MASK_SHIFT	IIM_BANK_MASK_WIDTH
#define IIM_BYTE(n)		((((n) >> 2) & ((1 << IIM_BYTE_MASK_WIDTH) - 1)) << IIM_BYTE_MASK_SHIFT)

#define IIM_BIT_MASK_WIDTH	3
#define IIM_BIT_MASK_SHIFT	(IIM_BYTE_MASK_SHIFT + IIM_BYTE_MASK_WIDTH)
#define IIM_BIT(n)		(((n) & ((1 << IIM_BIT_MASK_WIDTH) - 1)) << IIM_BIT_MASK_SHIFT)

#define IIM_WIDTH_MASK_WIDTH	3
#define IIM_WIDTH_MASK_SHIFT	(IIM_BIT_MASK_SHIFT + IIM_BIT_MASK_WIDTH)
#define IIM_WIDTH(n)		((((n) - 1) & ((1 << IIM_WIDTH_MASK_WIDTH) - 1)) << IIM_WIDTH_MASK_SHIFT)

int imx_iim_read_field(uint32_t field, unsigned *value);
int imx_iim_write_field(uint32_t field, unsigned value);
int imx_iim_permanent_write(int enable);

#endif /* __MACH_IMX_IIM_H */

/*
 * Copyright (c) 2009 Daniel Mack <daniel@caiaq.de>
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <errno.h>
#include <usb/chipidea-imx.h>
#include <mach/imx6-regs.h>
#include <mach/iomux-mx6.h>

#define MX25_OTG_SIC_SHIFT	29
#define MX25_OTG_SIC_MASK	(0x3 << MX25_OTG_SIC_SHIFT)
#define MX25_OTG_PM_BIT		(1 << 24)
#define MX25_OTG_PP_BIT		(1 << 11)
#define MX25_OTG_OCPOL_BIT	(1 << 3)

#define MX25_H1_SIC_SHIFT	21
#define MX25_H1_SIC_MASK	(0x3 << MX25_H1_SIC_SHIFT)
#define MX25_H1_PP_BIT		(1 << 18)
#define MX25_H1_PM_BIT		(1 << 16)
#define MX25_H1_IPPUE_UP_BIT	(1 << 7)
#define MX25_H1_IPPUE_DOWN_BIT	(1 << 6)
#define MX25_H1_TLL_BIT		(1 << 5)
#define MX25_H1_USBTE_BIT	(1 << 4)
#define MX25_H1_OCPOL_BIT	(1 << 2)

struct imx_usb_misc_data {
	int (*init)(void __iomem *base, int port, unsigned int flags);
	int (*post_init)(void __iomem *base, int port, unsigned int flags);
};

struct imx_usb_misc_priv {
	struct imx_usb_misc_data *data;
	void __iomem *base;
};

static __maybe_unused int mx25_initialize_usb_hw(void __iomem *base, int port, unsigned int flags)
{
	unsigned int v;

	v = readl(base);

	switch (port) {
	case 0:	/* OTG port */
		v &= ~(MX25_OTG_SIC_MASK | MX25_OTG_PM_BIT | MX25_OTG_PP_BIT |
			MX25_OTG_OCPOL_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX25_OTG_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX25_OTG_PM_BIT;

		if (flags & MXC_EHCI_PWR_PIN_ACTIVE_HIGH)
			v |= MX25_OTG_PP_BIT;

		if (!(flags & MXC_EHCI_OC_PIN_ACTIVE_LOW))
			v |= MX25_OTG_OCPOL_BIT;

		break;
	case 1: /* H1 port */
		v &= ~(MX25_H1_SIC_MASK | MX25_H1_PM_BIT | MX25_H1_PP_BIT |
			MX25_H1_OCPOL_BIT | MX25_H1_TLL_BIT | MX25_H1_USBTE_BIT |
			MX25_H1_IPPUE_DOWN_BIT | MX25_H1_IPPUE_UP_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX25_H1_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX25_H1_PM_BIT;

		if (flags & MXC_EHCI_PWR_PIN_ACTIVE_HIGH)
			v |= MX25_H1_PP_BIT;

		if (!(flags & MXC_EHCI_OC_PIN_ACTIVE_LOW))
			v |= MX25_H1_OCPOL_BIT;

		if (!(flags & MXC_EHCI_TLL_ENABLED))
			v |= MX25_H1_TLL_BIT;

		if (flags & MXC_EHCI_INTERNAL_PHY)
			v |= MX25_H1_USBTE_BIT;

		if (flags & MXC_EHCI_IPPUE_DOWN)
			v |= MX25_H1_IPPUE_DOWN_BIT;

		if (flags & MXC_EHCI_IPPUE_UP)
			v |= MX25_H1_IPPUE_UP_BIT;

		break;
	default:
		return -EINVAL;
	}

	writel(v, base);

	return 0;
}

static __maybe_unused struct imx_usb_misc_data mx25_data = {
	.init = mx25_initialize_usb_hw,
};

#define MX27_OTG_SIC_SHIFT	29
#define MX27_OTG_SIC_MASK	(0x3 << MX27_OTG_SIC_SHIFT)
#define MX27_OTG_PM_BIT		(1 << 24)

#define MX27_H2_SIC_SHIFT	21
#define MX27_H2_SIC_MASK	(0x3 << MX27_H2_SIC_SHIFT)
#define MX27_H2_PM_BIT		(1 << 16)
#define MX27_H2_DT_BIT		(1 << 5)

#define MX27_H1_SIC_SHIFT	13
#define MX27_H1_SIC_MASK	(0x3 << MX27_H1_SIC_SHIFT)
#define MX27_H1_PM_BIT		(1 << 8)
#define MX27_H1_DT_BIT		(1 << 4)

static __maybe_unused int mx27_mx31_initialize_usb_hw(void __iomem *base, int port, unsigned int flags)
{
	unsigned int v;

	v = readl(base);

	switch (port) {
	case 0:	/* OTG port */
		v &= ~(MX27_OTG_SIC_MASK | MX27_OTG_PM_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX27_OTG_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX27_OTG_PM_BIT;
		break;
	case 1: /* H1 port */
		v &= ~(MX27_H1_SIC_MASK | MX27_H1_PM_BIT | MX27_H1_DT_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX27_H1_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX27_H1_PM_BIT;

		if (!(flags & MXC_EHCI_TLL_ENABLED))
			v |= MX27_H1_DT_BIT;

		break;
	case 2:	/* H2 port */
		v &= ~(MX27_H2_SIC_MASK | MX27_H2_PM_BIT | MX27_H2_DT_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX27_H2_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX27_H2_PM_BIT;

		if (!(flags & MXC_EHCI_TLL_ENABLED))
			v |= MX27_H2_DT_BIT;

		break;
	default:
		return -EINVAL;
	}

	writel(v, base);

	return 0;
}

static __maybe_unused struct imx_usb_misc_data mx27_mx31_data = {
	.init = mx27_mx31_initialize_usb_hw,
};

#define USBCTRL_OTGBASE_OFFSET	0x600

#define MX35_OTG_SIC_SHIFT	29
#define MX35_OTG_SIC_MASK	(0x3 << MX35_OTG_SIC_SHIFT)
#define MX35_OTG_PM_BIT		(1 << 24)
#define MX35_OTG_PP_BIT		(1 << 11)
#define MX35_OTG_OCPOL_BIT	(1 << 3)

#define MX35_H1_SIC_SHIFT	21
#define MX35_H1_SIC_MASK	(0x3 << MX35_H1_SIC_SHIFT)
#define MX35_H1_PP_BIT		(1 << 18)
#define MX35_H1_PM_BIT		(1 << 8)
#define MX35_H1_IPPUE_UP_BIT	(1 << 7)
#define MX35_H1_IPPUE_DOWN_BIT	(1 << 6)
#define MX35_H1_TLL_BIT		(1 << 5)
#define MX35_H1_USBTE_BIT	(1 << 4)
#define MX35_H1_OCPOL_BIT	(1 << 2)

static __maybe_unused int mx35_initialize_usb_hw(void __iomem *base, int port, unsigned int flags)
{
	unsigned int v;

	v = readl(base);

	switch (port) {
	case 0:	/* OTG port */
		v &= ~(MX35_OTG_SIC_MASK | MX35_OTG_PM_BIT | MX35_OTG_PP_BIT |
			MX35_OTG_OCPOL_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX35_OTG_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX35_OTG_PM_BIT;

		if (flags & MXC_EHCI_PWR_PIN_ACTIVE_HIGH)
			v |= MX35_OTG_PP_BIT;

		if (!(flags & MXC_EHCI_OC_PIN_ACTIVE_LOW))
			v |= MX35_OTG_OCPOL_BIT;

		break;
	case 1: /* H1 port */
		v &= ~(MX35_H1_SIC_MASK | MX35_H1_PM_BIT | MX35_H1_PP_BIT |
			MX35_H1_OCPOL_BIT | MX35_H1_TLL_BIT | MX35_H1_USBTE_BIT |
			MX35_H1_IPPUE_DOWN_BIT | MX35_H1_IPPUE_UP_BIT);
		v |= (flags & MXC_EHCI_INTERFACE_MASK) << MX35_H1_SIC_SHIFT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX35_H1_PM_BIT;

		if (flags & MXC_EHCI_PWR_PIN_ACTIVE_HIGH)
			v |= MX35_H1_PP_BIT;

		if (!(flags & MXC_EHCI_OC_PIN_ACTIVE_LOW))
			v |= MX35_H1_OCPOL_BIT;

		if (!(flags & MXC_EHCI_TLL_ENABLED))
			v |= MX35_H1_TLL_BIT;

		if (flags & MXC_EHCI_INTERNAL_PHY)
			v |= MX35_H1_USBTE_BIT;

		if (flags & MXC_EHCI_IPPUE_DOWN)
			v |= MX35_H1_IPPUE_DOWN_BIT;

		if (flags & MXC_EHCI_IPPUE_UP)
			v |= MX35_H1_IPPUE_UP_BIT;

		break;
	default:
		return -EINVAL;
	}

	writel(v, base);

	return 0;
}

static __maybe_unused struct imx_usb_misc_data mx35_data = {
	.init = mx35_initialize_usb_hw,
};

/* USB_CTRL */
#define MX5_OTG_UCTRL_OWIE_BIT		(1 << 27)	/* OTG wakeup intr enable */
#define MX5_OTG_UCTRL_OPM_BIT		(1 << 24)	/* OTG power mask */
#define MX5_H1_UCTRL_H1UIE_BIT		(1 << 12)	/* Host1 ULPI interrupt enable */
#define MX5_H1_UCTRL_H1WIE_BIT		(1 << 11)	/* HOST1 wakeup intr enable */
#define MX5_H1_UCTRL_H1PM_BIT		(1 <<  8)	/* HOST1 power mask */

/* USB_PHY_CTRL_FUNC */
#define MX5_OTG_PHYCTRL_OC_POL_BIT	(1 << 9)	/* OTG Polarity of Overcurrent */
#define MX5_OTG_PHYCTRL_OC_DIS_BIT	(1 << 8)	/* OTG Disable Overcurrent Event */
#define MX5_H1_OC_POL_BIT		(1 << 6)	/* UH1 Polarity of Overcurrent */
#define MX5_H1_OC_DIS_BIT		(1 << 5)	/* UH1 Disable Overcurrent Event */
#define MX5_OTG_PHYCTRL_PWR_POL_BIT	(1 << 3)	/* OTG Power Pin Polarity */

/* USBH2CTRL */
#define MX5_H2_UCTRL_H2UIE_BIT		(1 << 8)
#define MX5_H2_UCTRL_H2WIE_BIT		(1 << 7)
#define MX5_H2_UCTRL_H2PM_BIT		(1 << 4)

#define MX5_USBCTRL_OFFSET		0x0
#define MX5_UTMI_PHY_CTRL_0		0x8
#define MX5_UTMI_PHY_CTRL_1		0xc
#define MX5_USBH2CTRL_OFFSET		0x14

static __maybe_unused int mx5_initialize_usb_hw(void __iomem *base, int port,
		unsigned int flags)
{
	unsigned int v;

	switch (port) {
	case 0:	/* OTG port */
		/* Adjust UTMI PHY frequency to 24MHz */
		v = readl(base + MX5_UTMI_PHY_CTRL_1);
		v = (v & ~0x3) | 0x01;
		writel(v, base + MX5_UTMI_PHY_CTRL_1);

		v = readl(base + MX5_UTMI_PHY_CTRL_0);
		v &= ~MX5_OTG_PHYCTRL_OC_POL_BIT;
		v &= ~MX5_OTG_PHYCTRL_OC_DIS_BIT;
		v &= ~MX5_OTG_PHYCTRL_PWR_POL_BIT;

		if (flags & MXC_EHCI_OC_PIN_ACTIVE_LOW)
			v |= MX5_OTG_PHYCTRL_OC_POL_BIT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX5_OTG_PHYCTRL_OC_DIS_BIT;

		if (flags & MXC_EHCI_PWR_PIN_ACTIVE_HIGH)
			v |= MX5_OTG_PHYCTRL_PWR_POL_BIT;

		writel(v, base + MX5_UTMI_PHY_CTRL_0);

		v = readl(base + MX5_USBCTRL_OFFSET);
		v &= ~MX5_OTG_UCTRL_OWIE_BIT;
		v &= ~MX5_OTG_UCTRL_OPM_BIT;

		if (flags & MXC_EHCI_WAKEUP_ENABLED)
			v |= MX5_OTG_UCTRL_OWIE_BIT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX5_OTG_UCTRL_OPM_BIT;

		writel(v, base + MX5_USBCTRL_OFFSET);

		break;
	case 1:	/* H1 port */
		v = readl(base + MX5_USBCTRL_OFFSET);
		v &= ~MX5_H1_UCTRL_H1PM_BIT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX5_H1_UCTRL_H1PM_BIT;

		writel(v, base + MX5_USBCTRL_OFFSET);

		break;
	case 2: /* H2 port */
		v = readl(base + MX5_USBH2CTRL_OFFSET);
		v &= ~MX5_H2_UCTRL_H2PM_BIT;

		if (!(flags & MXC_EHCI_POWER_PINS_ENABLED))
			v |= MX5_H2_UCTRL_H2PM_BIT;

		writel(v, base + MX5_USBH2CTRL_OFFSET);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static __maybe_unused struct imx_usb_misc_data mx5_data = {
	.init = mx5_initialize_usb_hw,
};

#define MX6_USB_CTRL(n)			((n) * 4)
#define MX6_USB_CTRL_OVER_CUR_DIS	(1 << 7)
#define MX6_USB_CTRL_OVER_CUR_ACT_HIGH	(1 << 8)

static void mx6_hsic_pullup(unsigned long reg, int on)
{
	u32 val;

	val = readl(MX6_IOMUXC_BASE_ADDR + reg);

	if (on)
		val |= MX6_PAD_CTL_PUS_47K_UP;
	else
		val &= ~MX6_PAD_CTL_PUS_47K_UP;

	writel(val, MX6_IOMUXC_BASE_ADDR + reg);
}

static __maybe_unused int mx6_initialize_usb_hw(void __iomem *base, int port,
		unsigned int flags)
{
	u32 val;

	switch (port) {
	case 0:
	case 1:
		val = readl(base + MX6_USB_CTRL(port));
		if (flags & MXC_EHCI_DISABLE_OVERCURRENT)
			val |= MX6_USB_CTRL_OVER_CUR_DIS;
		writel(val, base + MX6_USB_CTRL(port));
		break;
	case 2: /* HSIC port */
		mx6_hsic_pullup(0x388, 0);

		writel(0x00003000, base + 0x8);
		writel(0x80001842, base + 0x10);

		break;
	case 3: /* HSIC port */
		writel(0x00003000, base + 0xc);
		writel(0x80001842, base + 0x14);

		mx6_hsic_pullup(0x398, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static __maybe_unused int mx6_post_init(void __iomem *base, int port,
		unsigned int flags)
{
	switch (port) {
	case 0:
		break;
	case 1:
		break;
	case 2: /* HSIC port */
		mx6_hsic_pullup(0x388, 1);
		break;
	case 3: /* HSIC port */
		mx6_hsic_pullup(0x398, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static __maybe_unused struct imx_usb_misc_data mx6_data = {
	.init = mx6_initialize_usb_hw,
	.post_init = mx6_post_init,
};

#define MX7D_USBNC_USB_CTRL2		0x4
#define MX7D_USB_VBUS_WAKEUP_SOURCE_MASK	0x3
#define MX7D_USB_VBUS_WAKEUP_SOURCE(v)		(v << 0)
#define MX7D_USB_VBUS_WAKEUP_SOURCE_VBUS	MX7D_USB_VBUS_WAKEUP_SOURCE(0)
#define MX7D_USB_VBUS_WAKEUP_SOURCE_AVALID	MX7D_USB_VBUS_WAKEUP_SOURCE(1)
#define MX7D_USB_VBUS_WAKEUP_SOURCE_BVALID	MX7D_USB_VBUS_WAKEUP_SOURCE(2)
#define MX7D_USB_VBUS_WAKEUP_SOURCE_SESS_END	MX7D_USB_VBUS_WAKEUP_SOURCE(3)

static int usbmisc_imx7d_init(void __iomem *base, int port,
		unsigned int flags)
{
	u32 reg;

	if (port >= 1)
		return -EINVAL;

	reg = readl(base);
	if (flags & MXC_EHCI_DISABLE_OVERCURRENT) {
		reg |= MX6_USB_CTRL_OVER_CUR_DIS;
	} else {
		reg &= ~MX6_USB_CTRL_OVER_CUR_DIS;
		if (flags & MXC_EHCI_OC_PIN_ACTIVE_LOW)
			reg &= ~MX6_USB_CTRL_OVER_CUR_ACT_HIGH;
		else
			reg |= MX6_USB_CTRL_OVER_CUR_ACT_HIGH;
	}
	writel(reg, base);

	reg = readl(base + MX7D_USBNC_USB_CTRL2);
	reg &= ~MX7D_USB_VBUS_WAKEUP_SOURCE_MASK;
	writel(reg | MX7D_USB_VBUS_WAKEUP_SOURCE_BVALID,
		 base + MX7D_USBNC_USB_CTRL2);

	return 0;
}

static __maybe_unused struct imx_usb_misc_data mx7_data = {
	.init = usbmisc_imx7d_init,
};

#define VF610_OVER_CUR_DIS		BIT(7)

static __maybe_unused int vf610_initialize_usb_hw(void __iomem *base, int port,
		unsigned int flags)
{
	u32 reg;

	if (port >= 1)
		return -EINVAL;

	if (flags & MXC_EHCI_DISABLE_OVERCURRENT) {
		reg = readl(base);
		writel(reg | VF610_OVER_CUR_DIS, base);
	}

	return 0;
}

static __maybe_unused struct imx_usb_misc_data vf610_data = {
	.init = vf610_initialize_usb_hw,
};

static struct platform_device_id imx_usbmisc_ids[] = {
#ifdef CONFIG_ARCH_IMX25
	{
		.name = "imx25-usb-misc",
		.driver_data = (unsigned long)&mx25_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX27
	{
		.name = "imx27-usb-misc",
		.driver_data = (unsigned long)&mx27_mx31_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX31
	{
		.name = "imx31-usb-misc",
		.driver_data = (unsigned long)&mx27_mx31_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX35
	{
		.name = "imx35-usb-misc",
		.driver_data = (unsigned long)&mx35_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX50
	{
		.name = "imx50-usb-misc",
		.driver_data = (unsigned long)&mx5_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX51
	{
		.name = "imx51-usb-misc",
		.driver_data = (unsigned long)&mx5_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX53
	{
		.name = "imx53-usb-misc",
		.driver_data = (unsigned long)&mx5_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX6
	{
		.name = "imx6-usb-misc",
		.driver_data = (unsigned long)&mx6_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX7
	{
		.name = "imx7d-usb-misc",
		.driver_data = (unsigned long)&mx7_data,
	},
#endif
	{
                /* sentinel */
	},
};

static __maybe_unused struct of_device_id imx_usbmisc_dt_ids[] = {
#ifdef CONFIG_ARCH_IMX25
	{
		.compatible = "fsl,imx25-usbmisc",
		.data = &mx25_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX27
	{
		.compatible = "fsl,imx27-usbmisc",
		.data = &mx27_mx31_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX31
	{
		.compatible = "fsl,imx31-usbmisc",
		.data = &mx27_mx31_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX35
	{
		.compatible = "fsl,imx35-usbmisc",
		.data = &mx35_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX51
	{
		.compatible = "fsl,imx51-usbmisc",
		.data = &mx5_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX53
	{
		.compatible = "fsl,imx53-usbmisc",
		.data = &mx5_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX6
	{
		.compatible = "fsl,imx6q-usbmisc",
		.data = &mx6_data,
	},
#endif
#ifdef CONFIG_ARCH_IMX7
	{
		.compatible = "fsl,imx7d-usbmisc",
		.data = &mx7_data,
	},
#endif
#ifdef CONFIG_ARCH_VF610
	{
		.compatible = "fsl,vf610-usbmisc",
		.data = &vf610_data,
	},
#endif
	{
		/* sentinel */
	},
};

int imx_usbmisc_port_init(struct device_d *dev, int port, unsigned flags)
{
	struct imx_usb_misc_priv *usbmisc = dev->priv;

	if (!usbmisc)
		return -ENODEV;

	if (!usbmisc->data->init)
		return 0;

	return usbmisc->data->init(usbmisc->base, port, flags);
}

int imx_usbmisc_port_post_init(struct device_d *dev, int port, unsigned flags)
{
	struct imx_usb_misc_priv *usbmisc = dev->priv;

	if (!usbmisc)
		return -ENODEV;

	if (!usbmisc->data->post_init)
		return 0;

	return usbmisc->data->post_init(usbmisc->base, port, flags);
}

static int imx_usbmisc_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imx_usb_misc_data *devtype;
	struct imx_usb_misc_priv *usbmisc;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	usbmisc = xzalloc(sizeof(*usbmisc));
	usbmisc->base = IOMEM(iores->start);
	usbmisc->data = devtype;

	dev->priv = usbmisc;

	return 0;
}

static struct driver_d imx_usbmisc_driver = {
	.name   = "imx-usbmisc",
	.probe  = imx_usbmisc_probe,
	.id_table = imx_usbmisc_ids,
	.of_compatible = DRV_OF_COMPAT(imx_usbmisc_dt_ids),
};

static int imx_usbmisc_init(void)
{
	platform_driver_register(&imx_usbmisc_driver);
	return 0;
}

coredevice_initcall(imx_usbmisc_init);

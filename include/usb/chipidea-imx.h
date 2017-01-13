#ifndef __USB_CHIPIDEA_IMX_H
#define __USB_CHIPIDEA_IMX_H

#include <usb/phy.h>

/*
 * POTSC flags
 */
#define MXC_EHCI_SERIAL			(1 << 29)
#define MXC_EHCI_MODE_UTMI_8BIT		(0 << 30)
#define MXC_EHCI_MODE_UTMI_16_BIT	((0 << 30) | (1 << 28))
#define MXC_EHCI_MODE_PHILIPS		(1 << 30)
#define MXC_EHCI_MODE_ULPI		(2 << 30)
#define MXC_EHCI_MODE_HSIC		(1 << 25)
#define MXC_EHCI_MODE_SERIAL		(3 << 30)

/*
 * USB misc flags
 */
#define MXC_EHCI_INTERFACE_DIFF_UNI	(0 << 0)
#define MXC_EHCI_INTERFACE_DIFF_BI	(1 << 0)
#define MXC_EHCI_INTERFACE_SINGLE_UNI	(2 << 0)
#define MXC_EHCI_INTERFACE_SINGLE_BI	(3 << 0)
#define MXC_EHCI_INTERFACE_MASK		(0xf)

#define MXC_EHCI_POWER_PINS_ENABLED	(1 << 5)
#define MXC_EHCI_PWR_PIN_ACTIVE_HIGH	(1 << 6)
#define MXC_EHCI_OC_PIN_ACTIVE_LOW	(1 << 7)
#define MXC_EHCI_TLL_ENABLED		(1 << 8)

#define MXC_EHCI_INTERNAL_PHY		(1 << 9)
#define MXC_EHCI_IPPUE_DOWN		(1 << 10)
#define MXC_EHCI_IPPUE_UP		(1 << 11)
#define MXC_EHCI_WAKEUP_ENABLED		(1 << 12)
#define MXC_EHCI_ITC_NO_THRESHOLD	(1 << 13)

#define MXC_EHCI_DISABLE_OVERCURRENT	(1 << 14)

enum imx_usb_mode {
	IMX_USB_MODE_HOST,
	IMX_USB_MODE_DEVICE,
	IMX_USB_MODE_OTG,
};

struct imxusb_platformdata {
	unsigned long flags;
	enum usb_phy_interface phymode;
	enum imx_usb_mode mode;
};

int imx_usbmisc_port_init(struct device_d *dev, int port, unsigned flags);
int imx_usbmisc_port_post_init(struct device_d *dev, int port, unsigned flags);

#endif /* __USB_CHIPIDEA_IMX_H */

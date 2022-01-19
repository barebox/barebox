/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __USB_EHCI_H
#define __USB_EHCI_H

#define EHCI_HAS_TT	(1 << 0)

struct ehci_platform_data {
	unsigned long flags;
};

struct ehci_data {
	void __iomem *hccr;
	void __iomem *hcor;
	unsigned long flags;
	struct usb_phy *usbphy;

	/* platform specific init functions */
	int (*init)(void *drvdata);
	int (*post_init)(void *drvdata);
	void *drvdata;
};

struct ehci_host;

#ifdef CONFIG_USB_EHCI
struct ehci_host *ehci_register(struct device_d *dev, struct ehci_data *data);
void ehci_unregister(struct ehci_host *);
#else
static inline struct ehci_host *ehci_register(struct device_d *dev,
					      struct ehci_data *data)
{
	return ERR_PTR(-ENOSYS);
}

static inline void ehci_unregister(struct ehci_host *ehci)
{
}
#endif

#endif  /* __USB_EHCI_H */

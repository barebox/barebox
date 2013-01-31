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

	/* platform specific init functions */
	int (*init)(void *drvdata);
	int (*post_init)(void *drvdata);
	void *drvdata;
};

#ifdef CONFIG_USB_EHCI
int ehci_register(struct device_d *dev, struct ehci_data *data);
#else
static inline int ehci_register(struct device_d *dev, struct ehci_data *data)
{
	return -ENOSYS;
}
#endif

#endif  /* __USB_EHCI_H */

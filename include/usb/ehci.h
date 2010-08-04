#ifndef __USB_EHCI_H
#define __USB_EHCI_H

#define EHCI_HAS_TT	(1 << 0)

struct ehci_platform_data {
	unsigned long flags;
	unsigned long hccr_offset;
	unsigned long hcor_offset;
};

#endif  /* __USB_EHCI_H */

/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _USB_SERIAL_H
#define _USB_SERIAL_H

struct usb_serial_pdata {
	bool			acm;
};

#ifdef CONFIG_USB_GADGET_SERIAL
int usb_serial_register(struct usb_serial_pdata *pdata);
void usb_serial_unregister(void);
#else
static inline int usb_serial_register(struct usb_serial_pdata *pdata)
{
	return -ENOSYS;
}

static inline void usb_serial_unregister(void)
{
}
#endif

#endif /* _USB_SERIAL_H */

#ifndef _USB_SERIAL_H
#define _USB_SERIAL_H

struct usb_serial_pdata {
	char		*manufacturer;
	const char		*productname;
	u16			idVendor;
	u16			idProduct;
	bool			acm;
};

int usb_serial_register(struct usb_serial_pdata *pdata);
void usb_serial_unregister(void);

#endif /* _USB_SERIAL_H */

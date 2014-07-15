#ifndef __CORE_USB_H
#define __CORE_USB_H

struct usb_device *usb_alloc_new_device(void);
int usb_new_device(struct usb_device *dev);
void usb_remove_device(struct usb_device *dev);

#endif /* __CORE_USB_H */

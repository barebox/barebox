/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __CORE_USB_H
#define __CORE_USB_H

struct usb_device *usb_alloc_new_device(void);
void usb_free_device(struct usb_device *dev);
int usb_new_device(struct usb_device *dev);
void usb_add_device(struct usb_device *dev);
void usb_remove_device(struct usb_device *dev);
void usb_hub_cancel_scans(struct usb_device *dev);
int usb_hub_port_connected(struct usb_device *hub, int port);
void usb_set_maxpacket_ep(struct usb_device *dev,
			  struct usb_endpoint_descriptor *ep);

#endif /* __CORE_USB_H */

#ifndef __CORE_HUB_H
#define __CORE_HUB_H

int hub_port_reset(struct usb_device *hub, int port,
		   struct usb_device *usb);

#endif /* __CORE_HUB_H */

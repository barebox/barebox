#ifndef __CORE_HUB_H
#define __CORE_HUB_H

int hub_port_reset(struct usb_device *dev, int port,
			unsigned short *portstat);

#endif /* __CORE_HUB_H */

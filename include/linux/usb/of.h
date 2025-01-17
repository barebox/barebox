// SPDX-License-Identifier: GPL-2.0
/*
 * OF helpers for usb devices.
 */

#ifndef __LINUX_USB_OF_H
#define __LINUX_USB_OF_H

#if IS_ENABLED(CONFIG_OFDEVICE) && IS_ENABLED(CONFIG_USB)
enum usb_phy_interface of_usb_get_phy_mode(struct device_node *np,
					   const char *propname);
#else
static inline enum usb_phy_interface of_usb_get_phy_mode(struct device_node *np,
							 const char *propname)
{
	return USBPHY_INTERFACE_MODE_UNKNOWN;
}
#endif

#endif /* __LINUX_USB_OF_H */

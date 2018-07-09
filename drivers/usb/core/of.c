/*
 * usb devicetree helper functions
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <usb/usb.h>
#include <usb/phy.h>
#include <of.h>

static const char *usb_dr_modes[] = {
	[USB_DR_MODE_UNKNOWN]		= "",
	[USB_DR_MODE_HOST]		= "host",
	[USB_DR_MODE_PERIPHERAL]	= "peripheral",
	[USB_DR_MODE_OTG]		= "otg",
};

/**
 * of_usb_get_dr_mode - Get dual role mode for given device_node
 * @np:	Pointer to the given device_node
 *
 * The function gets phy interface string from property 'dr_mode',
 * and returns the correspondig enum usb_dr_mode
 */
enum usb_dr_mode of_usb_get_dr_mode(struct device_node *np,
		const char *propname)
{
	const char *dr_mode;
	int err, i;

	if (!propname)
		propname = "dr_mode";

	err = of_property_read_string(np, propname, &dr_mode);
	if (err < 0)
		return USB_DR_MODE_UNKNOWN;

	for (i = 0; i < ARRAY_SIZE(usb_dr_modes); i++)
		if (!strcmp(dr_mode, usb_dr_modes[i]))
			return i;

	return USB_DR_MODE_UNKNOWN;
}
EXPORT_SYMBOL_GPL(of_usb_get_dr_mode);

static const char *usbphy_modes[] = {
	[USBPHY_INTERFACE_MODE_UNKNOWN]	= "",
	[USBPHY_INTERFACE_MODE_UTMI]	= "utmi",
	[USBPHY_INTERFACE_MODE_UTMIW]	= "utmi_wide",
	[USBPHY_INTERFACE_MODE_ULPI]	= "ulpi",
	[USBPHY_INTERFACE_MODE_SERIAL]	= "serial",
	[USBPHY_INTERFACE_MODE_HSIC]	= "hsic",
};

/**
 * of_usb_get_phy_mode - Get phy mode for given device_node
 * @np:	Pointer to the given device_node
 *
 * The function gets phy interface string from property 'phy_type',
 * and returns the correspondig enum usb_phy_interface
 */
enum usb_phy_interface of_usb_get_phy_mode(struct device_node *np,
		const char *propname)
{
	const char *phy_type;
	int err, i;

	if (!propname)
		propname = "phy_type";

	err = of_property_read_string(np, propname, &phy_type);
	if (err < 0)
		return USBPHY_INTERFACE_MODE_UNKNOWN;

	for (i = 0; i < ARRAY_SIZE(usbphy_modes); i++)
		if (!strcmp(phy_type, usbphy_modes[i]))
			return i;

	return USBPHY_INTERFACE_MODE_UNKNOWN;
}
EXPORT_SYMBOL_GPL(of_usb_get_phy_mode);

/**
 * of_usb_get_maximum_speed - Get maximum speed for given device_node
 * @np:	Pointer to the given device_node
 *
 * The function gets maximum speed string from property 'maximum-speed',
 * and returns the correspondig enum usb_device_speed
 */
enum usb_device_speed of_usb_get_maximum_speed(struct device_node *np,
		const char *propname)
{
	const char *maximum_speed;
	int err;

	if (!propname)
		propname = "maximum-speed";

	err = of_property_read_string(np, propname, &maximum_speed);
	if (err < 0)
		return USB_SPEED_UNKNOWN;

	return usb_speed_by_string(maximum_speed);
}
EXPORT_SYMBOL_GPL(of_usb_get_maximum_speed);

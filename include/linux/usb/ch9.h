/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This file holds USB constants and structures that are needed for
 * USB device APIs.  These are used by the USB device model, which is
 * defined in chapter 9 of the USB 2.0 specification and in the
 * Wireless USB 1.0 (spread around).  Linux has several APIs in C that
 * need these:
 *
 * - the host side Linux-USB kernel driver API;
 * - the "usbfs" user space API; and
 * - the Linux "gadget" device/peripheral side driver API.
 *
 * USB 2.0 adds an additional "On The Go" (OTG) mode, which lets systems
 * act either as a USB host or as a USB device.  That means the host and
 * device side APIs benefit from working well together.
 *
 * There's also "Wireless USB", using low power short range radios for
 * peripheral interconnection but otherwise building on the USB framework.
 *
 * Note all descriptors are declared '__attribute__((packed))' so that:
 *
 * [a] they never get padded, either internally (USB spec writers
 *     probably handled that) or externally;
 *
 * [b] so that accessing bigger-than-a-bytes fields will never
 *     generate bus errors on any platform, even when the location of
 *     its descriptor inside a bundle isn't "naturally aligned", and
 *
 * [c] for consistency, removing all doubt even when it appears to
 *     someone that the two other points are non-issues for that
 *     particular descriptor type.
 */
#ifndef __LINUX_USB_CH9_H
#define __LINUX_USB_CH9_H

#include <uapi/linux/usb/ch9.h>

/* USB 3.2 SuperSpeed Plus phy signaling rate generation and lane count */

enum usb_ssp_rate {
	USB_SSP_GEN_UNKNOWN = 0,
	USB_SSP_GEN_2x1,
	USB_SSP_GEN_1x2,
	USB_SSP_GEN_2x2,
};

extern const char *usb_speed_string(enum usb_device_speed speed);

/**
 * usb_speed_by_string() - Get speed from human readable name.
 * @string: The human readable name for the speed. If it is not one of known
 *   names, USB_SPEED_UNKNOWN will be returned.
 */
enum usb_device_speed usb_speed_by_string(const char *string);

#endif /* __LINUX_USB_CH9_H */

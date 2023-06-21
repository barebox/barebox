/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __USB_TYPEC_ALTMODE_H
#define __USB_TYPEC_ALTMODE_H

/*
 * These are the connector states (USB, Safe and Alt Mode) defined in USB Type-C
 * Specification. SVID specific connector states are expected to follow and
 * start from the value TYPEC_STATE_MODAL.
 */
enum {
	TYPEC_STATE_SAFE,	/* USB Safe State */
	TYPEC_STATE_USB,	/* USB Operation */
	TYPEC_STATE_MODAL,	/* Alternate Modes */
};

/*
 * For the muxes there is no difference between Accessory Modes and Alternate
 * Modes, so the Accessory Modes are supplied with specific modal state values
 * here. Unlike with Alternate Modes, where the mux will be linked with the
 * alternate mode device, the mux for Accessory Modes will be linked with the
 * port device instead.
 *
 * Port drivers can use TYPEC_MODE_AUDIO and TYPEC_MODE_DEBUG as the mode
 * value for typec_set_mode() when accessory modes are supported.
 *
 * USB4 also requires that the pins on the connector are repurposed, just like
 * Alternate Modes. USB4 mode is however not entered with the Enter Mode Command
 * like the Alternate Modes are, but instead with a special Enter_USB Message.
 * The Enter_USB Message can also be used for setting to connector to operate in
 * USB 3.2 or in USB 2.0 mode instead of USB4.
 *
 * The Enter_USB specific "USB Modes" are also supplied here as special modal
 * state values, just like the Accessory Modes.
 */
enum {
	TYPEC_MODE_USB2 = TYPEC_STATE_MODAL,	/* USB 2.0 mode */
	TYPEC_MODE_USB3,			/* USB 3.2 mode */
	TYPEC_MODE_USB4,			/* USB4 mode */
	TYPEC_MODE_AUDIO,			/* Audio Accessory */
	TYPEC_MODE_DEBUG,			/* Debug Accessory */
};

#endif /* __USB_TYPEC_ALTMODE_H */

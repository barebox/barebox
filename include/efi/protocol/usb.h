/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_USB_H_
#define __EFI_PROTOCOL_USB_H_

#include <efi/types.h>

struct devrequest;
struct usb_device_descriptor;
struct usb_config_descriptor;
struct usb_interface_descriptor;
struct usb_endpoint_descriptor;

enum efi_usb_data_direction {
	EFI_USB_DATA_IN,
	EFI_USB_DATA_OUT,
	EFI_USB_NO_DATA
};

//
// Error code for USB Transfer Results
//
#define EFI_USB_NOERROR 0x0000
#define EFI_USB_ERR_NOTEXECUTE 0x0001
#define EFI_USB_ERR_STALL 0x0002
#define EFI_USB_ERR_BUFFER 0x0004
#define EFI_USB_ERR_BABBLE 0x0008
#define EFI_USB_ERR_NAK 0x0010
#define EFI_USB_ERR_CRC 0x0020
#define EFI_USB_ERR_TIMEOUT 0x0040
#define EFI_USB_ERR_BITSTUFF 0x0080
#define EFI_USB_ERR_SYSTEM 0x0100

struct efi_usb_io_protocol {
	efi_status_t(EFIAPI *control_transfer)(
		struct efi_usb_io_protocol *this, struct devrequest *request,
		enum efi_usb_data_direction direction, u32 timeout, void *data,
		efi_uintn_t length, u32 *status);
	efi_status_t(EFIAPI *bulk_transfer)(struct efi_usb_io_protocol *this,
					    u8 device_endpoint, void *data,
					    efi_uintn_t *length,
					    efi_uintn_t timeout, u32 *status);
	void *usb_io_async_interrupt_transfer;
	efi_status_t(EFIAPI *sync_interrupt_transfer)(
		struct efi_usb_io_protocol *this, u8 device_endpoint,
		void *data, efi_uintn_t *length, efi_uintn_t timeout,
		u32 *status);
	void *efi_usb_io_isochronous_transfer;
	void *efi_usb_io_async_isochronous_transfer;
	efi_status_t(EFIAPI *get_device_descriptor)(
		struct efi_usb_io_protocol *this,
		struct usb_device_descriptor *desc);
	efi_status_t(EFIAPI *get_config_descriptor)(
		struct efi_usb_io_protocol *this,
		struct usb_config_descriptor *desc);
	efi_status_t(EFIAPI *get_interface_descriptor)(
		struct efi_usb_io_protocol *this,
		struct usb_interface_descriptor *desc);
	efi_status_t(EFIAPI *get_endpoint_descriptor)(
		struct efi_usb_io_protocol *this, u8 index,
		struct usb_endpoint_descriptor *desc);
	efi_status_t(EFIAPI *get_string_descriptor)(
		struct efi_usb_io_protocol *this, u16 lang_id, u8 string_id,
		wchar_t **desc);
	efi_status_t(EFIAPI *get_supported_languages)(
		struct efi_usb_io_protocol *this, u16 **lang_id_table,
		u16 *table_size);
	efi_status_t(EFIAPI *port_reset)(struct efi_usb_io_protocol *this);
};

#endif

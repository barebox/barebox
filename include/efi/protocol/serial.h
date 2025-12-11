/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_SERIAL_H_
#define __EFI_PROTOCOL_SERIAL_H_

#include <efi/types.h>

/*
 * define for Control bits, grouped by read only, write only, and read write
 *
 * Read Only
 */
#define EFI_SERIAL_CLEAR_TO_SEND        0x00000010
#define EFI_SERIAL_DATA_SET_READY       0x00000020
#define EFI_SERIAL_RING_INDICATE        0x00000040
#define EFI_SERIAL_CARRIER_DETECT       0x00000080
#define EFI_SERIAL_INPUT_BUFFER_EMPTY   0x00000100
#define EFI_SERIAL_OUTPUT_BUFFER_EMPTY  0x00000200

/*
 * Write Only
 */
#define EFI_SERIAL_REQUEST_TO_SEND      0x00000002
#define EFI_SERIAL_DATA_TERMINAL_READY  0x00000001

/*
 * Read Write
 */
#define EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE     0x00001000
#define EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE     0x00002000
#define EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE 0x00004000

typedef enum {
	DefaultParity,
	NoParity,
	EvenParity,
	OddParity,
	MarkParity,
	SpaceParity
} efi_parity_type;

typedef enum {
	DefaultStopBits,
	OneStopBit,
	OneFiveStopBits,
	TwoStopBits
} efi_stop_bits_type;

struct efi_serial_io_mode {
	uint32_t controlmask;
	uint32_t timeout;
	uint64_t baudrate;
	uint32_t receivefifodepth;
	uint32_t databits;
	uint32_t parity;
	uint32_t stopbits;
};

struct efi_serial_io_protocol {
	uint32_t revision;

	efi_status_t (EFIAPI *reset) (struct efi_serial_io_protocol *This);
	efi_status_t (EFIAPI *set_attributes) (struct efi_serial_io_protocol *This,
			uint64_t baudrate, uint32_t receivefifodepth,
			uint32_t timeout, efi_parity_type parity,
			uint8_t databits, efi_stop_bits_type stopbits);
	efi_status_t (EFIAPI *setcontrol) (struct efi_serial_io_protocol *This,
			uint32_t control);
	efi_status_t (EFIAPI *getcontrol) (struct efi_serial_io_protocol *This,
			uint32_t *control);
	efi_status_t (EFIAPI *write) (struct efi_serial_io_protocol *This,
			size_t *buffersize, void *buffer);
	efi_status_t (EFIAPI *read) (struct efi_serial_io_protocol *This,
			size_t *buffersize, void *buffer);

	struct efi_serial_io_mode *mode;
};

#endif

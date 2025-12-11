/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_I2C_H_
#define __EFI_PROTOCOL_I2C_H_

#include <efi/types.h>

/* Define EFI I2C transfer control flags */
#define EFI_I2C_FLAG_READ           0x00000001

#define EFI_I2C_ADDRESSING_10_BIT   0x80000000

/* The set_bus_frequency() EFI call doesn't work (doesn't alter SPI clock
 * frequency) if it's parameter is defined on the stack (observed with
 * American Megatrends EFI Revision 5.19) - let's define it globaly.
 */
static unsigned int bus_clock;

struct efi_i2c_capabilities {
	u32 StructureSizeInBytes;
	u32 MaximumReceiveBytes;
	u32 MaximumTransmitBytes;
	u32 MaximumTotalBytes;
};

struct efi_i2c_operation {
	u32 Flags;
	u32 LengthInBytes;
	u8  *Buffer;
};

struct efi_i2c_request_packet {
	unsigned int OperationCount;
	struct efi_i2c_operation Operation[];
};

struct efi_i2c_master_protocol {
	efi_status_t(EFIAPI * set_bus_frequency)(
		struct efi_i2c_master_protocol *this,
		unsigned int *bus_clock
	);
	efi_status_t(EFIAPI * reset)(
		struct efi_i2c_master_protocol *this
	);
	efi_status_t(EFIAPI * start_request)(
		struct efi_i2c_master_protocol *this,
		unsigned int slave_address,
		struct efi_i2c_request_packet *request_packet,
		void *event,
		efi_status_t *status
	);
	struct efi_i2c_capabilities *capabilities;
};

#endif

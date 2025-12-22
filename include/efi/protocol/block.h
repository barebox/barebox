/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_BLOCK_H_
#define __EFI_PROTOCOL_BLOCK_H_

#include <efi/types.h>

struct efi_block_io_media{
	u32 media_id;
	bool removable_media;
	bool media_present;
	bool logical_partition;
	bool read_only;
	bool write_caching;
	u32 block_size;
	u32 io_align;
	sector_t last_block;
	u64 lowest_aligned_lba; /* added in Revision 2 */
	u32 logical_blocks_per_physical_block; /* added in Revision 2 */
	u32 optimal_transfer_length_granularity; /* added in Revision 3 */
};

struct efi_block_io_protocol {
	u64 revision;
	struct efi_block_io_media *media;
	efi_status_t(EFIAPI *reset)(struct efi_block_io_protocol *this,
			bool ExtendedVerification);
	efi_status_t(EFIAPI *read)(struct efi_block_io_protocol *this, u32 media_id,
			u64 lba, size_t buffer_size, void *buf);
	efi_status_t(EFIAPI *write)(struct efi_block_io_protocol *this, u32 media_id,
			u64 lba, size_t buffer_size, void *buf);
	efi_status_t(EFIAPI *flush)(struct efi_block_io_protocol *this);
};

#endif

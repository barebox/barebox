/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_RNG_H_
#define __EFI_PROTOCOL_RNG_H_

#include <efi/types.h>

struct efi_rng_protocol {
	efi_status_t (EFIAPI *get_info)(struct efi_rng_protocol *protocol,
					size_t *rng_algorithm_list_size,
					efi_guid_t *rng_algorithm_list);
	efi_status_t (EFIAPI *get_rng)(struct efi_rng_protocol *protocol,
				       efi_guid_t *rng_algorithm,
				       size_t rng_value_length, uint8_t *rng_value);
};

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_PROTOCOL_UNICODE_COLLATION_H
#define _EFI_PROTOCOL_UNICODE_COLLATION_H

#include <efi/types.h>

struct efi_unicode_collation_protocol {
	efi_intn_t (EFIAPI *stri_coll)(
		struct efi_unicode_collation_protocol *this, u16 *s1, u16 *s2);
	bool (EFIAPI *metai_match)(struct efi_unicode_collation_protocol *this,
				   const u16 *string, const u16 *patter);
	void (EFIAPI *str_lwr)(struct efi_unicode_collation_protocol
			       *this, u16 *string);
	void (EFIAPI *str_upr)(struct efi_unicode_collation_protocol *this,
			       u16 *string);
	void (EFIAPI *fat_to_str)(struct efi_unicode_collation_protocol *this,
				  efi_uintn_t fat_size, char *fat, u16 *string);
	bool (EFIAPI *str_to_fat)(struct efi_unicode_collation_protocol *this,
				  const u16 *string, efi_uintn_t fat_size,
				  char *fat);
	char *supported_languages;
};

#endif

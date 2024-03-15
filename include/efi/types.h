/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_TYPES_H_
#define _EFI_TYPES_H_

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/uuid.h>

typedef unsigned long efi_status_t;
typedef u16 efi_char16_t;		/* UNICODE character */
typedef u64 efi_physical_addr_t;

struct efi_object;
typedef struct efi_object *efi_handle_t;

/*
 * The UEFI spec and EDK2 reference implementation both define EFI_GUID as
 * struct { u32 a; u16; b; u16 c; u8 d[8]; }; and so the implied alignment
 * is 32 bits not 8 bits like our guid_t. In some cases (i.e., on 32-bit ARM),
 * this means that firmware services invoked by the kernel may assume that
 * efi_guid_t* arguments are 32-bit aligned, and use memory accessors that
 * do not tolerate misalignment. So let's set the minimum alignment to 32 bits.
 *
 * Note that the UEFI spec as well as some comments in the EDK2 code base
 * suggest that EFI_GUID should be 64-bit aligned, but this appears to be
 * a mistake, given that no code seems to exist that actually enforces that
 * or relies on it.
 */
typedef guid_t efi_guid_t __aligned(__alignof__(u32));

#define EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7) \
((efi_guid_t) \
{{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
  (b) & 0xff, ((b) >> 8) & 0xff, \
  (c) & 0xff, ((c) >> 8) & 0xff, \
  (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }})

#ifdef __x86_64__
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

struct efi_device_path {
	u8 type;
	u8 sub_type;
	u16 length;
} __packed;

struct efi_mac_address {
	uint8_t Addr[32];
};

struct efi_ipv4_address {
	uint8_t Addr[4];
};

struct efi_ipv6_address {
	uint8_t Addr[16];
};

union efi_ip_address {
	uint32_t Addr[4];
	struct efi_ipv4_address v4;
	struct efi_ipv6_address v6;
};

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_TYPES_H_
#define _EFI_TYPES_H_

#include <efi/attributes.h>

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <linux/limits.h>
#include <linux/stddef.h>
#include <linux/compiler.h>
#include <linux/uuid.h>

typedef unsigned long efi_status_t;
typedef wchar_t efi_char16_t;		/* UNICODE character */
typedef u64 efi_physical_addr_t;	/* always, even on 32-bit systems */

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

static inline void *efi_phys_to_virt(efi_physical_addr_t addr)
{
	if (addr > UINTPTR_MAX)
		__builtin_trap();

	return (void *)(uintptr_t)addr;
}

static inline efi_physical_addr_t efi_virt_to_phys(const void *addr)
{
	return (uintptr_t)addr;
}

/*
 * Types and defines for Time Services
 */
#define EFI_TIME_ADJUST_DAYLIGHT 0x1
#define EFI_TIME_IN_DAYLIGHT     0x2
#define EFI_UNSPECIFIED_TIMEZONE 0x07ff

struct efi_time {
	u16 year;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 second;
	u8 pad1;
	u32 nanosecond;
	s16 timezone;
	u8 daylight;
	u8 pad2;
};

struct efi_time_cap {
	u32 resolution;
	u32 accuracy;
	u8 sets_to_zero;
};

/*
 * Allocation types for calls to boottime->allocate_pages.
 */
enum efi_allocate_type {
	EFI_ALLOCATE_ANY_PAGES,
	EFI_ALLOCATE_MAX_ADDRESS,
	EFI_ALLOCATE_ADDRESS,
	EFI_MAX_ALLOCATE_TYPE
};

#define EFI_PAGE_SHIFT			12
#define EFI_PAGE_SIZE			(1ULL << EFI_PAGE_SHIFT)
#define EFI_PAGE_MASK			(EFI_PAGE_SIZE - 1)

#endif

#endif

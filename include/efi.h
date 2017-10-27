#ifndef _LINUX_EFI_H
#define _LINUX_EFI_H

/*
 * Extensible Firmware Interface
 * Based on 'Extensible Firmware Interface Specification' version 0.9, April 30, 1999
 *
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999, 2002-2003 Hewlett-Packard Co.
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 */
#include <linux/string.h>
#include <linux/types.h>

#ifdef CONFIG_EFI_BOOTUP
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

struct efi_device_path;

#define EFI_SUCCESS                             0
#define EFI_LOAD_ERROR                  ( 1 | (1UL << (BITS_PER_LONG-1)))
#define EFI_INVALID_PARAMETER           ( 2 | (1UL << (BITS_PER_LONG-1)))
#define EFI_UNSUPPORTED                 ( 3 | (1UL << (BITS_PER_LONG-1)))
#define EFI_BAD_BUFFER_SIZE             ( 4 | (1UL << (BITS_PER_LONG-1)))
#define EFI_BUFFER_TOO_SMALL            ( 5 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NOT_READY                   ( 6 | (1UL << (BITS_PER_LONG-1)))
#define EFI_DEVICE_ERROR                ( 7 | (1UL << (BITS_PER_LONG-1)))
#define EFI_WRITE_PROTECTED             ( 8 | (1UL << (BITS_PER_LONG-1)))
#define EFI_OUT_OF_RESOURCES            ( 9 | (1UL << (BITS_PER_LONG-1)))
#define EFI_VOLUME_CORRUPTED            ( 10 | (1UL << (BITS_PER_LONG-1)))
#define EFI_VOLUME_FULL                 ( 11 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NO_MEDIA                    ( 12 | (1UL << (BITS_PER_LONG-1)))
#define EFI_MEDIA_CHANGED               ( 13 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NOT_FOUND                   ( 14 | (1UL << (BITS_PER_LONG-1)))
#define EFI_ACCESS_DENIED               ( 15 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NO_RESPONSE                 ( 16 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NO_MAPPING                  ( 17 | (1UL << (BITS_PER_LONG-1)))
#define EFI_TIMEOUT                     ( 18 | (1UL << (BITS_PER_LONG-1)))
#define EFI_NOT_STARTED                 ( 19 | (1UL << (BITS_PER_LONG-1)))
#define EFI_ALREADY_STARTED             ( 20 | (1UL << (BITS_PER_LONG-1)))
#define EFI_ABORTED                     ( 21 | (1UL << (BITS_PER_LONG-1)))
#define EFI_ICMP_ERROR                  ( 22 | (1UL << (BITS_PER_LONG-1)))
#define EFI_TFTP_ERROR                  ( 23 | (1UL << (BITS_PER_LONG-1)))
#define EFI_PROTOCOL_ERROR              ( 24 | (1UL << (BITS_PER_LONG-1)))
#define EFI_INCOMPATIBLE_VERSION        ( 25 | (1UL << (BITS_PER_LONG-1)))
#define EFI_SECURITY_VIOLATION          ( 26 | (1UL << (BITS_PER_LONG-1)))
#define EFI_CRC_ERROR                   ( 27 | (1UL << (BITS_PER_LONG-1)))
#define EFI_END_OF_MEDIA                ( 28 | (1UL << (BITS_PER_LONG-1)))
#define EFI_END_OF_FILE                 ( 31 | (1UL << (BITS_PER_LONG-1)))
#define EFI_INVALID_LANGUAGE            ( 32 | (1UL << (BITS_PER_LONG-1)))
#define EFI_COMPROMISED_DATA            ( 33 | (1UL << (BITS_PER_LONG-1)))

#define EFI_ERROR(a)	(((signed long) a) < 0)

typedef unsigned long efi_status_t;
typedef u8 efi_bool_t;
typedef u16 efi_char16_t;		/* UNICODE character */
typedef u64 efi_physical_addr_t;
typedef void *efi_handle_t;


typedef struct {
	u8 b[16];
} efi_guid_t;

#define EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7) \
((efi_guid_t) \
{{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
  (b) & 0xff, ((b) >> 8) & 0xff, \
  (c) & 0xff, ((c) >> 8) & 0xff, \
  (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }})

/*
 * Generic EFI table header
 */
typedef	struct {
	u64 signature;
	u32 revision;
	u32 headersize;
	u32 crc32;
	u32 reserved;
} efi_table_hdr_t;

/*
 * Memory map descriptor:
 */

/* Memory types: */
#define EFI_RESERVED_TYPE		 0
#define EFI_LOADER_CODE			 1
#define EFI_LOADER_DATA			 2
#define EFI_BOOT_SERVICES_CODE		 3
#define EFI_BOOT_SERVICES_DATA		 4
#define EFI_RUNTIME_SERVICES_CODE	 5
#define EFI_RUNTIME_SERVICES_DATA	 6
#define EFI_CONVENTIONAL_MEMORY		 7
#define EFI_UNUSABLE_MEMORY		 8
#define EFI_ACPI_RECLAIM_MEMORY		 9
#define EFI_ACPI_MEMORY_NVS		10
#define EFI_MEMORY_MAPPED_IO		11
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE	12
#define EFI_PAL_CODE			13
#define EFI_MAX_MEMORY_TYPE		14

/* Attribute values: */
#define EFI_MEMORY_UC		((u64)0x0000000000000001ULL)	/* uncached */
#define EFI_MEMORY_WC		((u64)0x0000000000000002ULL)	/* write-coalescing */
#define EFI_MEMORY_WT		((u64)0x0000000000000004ULL)	/* write-through */
#define EFI_MEMORY_WB		((u64)0x0000000000000008ULL)	/* write-back */
#define EFI_MEMORY_WP		((u64)0x0000000000001000ULL)	/* write-protect */
#define EFI_MEMORY_RP		((u64)0x0000000000002000ULL)	/* read-protect */
#define EFI_MEMORY_XP		((u64)0x0000000000004000ULL)	/* execute-protect */
#define EFI_MEMORY_RUNTIME	((u64)0x8000000000000000ULL)	/* range requires runtime mapping */
#define EFI_MEMORY_DESCRIPTOR_VERSION	1

#define EFI_PAGE_SHIFT		12
#define EFI_PAGE_SIZE		(1UL << EFI_PAGE_SHIFT)

/*
 * Allocation types for calls to boottime->allocate_pages.
 */
#define EFI_ALLOCATE_ANY_PAGES		0
#define EFI_ALLOCATE_MAX_ADDRESS	1
#define EFI_ALLOCATE_ADDRESS		2
#define EFI_MAX_ALLOCATE_TYPE		3

typedef int (*efi_freemem_callback_t) (u64 start, u64 end, void *arg);

/*
 * Types and defines for Time Services
 */
#define EFI_TIME_ADJUST_DAYLIGHT 0x1
#define EFI_TIME_IN_DAYLIGHT     0x2
#define EFI_UNSPECIFIED_TIMEZONE 0x07ff

typedef struct {
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
} efi_time_t;

typedef struct {
	u32 resolution;
	u32 accuracy;
	u8 sets_to_zero;
} efi_time_cap_t;

enum efi_locate_search_type {
	all_handles,
	by_register_notify,
	by_protocol
};

struct efi_open_protocol_information_entry {
	efi_handle_t agent_handle;
	efi_handle_t controller_handle;
	u32 attributes;
	u32 open_count;
};

typedef enum {
	EFI_TIMER_CANCEL = 0,
	EFI_TIMER_PERIODIC = 1,
	EFI_TIMER_RELATIVE = 2
} efi_timer_delay_t;

/*
 * EFI Boot Services table
 */
typedef struct {
	efi_table_hdr_t hdr;
	void *raise_tpl;
	void *restore_tpl;
	efi_status_t (EFIAPI *allocate_pages)(int, int, unsigned long,
				       efi_physical_addr_t *);
	efi_status_t (EFIAPI *free_pages)(efi_physical_addr_t, unsigned long);
	efi_status_t (EFIAPI *get_memory_map)(unsigned long *, void *, unsigned long *,
				       unsigned long *, u32 *);
	efi_status_t (EFIAPI *allocate_pool)(int, unsigned long, void **);
	efi_status_t (EFIAPI *free_pool)(void *);
#define EFI_EVT_TIMER				0x80000000
#define EFI_EVT_RUNTIME				0x40000000
#define EFI_EVT_NOTIFY_WAIT			0x00000100
#define EFI_EVT_NOTIFY_SIGNAL			0x00000200
#define EFI_EVT_SIGNAL_EXIT_BOOT_SERVICES	0x00000201
#define EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

#define EFI_TPL_APPLICATION	4
#define EFI_TPL_CALLBACK	8
#define EFI_TPL_NOTIFY		16
#define EFI_TPL_HIGH_LEVEL	31
	efi_status_t(EFIAPI *create_event)(u32 type , unsigned long tpl,
			void (*fn) (void *event, void *ctx),
			void *ctx, void **event);
	efi_status_t(EFIAPI *set_timer)(void *event, efi_timer_delay_t type, uint64_t time);
	efi_status_t(EFIAPI *wait_for_event)(unsigned long number_of_events, void *event,
			unsigned long *index);
	void *signal_event;
	efi_status_t(EFIAPI *close_event)(void *event);
	void *check_event;
	void *install_protocol_interface;
	void *reinstall_protocol_interface;
	void *uninstall_protocol_interface;
	efi_status_t (EFIAPI *handle_protocol)(efi_handle_t, efi_guid_t *, void **);
	void *__reserved;
	void *register_protocol_notify;
	efi_status_t (EFIAPI *locate_handle) (enum efi_locate_search_type search_type,
			efi_guid_t *protocol, void *search_key,
			unsigned long *buffer_size, efi_handle_t *buffer);
	efi_status_t (EFIAPI *locate_device_path)(efi_guid_t *protocol,
			struct efi_device_path **device_path, efi_handle_t *device);
	void *install_configuration_table;
	efi_status_t (EFIAPI *load_image)(bool boot_policiy, efi_handle_t parent_image,
			struct efi_device_path *file_path, void *source_buffer,
			unsigned long source_size, efi_handle_t *image);
	efi_status_t (EFIAPI *start_image)(efi_handle_t handle,
			unsigned long *exitdata_size, s16 **exitdata);
	efi_status_t(EFIAPI *exit)(efi_handle_t handle,  efi_status_t exit_status,
			unsigned long exitdata_size, s16 *exitdata);
	efi_status_t (EFIAPI *unload_image)(efi_handle_t handle);
	efi_status_t (EFIAPI *exit_boot_services)(efi_handle_t, unsigned long);
	void *get_next_monotonic_count;
	efi_status_t (EFIAPI *stall)(unsigned long usecs);
	void *set_watchdog_timer;
	efi_status_t(EFIAPI *connect_controller)(efi_handle_t controller_handle,
			efi_handle_t *driver_image_handle,
			struct efi_device_path *remaining_device_path,
			bool Recursive);
	void *disconnect_controller;
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020
	efi_status_t (EFIAPI *open_protocol)(efi_handle_t handle, efi_guid_t *protocol,
			void ** interface, efi_handle_t agent_handle,
			efi_handle_t controller_handle, u32 attributes);
	void *close_protocol;
	efi_status_t(EFIAPI *open_protocol_information)(efi_handle_t handle, efi_guid_t *Protocol,
			struct efi_open_protocol_information_entry **entry_buffer,
			unsigned long *entry_count);
	efi_status_t (EFIAPI *protocols_per_handle)(efi_handle_t handle,
			efi_guid_t ***protocol_buffer,
			unsigned long *protocols_buffer_count);
	efi_status_t (EFIAPI *locate_handle_buffer) (
			enum efi_locate_search_type search_type,
			efi_guid_t *protocol, void *search_key,
			unsigned long *no_handles, efi_handle_t **buffer);
	void *locate_protocol;
	void *install_multiple_protocol_interfaces;
	void *uninstall_multiple_protocol_interfaces;
	void *calculate_crc32;
	void *copy_mem;
	void *set_mem;
	void *create_event_ex;
} efi_boot_services_t;

extern efi_boot_services_t *BS;

/*
 * Types and defines for EFI ResetSystem
 */
typedef enum {
	EFI_RESET_COLD = 0,
	EFI_RESET_WARM = 1,
	EFI_RESET_SHUTDOWN = 2
} efi_reset_type_t;

/*
 * EFI Runtime Services table
 */
#define EFI_RUNTIME_SERVICES_SIGNATURE ((u64)0x5652453544e5552ULL)
#define EFI_RUNTIME_SERVICES_REVISION  0x00010000

typedef struct {
	efi_table_hdr_t hdr;
	void *get_time;
	void *set_time;
	void *get_wakeup_time;
	void *set_wakeup_time;
	void *set_virtual_address_map;
	void *convert_pointer;
	efi_status_t (EFIAPI *get_variable)(s16 *variable_name, efi_guid_t *vendor,
			u32 *Attributes, unsigned long *data_size, void *data);
	efi_status_t (EFIAPI *get_next_variable)(unsigned long *variable_name_size,
			s16 *variable_name, efi_guid_t *vendor);
	efi_status_t (EFIAPI *set_variable)(s16 *variable_name, efi_guid_t *vendor,
			u32 Attributes, unsigned long data_size, void *data);
	void *get_next_high_mono_count;
	void (EFIAPI *reset_system)(efi_reset_type_t reset_type, efi_status_t reset_status,
			unsigned long data_size, void *reset_data);
	void *update_capsule;
	void *query_capsule_caps;
	void *query_variable_info;
} efi_runtime_services_t;

extern efi_runtime_services_t *RT;

/*
 *  EFI Configuration Table and GUID definitions
 */
#define EFI_NULL_GUID \
    EFI_GUID(  0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 )

#define EFI_MPS_TABLE_GUID    \
    EFI_GUID(  0xeb9d2d2f, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define EFI_ACPI_TABLE_GUID    \
    EFI_GUID(  0xeb9d2d30, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define EFI_ACPI_20_TABLE_GUID    \
    EFI_GUID(  0x8868e871, 0xe4f1, 0x11d3, 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 )

#define EFI_SMBIOS_TABLE_GUID    \
    EFI_GUID(  0xeb9d2d31, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define EFI_SAL_SYSTEM_TABLE_GUID    \
    EFI_GUID(  0xeb9d2d32, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d )

#define EFI_HCDP_TABLE_GUID	\
    EFI_GUID(  0xf951938d, 0x620b, 0x42ef, 0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98 )

#define EFI_UGA_IO_PROTOCOL_GUID \
    EFI_GUID(  0x61a4d49e, 0x6f68, 0x4f1b, 0xb9, 0x22, 0xa8, 0x6e, 0xed, 0xb, 0x7, 0xa2 )

#define EFI_GLOBAL_VARIABLE_GUID \
    EFI_GUID(  0x8be4df61, 0x93ca, 0x11d2, 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c )

#define EFI_UV_SYSTEM_TABLE_GUID \
    EFI_GUID(  0x3b13a7d4, 0x633e, 0x11dd, 0x93, 0xec, 0xda, 0x25, 0x56, 0xd8, 0x95, 0x93 )

#define EFI_LINUX_EFI_CRASH_GUID \
    EFI_GUID(  0xcfc8fc79, 0xbe2e, 0x4ddc, 0x97, 0xf0, 0x9f, 0x98, 0xbf, 0xe2, 0x98, 0xa0 )

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    EFI_GUID(  0x5b1b31a1, 0x9562, 0x11d2, 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b )

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    EFI_GUID(  0x9042a9de, 0x23dc, 0x4a38, 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a )

#define EFI_UGA_PROTOCOL_GUID \
    EFI_GUID(  0x982c298b, 0xf4fa, 0x41cb, 0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39 )

#define EFI_PCI_IO_PROTOCOL_GUID \
    EFI_GUID(  0x4cf5b200, 0x68b8, 0x4ca5, 0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x2, 0x9a )

#define EFI_USB_IO_PROTOCOL_GUID \
    EFI_GUID(0x2B2F68D6, 0x0CD2, 0x44cf, 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75)

#define EFI_FILE_INFO_GUID \
    EFI_GUID(  0x9576e92, 0x6d3f, 0x11d2, 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b )

#define EFI_SIMPLE_FILE_SYSTEM_GUID \
    EFI_GUID(  0x964e5b22, 0x6459, 0x11d2, 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b )

#define EFI_DEVICE_TREE_GUID \
    EFI_GUID(  0xb1b621d5, 0xf19c, 0x41a5, 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 )

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    EFI_GUID(  0x9576e91, 0x6d3f, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b )

#define EFI_SIMPLE_NETWORK_PROTOCOL_GUID \
    EFI_GUID( 0xA19832B9, 0xAC25, 0x11D3, 0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D )

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
	EFI_GUID(0x0964e5b22, 0x6459, 0x11d2, 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

#define EFI_UNKNOWN_DEVICE_GUID \
	EFI_GUID(0xcf31fac5, 0xc24e, 0x11d2, 0x85, 0xf3, 0x0, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b)

#define EFI_BLOCK_IO_PROTOCOL_GUID \
	EFI_GUID(0x964e5b21, 0x6459, 0x11d2, 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

/* additional GUID from EDK2 */
#define EFI_FIRMWARE_VOLUME2_PROTOCOL_GUID \
	EFI_GUID(0x220e73b6, 0x6bdb, 0x4413, 0x84, 0x5, 0xb9, 0x74, 0xb1, 0x8, 0x61, 0x9a)

#define EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
	EFI_GUID(0x8f644fa9, 0xe850, 0x4db1, 0x9c, 0xe2, 0xb, 0x44, 0x69, 0x8e, 0x8d, 0xa4)

#define EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID \
	EFI_GUID(0x2f707ebb, 0x4a1a, 0x11d4, 0x9a, 0x38, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d)

#define EFI_ISA_ACPI_PROTOCOL_GUID \
	EFI_GUID(0x64a892dc, 0x5561, 0x4536, 0x92, 0xc7, 0x79, 0x9b, 0xfc, 0x18, 0x33, 0x55)

#define EFI_ISA_IO_PROTOCOL_GUID \
	EFI_GUID(0x7ee2bd44, 0x3da0, 0x11d4, 0x9a, 0x38, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d)

#define EFI_STANDARD_ERROR_DEVICE_GUID \
	EFI_GUID(0xd3b36f2d, 0xd551, 0x11d4, 0x9a, 0x46, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d)

#define EFI_CONSOLE_OUT_DEVICE_GUID \
	EFI_GUID(0xd3b36f2c, 0xd551, 0x11d4, 0x9a, 0x46, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d)

#define EFI_CONSOLE_IN_DEVICE_GUID \
	EFI_GUID(0xd3b36f2b, 0xd551, 0x11d4, 0x9a, 0x46, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d)

#define EFI_SIMPLE_TEXT_OUT_PROTOCOL_GUID \
	EFI_GUID(0x387477c2, 0x69c7, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

#define EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID \
	EFI_GUID(0xdd9e7534, 0x7762, 0x4698, 0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa)

#define EFI_SIMPLE_TEXT_IN_PROTOCOL_GUID \
	EFI_GUID(0x387477c1, 0x69c7, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

#define EFI_DISK_IO_PROTOCOL_GUID \
	EFI_GUID(0xce345171, 0xba0b, 0x11d2, 0x8e, 0x4f, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

#define EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID \
	EFI_GUID(0xa1e37052, 0x80d9, 0x4e65, 0xa3, 0x17, 0x3e, 0x9a, 0x55, 0xc4, 0x3e, 0xc9)

#define EFI_DISK_INFO_PROTOCOL_GUID \
	EFI_GUID(0xd432a67f, 0x14dc, 0x484b, 0xb3, 0xbb, 0x3f, 0x2, 0x91, 0x84, 0x93, 0x27)

#define EFI_SERIAL_IO_PROTOCOL_GUID \
	EFI_GUID(0xbb25cf6f, 0xf1d4, 0x11d2, 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd)

#define EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_GUID \
	EFI_GUID(0x3bc1b285, 0x8a15, 0x4a82, 0xaa, 0xbf, 0x4d, 0x7d, 0x13, 0xfb, 0x32, 0x65)

#define EFI_LOAD_FILE2_PROTOCOL_GUID \
	EFI_GUID(0x4006c0c1, 0xfcb3, 0x403e, 0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d)

#define EFI_MTFTP4_SERVICE_BINDING_PROTOCOL_GUID \
	EFI_GUID(0x2fe800be, 0x8f01, 0x4aa6, 0x94, 0x6b, 0xd7, 0x13, 0x88, 0xe1, 0x83, 0x3f)

#define EFI_DHCP4_PROTOCOL_GUID \
	EFI_GUID(0x9d9a39d8, 0xbd42, 0x4a73, 0xa4, 0xd5, 0x8e, 0xe9, 0x4b, 0xe1, 0x13, 0x80)

#define EFI_UDP4_SERVICE_BINDING_PROTOCOL_GUID \
	EFI_GUID(0x83f01464, 0x99bd, 0x45e5, 0xb3, 0x83, 0xaf, 0x63, 0x05, 0xd8, 0xe9, 0xe6)

#define EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID \
	EFI_GUID(0x00720665, 0x67EB, 0x4a99, 0xBA, 0xF7, 0xD3, 0xC3, 0x3A, 0x1C, 0x7C, 0xC9)

#define EFI_IP4_SERVICE_BINDING_PROTOCOL_GUID \
	EFI_GUID(0xc51711e7, 0xb4bf, 0x404a, 0xbf, 0xb8, 0x0a, 0x04, 0x8e, 0xf1, 0xff, 0xe4)

#define EFI_IP4_CONFIG_PROTOCOL_GUID \
	EFI_GUID(0x3b95aa31, 0x3793, 0x434b, 0x86, 0x67, 0xc8, 0x07, 0x08, 0x92, 0xe0, 0x5e)

#define EFI_ARP_SERVICE_BINDING_PROTOCOL_GUID\
	EFI_GUID(0xf44c00ee, 0x1f2c, 0x4a00, 0xaa, 0x9, 0x1c, 0x9f, 0x3e, 0x8, 0x0, 0xa3)

#define EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL_GUID \
	EFI_GUID(0xf36ff770, 0xa7e1, 0x42cf, 0x9e, 0xd2, 0x56, 0xf0, 0xf2, 0x71, 0xf4, 0x4c)

#define EFI_VLAN_CONFIG_PROTOCOL_GUID \
	EFI_GUID(0x9e23d768, 0xd2f3, 0x4366, 0x9f, 0xc3, 0x3a, 0x7a, 0xba, 0x86, 0x43, 0x74)

#define EFI_HII_CONFIG_ACCESS_PROTOCOL_GUID \
	EFI_GUID(0x330d4706, 0xf2a0, 0x4e4f, 0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85)

#define LOAD_FILE_PROTOCOL_GUID \
	EFI_GUID(0x56ec3091, 0x954c, 0x11d2, 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

#define EFI_COMPONENT_NAME2_PROTOCOL_GUID \
	EFI_GUID(0x6a7a5cff, 0xe8d9, 0x4f70, 0xba, 0xda, 0x75, 0xab, 0x30, 0x25, 0xce, 0x14)

#define EFI_IDEBUSDXE_INF_GUID \
	EFI_GUID(0x69fd8e47, 0xa161, 0x4550, 0xb0, 0x1a, 0x55, 0x94, 0xce, 0xb2, 0xb2, 0xb2)

#define EFI_TERMINALDXE_INF_GUID \
	EFI_GUID(0x9e863906, 0xa40f, 0x4875, 0x97, 0x7f, 0x5b, 0x93, 0xff, 0x23, 0x7f, 0xc6)

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID_31 \
	EFI_GUID(0x1aced566, 0x76ed, 0x4218, 0xbc, 0x81, 0x76, 0x7f, 0x1f, 0x97, 0x7a, 0x89)

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID \
	EFI_GUID(0xe18541cd, 0xf755, 0x4f73, 0x92, 0x8d, 0x64, 0x3c, 0x8a, 0x79, 0xb2, 0x29)

#define EFI_ISCSIDXE_INF_GUID \
	EFI_GUID(0x4579b72d, 0x7ec4, 0x4dd4, 0x84, 0x86, 0x08, 0x3c, 0x86, 0xb1, 0x82, 0xa7)

#define EFI_VLANCONFIGDXE_INF_GUID \
	EFI_GUID(0xe4f61863, 0xfe2c, 0x4b56, 0xa8, 0xf4, 0x08, 0x51, 0x9b, 0xc4, 0x39, 0xdf)

#define EFI_TIMESTAMP_PROTOCOL_GUID \
	EFI_GUID(0xafbfde41, 0x2e6e, 0x4262, 0xba, 0x65, 0x62, 0xb9, 0x23, 0x6e, 0x54, 0x95)

/* barebox specific GUIDs */
#define EFI_BAREBOX_VENDOR_GUID \
	EFI_GUID(0x5b91f69c, 0x8b88, 0x4a2b, 0x92, 0x69, 0x5f, 0x1d, 0x80, 0x2b, 0x51, 0x75)

/* for systemd */
#define EFI_SYSTEMD_VENDOR_GUID \
	EFI_GUID(0x4a67b082, 0x0a4c, 0x41cf, 0xb6, 0xc7, 0x44, 0x0b, 0x29, 0xbb, 0x8c, 0x4f)

/* for TPM 1.2 */
#define EFI_TCG_PROTOCOL_GUID \
	EFI_GUID(0xf541796d, 0xa62e, 0x4954, 0xa7, 0x75, 0x95, 0x84, 0xf6, 0x1b, 0x9c, 0xdd)

/* for TPM 2.0 */
#define EFI_TCG2_PROTOCOL_GUID \
	EFI_GUID(0x607f766c, 0x7455, 0x42be, 0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f)

extern efi_guid_t efi_file_info_id;
extern efi_guid_t efi_simple_file_system_protocol_guid;
extern efi_guid_t efi_device_path_protocol_guid;
extern efi_guid_t efi_loaded_image_protocol_guid;
extern efi_guid_t efi_unknown_device_guid;
extern efi_guid_t efi_null_guid;
extern efi_guid_t efi_global_variable_guid;
extern efi_guid_t efi_block_io_protocol_guid;
extern efi_guid_t efi_barebox_vendor_guid;
extern efi_guid_t efi_systemd_vendor_guid;

typedef struct {
	efi_guid_t guid;
	unsigned long table;
} efi_config_table_t;

#define EFI_SYSTEM_TABLE_SIGNATURE ((u64)0x5453595320494249ULL)

#define EFI_2_30_SYSTEM_TABLE_REVISION  ((2 << 16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION  ((2 << 16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION  ((2 << 16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION  ((2 << 16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION  ((1 << 16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION  ((1 << 16) | (02))

typedef struct {
	efi_table_hdr_t hdr;
	unsigned long fw_vendor;	/* physical addr of CHAR16 vendor string */
	u32 fw_revision;
	unsigned long con_in_handle;
	struct efi_simple_input_interface *con_in;
	unsigned long con_out_handle;
	struct efi_simple_text_output_protocol *con_out;
	unsigned long stderr_handle;
	unsigned long std_err;
	efi_runtime_services_t *runtime;
	efi_boot_services_t *boottime;
	unsigned long nr_tables;
	efi_config_table_t *tables;
} efi_system_table_t;

typedef struct {
	u32 revision;
	void *parent_handle;
	efi_system_table_t *system_table;
	void *device_handle;
	void *file_path;
	void *reserved;
	u32 load_options_size;
	void *load_options;
	void *image_base;
	__aligned_u64 image_size;
	unsigned int image_code_type;
	unsigned int image_data_type;
	unsigned long unload;
} efi_loaded_image_t;

static inline int
efi_guidcmp (efi_guid_t left, efi_guid_t right)
{
	return memcmp(&left, &right, sizeof (efi_guid_t));
}

/*
 * Variable Attributes
 */
#define EFI_VARIABLE_NON_VOLATILE       0x0000000000000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x0000000000000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x0000000000000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x0000000000000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x0000000000000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x0000000000000020
#define EFI_VARIABLE_APPEND_WRITE	0x0000000000000040

#define EFI_VARIABLE_MASK 	(EFI_VARIABLE_NON_VOLATILE | \
				EFI_VARIABLE_BOOTSERVICE_ACCESS | \
				EFI_VARIABLE_RUNTIME_ACCESS | \
				EFI_VARIABLE_HARDWARE_ERROR_RECORD | \
				EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS | \
				EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | \
				EFI_VARIABLE_APPEND_WRITE)
/*
 * Length of a GUID string (strlen("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"))
 * not including trailing NUL
 */
#define EFI_VARIABLE_GUID_LEN 36

struct efi_device_path {
	u8 type;
	u8 sub_type;
	u16 length;
} __attribute ((packed));

struct simple_text_output_mode {
	s32 max_mode;
	s32 mode;
	s32 attribute;
	s32 cursor_column;
	s32 cursor_row;
	bool cursor_visible;
};

struct efi_simple_text_output_protocol {
	void *reset;
	efi_status_t (EFIAPI *output_string)(void *, void *);
	void *test_string;

	efi_status_t(EFIAPI *query_mode)(struct efi_simple_text_output_protocol *this,
			unsigned long mode_number, unsigned long *columns, unsigned long *rows);
	efi_status_t(EFIAPI *set_mode)(struct efi_simple_text_output_protocol *this,
			unsigned long mode_number);
	efi_status_t(EFIAPI *set_attribute)(struct efi_simple_text_output_protocol *this,
			unsigned long attribute);
	efi_status_t(EFIAPI *clear_screen) (struct efi_simple_text_output_protocol *this);
	efi_status_t(EFIAPI *set_cursor_position) (struct efi_simple_text_output_protocol *this,
			unsigned long column, unsigned long row);
	efi_status_t(EFIAPI *enable_cursor)(void *, bool enable);
	struct simple_text_output_mode *mode;
};

struct efi_input_key {
	u16 scan_code;
	s16 unicode_char;
};

struct efi_simple_input_interface {
	efi_status_t(EFIAPI *reset)(struct efi_simple_input_interface *this,
			bool ExtendedVerification);
	efi_status_t(EFIAPI *read_key_stroke)(struct efi_simple_input_interface *this,
			struct efi_input_key *key);
	void *wait_for_key;
};

typedef struct {
	uint8_t Addr[32];
} efi_mac_address;

typedef struct {
	uint8_t Addr[4];
} efi_ipv4_address;

typedef struct {
	uint8_t Addr[16];
} efi_ipv6_address;

typedef union {
	uint32_t Addr[4];
	efi_ipv4_address v4;
	efi_ipv6_address v6;
} efi_ip_address;

static inline int efi_compare_guid(efi_guid_t *a, efi_guid_t *b)
{
	return memcmp(a, b, sizeof(efi_guid_t));
}

struct efi_device_path *device_path_from_handle(efi_handle_t Handle);
char *device_path_to_str(struct efi_device_path *dev_path);
u8 device_path_to_type(struct efi_device_path *dev_path);
char *device_path_to_partuuid(struct efi_device_path *dev_path);

const char *efi_guid_string(efi_guid_t *g);

#endif /* _LINUX_EFI_H */

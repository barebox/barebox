/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _EFI_SERVICES_H_
#define _EFI_SERVICES_H_

#include <efi/types.h>

/*
 * Generic EFI table header
 */
struct efi_table_hdr {
	u64 signature;
	u32 revision;
	u32 headersize;
	u32 crc32;
	u32 reserved;
};

typedef int (*efi_freemem_callback_t) (u64 start, u64 end, void *arg);

enum efi_locate_search_type {
	ALL_HANDLES,
	BY_REGISTER_NOTIFY,
	BY_PROTOCOL
};

struct efi_open_protocol_information_entry {
	efi_handle_t agent_handle;
	efi_handle_t controller_handle;
	u32 attributes;
	u32 open_count;
};

enum efi_timer_delay {
	EFI_TIMER_CANCEL = 0,
	EFI_TIMER_PERIODIC = 1,
	EFI_TIMER_RELATIVE = 2
};

struct efi_event;
struct efi_memory_desc;

/*
 * EFI Boot Services table
 */
struct efi_boot_services {
	struct efi_table_hdr hdr;
	efi_status_t (EFIAPI *raise_tpl)(efi_uintn_t new_tpl);
	void (EFIAPI *restore_tpl)(efi_uintn_t old_tpl);
	efi_status_t (EFIAPI *allocate_pages)(int, int, size_t,
				       efi_physical_addr_t *);
	efi_status_t (EFIAPI *free_pages)(efi_physical_addr_t, size_t);
	efi_status_t (EFIAPI *get_memory_map)(size_t *, struct efi_memory_desc *,
					      efi_uintn_t *, size_t *, u32 *);
	efi_status_t (EFIAPI *allocate_pool)(int, size_t, void **);
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
	efi_status_t(EFIAPI *create_event)(u32 type , efi_uintn_t tpl,
			void (EFIAPI *fn) (struct efi_event *event, void *ctx),
			void *ctx, struct efi_event **event);
	efi_status_t(EFIAPI *set_timer)(struct efi_event *event, enum efi_timer_delay type, uint64_t time);
	efi_status_t(EFIAPI *wait_for_event)(size_t number_of_events, struct efi_event **event,
			size_t *index);
	efi_status_t (EFIAPI *signal_event)(struct efi_event *event);
	efi_status_t(EFIAPI *close_event)(struct efi_event *event);
#define EFI_NATIVE_INTERFACE	0x00000000
	efi_status_t (EFIAPI *check_event)(struct efi_event *event);
	efi_status_t (EFIAPI *install_protocol_interface)(efi_handle_t *handle, const efi_guid_t *protocol,
			int protocol_interface_type, void *protocol_interface);
	efi_status_t (EFIAPI *reinstall_protocol_interface)(efi_handle_t handle, const efi_guid_t *protocol,
			void *old_interface, void *new_interface);
	efi_status_t (EFIAPI *uninstall_protocol_interface)(efi_handle_t handle,
			const efi_guid_t *protocol, void *protocol_interface);

	efi_status_t (EFIAPI *handle_protocol)(efi_handle_t, const efi_guid_t *, void **);
	void *__reserved;
	efi_status_t (EFIAPI *register_protocol_notify)(const efi_guid_t *protocol, struct efi_event *event,
			void **registration);
	efi_status_t (EFIAPI *locate_handle) (enum efi_locate_search_type search_type,
			const efi_guid_t *protocol, void *search_key,
			size_t *buffer_size, efi_handle_t *buffer);
	efi_status_t (EFIAPI *locate_device_path)(const efi_guid_t *protocol,
			struct efi_device_path **device_path, efi_handle_t *device);
	efi_status_t (EFIAPI *install_configuration_table)(const efi_guid_t *guid, void *table);
	efi_status_t (EFIAPI *load_image)(bool boot_policiy, efi_handle_t parent_image,
			struct efi_device_path *file_path, void *source_buffer,
			size_t source_size, efi_handle_t *image);
	efi_status_t (EFIAPI *start_image)(efi_handle_t handle,
			size_t *exitdata_size, u16 **exitdata);
	efi_status_t(EFIAPI *exit)(efi_handle_t handle,  efi_status_t exit_status,
			size_t exitdata_size, u16 *exitdata);
	efi_status_t (EFIAPI *unload_image)(efi_handle_t handle);
	efi_status_t (EFIAPI *exit_boot_services)(efi_handle_t, efi_uintn_t);
	void *get_next_monotonic_count;
	efi_status_t (EFIAPI *stall)(unsigned long usecs);
	efi_status_t (EFIAPI *set_watchdog_timer)(unsigned long timeout,
						  uint64_t watchdog_code,
						  size_t data_size,
						  u16 *watchdog_data);
	efi_status_t(EFIAPI *connect_controller)(efi_handle_t controller_handle,
			efi_handle_t *driver_image_handle,
			struct efi_device_path *remaining_device_path,
			bool Recursive);
	efi_status_t (EFIAPI *disconnect_controller)(efi_handle_t controller_handle,
			efi_handle_t driver_image_handle, efi_handle_t child_handle);
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020
	efi_status_t (EFIAPI *open_protocol)(efi_handle_t handle, const efi_guid_t *protocol,
			void ** interface, efi_handle_t agent_handle,
			efi_handle_t controller_handle, u32 attributes);
	efi_status_t (EFIAPI *close_protocol)(efi_handle_t handle, const efi_guid_t *protocol,
					      efi_handle_t agent, efi_handle_t controller);
	efi_status_t(EFIAPI *open_protocol_information)(efi_handle_t handle, const efi_guid_t *Protocol,
			struct efi_open_protocol_information_entry **entry_buffer,
			size_t *entry_count);
	efi_status_t (EFIAPI *protocols_per_handle)(efi_handle_t handle,
			efi_guid_t ***protocol_buffer,
			size_t *protocols_buffer_count);
	efi_status_t (EFIAPI *locate_handle_buffer) (
			enum efi_locate_search_type search_type,
			const efi_guid_t *protocol, void *search_key,
			size_t *no_handles, efi_handle_t **buffer);
	efi_status_t (EFIAPI *locate_protocol)(const efi_guid_t *protocol,
			void *registration, void **protocol_interface);
	efi_status_t (EFIAPI *install_multiple_protocol_interfaces)(efi_handle_t *handle, ...);
	efi_status_t (EFIAPI *uninstall_multiple_protocol_interfaces)(efi_handle_t handle, ...);
	efi_status_t (EFIAPI *calculate_crc32)(const void *data,
			size_t data_size, uint32_t *crc32);
	void (EFIAPI *copy_mem)(void *destination, const void *source, size_t length);
	void (EFIAPI *set_mem)(void *buffer, size_t size, uint8_t value);
	void *create_event_ex;
};

/*
 * Types and defines for EFI ResetSystem
 */
enum efi_reset_type {
	EFI_RESET_COLD = 0,
	EFI_RESET_WARM = 1,
	EFI_RESET_SHUTDOWN = 2,
	EFI_RESET_PLATFORM_SPECIFIC = 3,
};

/*
 * EFI Runtime Services table
 */
#define EFI_RUNTIME_SERVICES_SIGNATURE ((u64)0x5652453544e5552ULL)
#define EFI_RUNTIME_SERVICES_REVISION  0x00010000

struct efi_capsule_header;

struct efi_runtime_services {
	struct efi_table_hdr hdr;
	efi_status_t (EFIAPI *get_time)(struct efi_time *time,
					struct efi_time_cap *capabilities);
	efi_status_t (EFIAPI *set_time)(struct efi_time *time);
	efi_status_t (EFIAPI *get_wakeup_time)(char *enabled, char *pending,
				       struct efi_time *time);
	efi_status_t (EFIAPI *set_wakeup_time)(char enabled, struct efi_time *time);
	efi_status_t (EFIAPI *set_virtual_address_map)(size_t memory_map_size,
				       size_t descriptor_size,
				       uint32_t descriptor_version,
				       struct efi_memory_desc *virtmap);
	efi_status_t (EFIAPI *convert_pointer)(unsigned long dbg, void **address);
	efi_status_t (EFIAPI *get_variable)(efi_char16_t *variable_name, const efi_guid_t *vendor,
			u32 *Attributes, size_t *data_size, void *data);
	efi_status_t (EFIAPI *get_next_variable)(size_t *variable_name_size,
			efi_char16_t *variable_name, efi_guid_t *vendor);
	efi_status_t (EFIAPI *set_variable)(efi_char16_t *variable_name, const efi_guid_t *vendor,
			u32 Attributes, size_t data_size, const void *data);
	efi_status_t (EFIAPI *get_next_high_mono_count)(uint32_t *high_count);
	void (EFIAPI *reset_system)(enum efi_reset_type reset_type, efi_status_t reset_status,
			size_t data_size, void *reset_data);
	efi_status_t (EFIAPI *update_capsule)(struct efi_capsule_header **capsule_header_array,
					      size_t capsule_count,
					      u64 scatter_gather_list);
	efi_status_t (EFIAPI *query_capsule_caps)(struct efi_capsule_header **capsule_header_array,
						  size_t capsule_count,
						  u64 *maximum_capsule_size,
						  u32 *reset_type);
	void *query_variable_info;
};

struct efi_driver_binding_protocol {
	efi_status_t (EFIAPI * supported)(
			struct efi_driver_binding_protocol *this,
			efi_handle_t controller_handle,
			struct efi_device_path *remaining_device_path);
	efi_status_t (EFIAPI * start)(
			struct efi_driver_binding_protocol *this,
			efi_handle_t controller_handle,
			struct efi_device_path *remaining_device_path);
	efi_status_t (EFIAPI * stop)(
			struct efi_driver_binding_protocol *this,
			efi_handle_t controller_handle,
			size_t number_of_children,
			efi_handle_t *child_handle_buffer);
	u32 version;
	efi_handle_t image_handle;
	efi_handle_t driver_binding_handle;
};

struct efi_config_table {
	efi_guid_t guid;
	void * table;
};

#define EFI_SYSTEM_TABLE_SIGNATURE ((u64)0x5453595320494249ULL)

#define EFI_2_30_SYSTEM_TABLE_REVISION  ((2 << 16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION  ((2 << 16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION  ((2 << 16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION  ((2 << 16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION  ((1 << 16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION  ((1 << 16) | (02))

struct efi_system_table {
	struct efi_table_hdr hdr;
	efi_char16_t *fw_vendor;	/* physical addr of CHAR16 vendor string */
	u32 fw_revision;
	efi_handle_t con_in_handle;
	struct efi_simple_text_input_protocol *con_in;
	efi_handle_t con_out_handle;
	struct efi_simple_text_output_protocol *con_out;
	efi_handle_t stderr_handle;
	struct efi_simple_text_output_protocol *std_err;
	struct efi_runtime_services *runtime;
	struct efi_boot_services *boottime;
	size_t nr_tables;
	struct efi_config_table *tables;
};

struct efi_loaded_image {
	u32 revision;
	efi_handle_t parent_handle;
	struct efi_system_table *system_table;
	efi_handle_t device_handle;
	void *file_path;
	void *reserved;
	u32 load_options_size;
	void *load_options;
	void *image_base;
	__aligned_u64 image_size;
	unsigned int image_code_type;
	unsigned int image_data_type;
	efi_status_t (EFIAPI *unload)(efi_handle_t image_handle);
};

enum efi_locate_search_type;

int __efi_locate_handle(struct efi_boot_services *bs,
		enum efi_locate_search_type search_type,
		efi_guid_t *protocol,
		void *search_key,
		size_t *no_handles,
		efi_handle_t **buffer);

#endif

/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_FILE_H_
#define __EFI_PROTOCOL_FILE_H_

#include <efi/types.h>

/* Open modes */
#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

/* File attributes */
#define EFI_FILE_READ_ONLY      0x0000000000000001
#define EFI_FILE_HIDDEN         0x0000000000000002
#define EFI_FILE_SYSTEM         0x0000000000000004
#define EFI_FILE_RESERVIED      0x0000000000000008
#define EFI_FILE_DIRECTORY      0x0000000000000010
#define EFI_FILE_ARCHIVE        0x0000000000000020
#define EFI_FILE_VALID_ATTR     0x0000000000000037

struct efi_file_io_token {
	struct efi_event *event;
	efi_status_t status;
	size_t buffer_size;
	void *buffer;
};

#define EFI_FILE_HANDLE_REVISION         0x00010000
#define EFI_FILE_HANDLE_REVISION2        0x00020000
#define EFI_FILE_HANDLE_LATEST_REVISION  EFI_FILE_PROTOCOL_REVISION2
struct efi_file_handle {
	uint64_t Revision;
	efi_status_t(EFIAPI *open)(struct efi_file_handle *File,
			struct efi_file_handle **NewHandle, efi_char16_t *FileName,
			uint64_t OpenMode, uint64_t Attributes);
	efi_status_t(EFIAPI *close)(struct efi_file_handle *File);
	efi_status_t(EFIAPI *delete)(struct efi_file_handle *File);
	efi_status_t(EFIAPI *read)(struct efi_file_handle *File, size_t *BufferSize,
			void *Buffer);
	efi_status_t(EFIAPI *write)(struct efi_file_handle *File,
			size_t *BufferSize, void *Buffer);
	efi_status_t(EFIAPI *get_position)(struct efi_file_handle *File,
			uint64_t *Position);
	efi_status_t(EFIAPI *set_position)(struct efi_file_handle *File,
			uint64_t Position);
	efi_status_t(EFIAPI *get_info)(struct efi_file_handle *File,
			const efi_guid_t *InformationType, size_t *BufferSize,
			void *Buffer);
	efi_status_t(EFIAPI *set_info)(struct efi_file_handle *File,
			const efi_guid_t *InformationType, size_t BufferSize,
			void *Buffer);
	efi_status_t(EFIAPI *flush)(struct efi_file_handle *File);
	efi_status_t (EFIAPI *open_ex)(struct efi_file_handle *this,
			struct efi_file_handle **new_handle,
			u16 *file_name, u64 open_mode, u64 attributes,
			struct efi_file_io_token *token);
	efi_status_t (EFIAPI *read_ex)(struct efi_file_handle *this,
			struct efi_file_io_token *token);
	efi_status_t (EFIAPI *write_ex)(struct efi_file_handle *this,
			struct efi_file_io_token *token);
	efi_status_t (EFIAPI *flush_ex)(struct efi_file_handle *this,
			struct efi_file_io_token *token);
};

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000

#define EFI_FILE_IO_INTERFACE_REVISION   0x00010000

struct efi_file_io_interface {
	uint64_t Revision;
	efi_status_t(EFIAPI *open_volume)(
			struct efi_file_io_interface *This,
			struct efi_file_handle **Root);
};

struct efi_simple_file_system_protocol {
	u64 Revision;
	efi_status_t (EFIAPI *open_volume)(struct efi_simple_file_system_protocol *this,
			struct efi_file_handle **root);
};

struct efi_file_info {
	uint64_t Size;
	uint64_t FileSize;
	uint64_t PhysicalSize;
	struct efi_time CreateTime;
	struct efi_time LastAccessTime;
	struct efi_time ModificationTime;
	uint64_t Attribute;
	efi_char16_t FileName[];
};

struct efi_file_system_info {
	u64 size;
	u8 read_only;
	u64 volume_size;
	u64 free_space;
	u32 block_size;
	efi_char16_t volume_label[];
};

struct efi_load_file_protocol {
	efi_status_t (EFIAPI *load_file)(struct efi_load_file_protocol *this,
					 struct efi_device_path *file_path,
					 bool boot_policy,
					 size_t *buffer_size,
					 void *buffer);
};

#endif

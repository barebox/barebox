/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PROTOCOL_GOP_H_
#define __EFI_PROTOCOL_GOP_H_

#include <efi/types.h>

#define PIXEL_RGB_RESERVED_8BIT_PER_COLOR		0
#define PIXEL_BGR_RESERVED_8BIT_PER_COLOR		1
#define PIXEL_BIT_MASK					2
#define PIXEL_BLT_ONLY					3
#define PIXEL_FORMAT_MAX				4

struct efi_pixel_bitmask {
	u32 red_mask;
	u32 green_mask;
	u32 blue_mask;
	u32 reserved_mask;
};

struct efi_graphics_output_mode_info {
	u32 version;
	u32 horizontal_resolution;
	u32 vertical_resolution;
	int pixel_format;
	struct efi_pixel_bitmask pixel_information;
	u32 pixels_per_scan_line;
};

struct efi_graphics_output_protocol_mode {
	uint32_t max_mode;
	uint32_t mode;
	struct efi_graphics_output_mode_info *info;
	size_t size_of_info;
	void *frame_buffer_base;
	size_t frame_buffer_size;
};

struct efi_graphics_output_protocol {
	efi_status_t (EFIAPI *query_mode) (struct efi_graphics_output_protocol *This,
			uint32_t mode_number, size_t *size_of_info,
			struct efi_graphics_output_mode_info **info);
	efi_status_t (EFIAPI *set_mode) (struct efi_graphics_output_protocol *This,
			uint32_t mode_number);
	efi_status_t (EFIAPI *blt)(struct efi_graphics_output_protocol *This,
			void *buffer,
			unsigned long operation,
			unsigned long sourcex, unsigned long sourcey,
			unsigned long destinationx, unsigned long destinationy,
			unsigned long width, unsigned long height, unsigned
			long delta);
	struct efi_graphics_output_protocol_mode *mode;
};

#endif

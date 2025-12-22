/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __EFI_PROTOCOL_GOP_H_
#define __EFI_PROTOCOL_GOP_H_

#include <efi/types.h>

#define PIXEL_RGB_RESERVED_8BIT_PER_COLOR		0
#define PIXEL_BGR_RESERVED_8BIT_PER_COLOR		1
#define PIXEL_BIT_MASK					2
#define PIXEL_BLT_ONLY					3
#define PIXEL_FORMAT_MAX				4

#define EFI_GOT_RGBA8		0
#define EFI_GOT_BGRA8		1
#define EFI_GOT_BITMASK		2

struct efi_pixel_bitmask {
	u32 red_mask;
	u32 green_mask;
	u32 blue_mask;
	u32 reserved_mask;
};

#define EFI_BLT_VIDEO_FILL		0
#define EFI_BLT_VIDEO_TO_BLT_BUFFER	1
#define EFI_BLT_BUFFER_TO_VIDEO		2
#define EFI_BLT_VIDEO_TO_VIDEO		3

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
			struct efi_pixel_bitmask *buffer,
			u32 operation,
			size_t sourcex, size_t sourcey,
			size_t destinationx, size_t destinationy,
			size_t width, size_t height, size_t delta);
	struct efi_graphics_output_protocol_mode *mode;
};

#endif

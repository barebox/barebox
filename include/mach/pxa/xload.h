/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MACH_PXA_XLOAD_H
#define __MACH_PXA_XLOAD_H

#include <linux/types.h>

#define NTIM_ID_TIMH		0x54494d48	/* 'TIMH' */
#define NTIM_ID_OBMI		0x4f424d49	/* 'OBMI' */
#define NTIM_ID_BOOT		0x424f4f54	/* 'BOOT' */
#define NTIM_OEM_UNIQUE_ID	0xcafeaffe

#define NTIM_MAX_IMAGES		8

/*
 * Non-Trusted Image Module, the header the PXA3xx Boot ROM reads from the
 * start of the boot device. Written by scripts/pxa-image.
 */
struct ntim_header {
	u32 version;
	u32 identifier;
	u32 trusted;
	u32 issue_date;
	u32 oem_unique_id;
	u32 reserved[5];
	u32 flash_info;
	u32 num_images;
	u32 num_keys;
	u32 size_of_reserved;
};

struct ntim_image {
	u32 image_id;
	u32 next_image_id;
	u32 flash_entry_addr;
	u32 load_addr;
	u32 image_size;
	u32 reserved[10];
};

void *pxa_nand_load_image(u32 image_id);

#endif /* __MACH_PXA_XLOAD_H */

/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com> */

#ifndef _LVM2_H
#define _LVM2_H

#include <linux/types.h>

#define LVM2_LABEL_ID		"LABELONE"
#define LVM2_LABEL_TYPE		"LVM2 001"
#define LVM2_MDA_MAGIC		" LVM2 x[5A%r0N*>"
#define LVM2_MDA_VERSION	1
#define LVM2_LABEL_SCAN_SECTORS	4
#define LVM2_RAW_LOCN_IGNORED	0x00000001

struct lvm2_label {
	u8     id[8];		/* LVM2_LABEL_ID */
	__le64 sector;		/* Sector number of this label */
	__le32 crc;
	__le32 pv_offset;	/* Byte offset to pv_header within sector */
	u8     type[8];		/* LVM2_LABEL_TYPE */
} __packed;

struct lvm2_area {
	__le64 offset;
	__le64 size;
} __packed;

struct lvm2_pv_header {
	u8     uuid[32];
	__le64 size;

	/* Zero terminated list of data areas, followed by zero
	 * terminated list of metadata areas.
	 */
	struct lvm2_area area[0];
} __packed;

struct lvm2_md_area {
	__le64 offset;		/* Byte offset from start of MDA area */
	__le64 size;		/* Includes trailing NUL */
	__le32 checksum;
	__le32 flags;
} __packed;

/* The CRC32 variant used to protect the label and the metadata, see
 * lib/misc/crc.c in lvm2. Note the nonstandard initial value.
 */
static inline u32 lvm2_crc(const void *buf, size_t len)
{
	static const u32 tab[16] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
	};
	u32 crc = 0xf597a6cf;
	const u8 *p = buf;

	while (len--) {
		crc ^= *p++;
		crc = (crc >> 4) ^ tab[crc & 0xf];
		crc = (crc >> 4) ^ tab[crc & 0xf];
	}

	return crc;
}

struct lvm2_md_header {
	__le32 checksum;
	u8     magic[16];	/* LVM2_MDA_MAGIC */
	__le32 version;		/* LVM2_MDA_VERSION */
	__le64 start;		/* Byte offset of MDA area on device */
	__le64 size;		/* Size of MDA area */

	struct lvm2_md_area area[0];
} __packed;

#endif	/* _LVM2_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef COMPRESSED_DTB_H_
#define COMPRESSED_DTB_H_

#include <linux/types.h>
#include <asm/unaligned.h>

struct barebox_boarddata_compressed_dtb {
#define BAREBOX_BOARDDATA_COMPRESSED_DTB_MAGIC 0x7b66bcbd
	u32 magic;
	u32 datalen;
	u32 datalen_uncompressed;
};

static inline bool blob_is_compressed_fdt(const void *blob)
{
	const struct barebox_boarddata_compressed_dtb *dtb = blob;

	return dtb->magic == BAREBOX_BOARDDATA_COMPRESSED_DTB_MAGIC;
}

static inline bool fdt_blob_can_be_decompressed(const void *blob)
{
	return IS_ENABLED(CONFIG_USE_COMPRESSED_DTB) && blob
			&& blob_is_compressed_fdt(blob);
}

static inline bool blob_is_fdt(const void *blob)
{
	return get_unaligned_be32(blob) == FDT_MAGIC;
}

#endif

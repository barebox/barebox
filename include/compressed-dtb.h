/* SPDX-License-Identifier: GPL-2.0 */
#ifndef COMPRESSED_DTB_H_
#define COMPRESSED_DTB_H_

#include <linux/types.h>
#include <linux/sizes.h>
#include <asm/unaligned.h>

struct barebox_boarddata_compressed_dtb {
#define BAREBOX_BOARDDATA_COMPRESSED_DTB_MAGIC 0x7b66bcbd
	u32 magic;
	u32 datalen;
	u32 datalen_uncompressed;
	u8 data[];
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

static inline bool blob_is_valid_fdt_ptr(const void *blob, unsigned long mem_start,
					 unsigned long mem_size, unsigned int *fdt_size)
{
	unsigned long dtb = (unsigned long)blob;
	unsigned int size;

	if (!IS_ALIGNED(dtb, 4))
		return false;
	if (dtb < mem_start || dtb >= mem_start + mem_size)
		return false;
	if (!blob_is_fdt(blob))
		return false;

	size = be32_to_cpup(blob + 4);
	if (size > SZ_2M || dtb + size > mem_start + mem_size)
		return false;

	if (fdt_size)
		*fdt_size = size;

	return true;
}

#endif

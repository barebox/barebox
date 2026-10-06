// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com>

#include <block.h>
#include <fuzz.h>
#include <lvm.h>

#include "lvm2.h"
#include "lvm-md.h"

static int fuzz_lvm_md(const char *text, size_t size)
{
	struct lvm_md *md;

	if (!lvm_md_parse_alloc(text, size, &md))
		lvm_md_free(md);

	return 0;
}
fuzz_test_str("lvm-md", fuzz_lvm_md);

static void lvm_fuzz_fixup_mda(u8 *img, size_t size, u64 mda_offset, u64 mda_size)
{
	struct lvm2_md_header *hdr;
	struct lvm2_md_area *area;
	u64 off, len, wrap;
	u8 *text;

	if (mda_offset >= size || mda_size < SECTOR_SIZE ||
	    mda_size > size - mda_offset)
		return;

	hdr = (void *)(img + mda_offset);
	memcpy(hdr->magic, LVM2_MDA_MAGIC, sizeof(hdr->magic));
	put_unaligned_le32(LVM2_MDA_VERSION, &hdr->version);

	for (area = hdr->area;
	     (u8 *)(area + 1) <= (u8 *)hdr + SECTOR_SIZE; area++) {
		off = get_unaligned_le64(&area->offset);
		len = get_unaligned_le64(&area->size);

		if (!off && !len)
			break;
		if (!len || off >= mda_size || len > mda_size)
			continue;

		text = malloc(len);
		if (!text)
			return;

		if (off + len > mda_size) {
			wrap = mda_size - off;

			if (SECTOR_SIZE + (len - wrap) > mda_size)
				goto next;

			memcpy(text, img + mda_offset + off, wrap);
			memcpy(text + wrap,
			       img + mda_offset + SECTOR_SIZE,
			       len - wrap);
		} else {
			memcpy(text, img + mda_offset + off, len);
		}

		put_unaligned_le32(lvm2_crc(text, len), &area->checksum);
next:
		free(text);
	}

	/* Last, as it covers the descriptors fixed up above. */
	put_unaligned_le32(lvm2_crc(hdr->magic,
				    SECTOR_SIZE
				    - offsetof(struct lvm2_md_header, magic)),
			   &hdr->checksum);
}

static void lvm_fuzz_fixup_image(u8 *img, size_t size)
{
	struct lvm2_pv_header *pvh;
	struct lvm2_label *label;
	struct lvm2_area *area;
	u8 *sector = NULL;
	u32 pv_offset;
	int s;

	for (s = 0; s < LVM2_LABEL_SCAN_SECTORS; s++) {
		if ((size_t)(s + 1) << SECTOR_SHIFT > size)
			break;

		label = (void *)(img + ((size_t)s << SECTOR_SHIFT));
		if (!memcmp(label->id, LVM2_LABEL_ID, sizeof(label->id))) {
			sector = (u8 *)label;
			break;
		}
	}

	/* Without a label there is nothing to find, so plant one in the
	 * sector that LVM uses by default, leaving the rest of it as it
	 * is.
	 */
	if (!sector) {
		if (size < 2 * SECTOR_SIZE)
			return;

		s = 1;
		sector = img + SECTOR_SIZE;
		label = (void *)sector;
		memcpy(label->id, LVM2_LABEL_ID, sizeof(label->id));
	}

	memcpy(label->type, LVM2_LABEL_TYPE, sizeof(label->type));
	put_unaligned_le64(s, &label->sector);

	pv_offset = get_unaligned_le32(&label->pv_offset);
	if (pv_offset < SECTOR_SIZE) {
		pvh = (void *)(sector + pv_offset);

		/* Same walk as lvm_pv_probe(): past the data areas, past
		 * the zero separator, then one fixup per metadata area.
		 */
		for (area = pvh->area;
		     (u8 *)(area + 1) <= sector + SECTOR_SIZE
			     && get_unaligned_le64(&area->offset);
		     area++)
			;

		for (area++;
		     (u8 *)(area + 1) <= sector + SECTOR_SIZE
			     && get_unaligned_le64(&area->offset);
		     area++)
			lvm_fuzz_fixup_mda(img, size,
					   get_unaligned_le64(&area->offset),
					   get_unaligned_le64(&area->size));
	}

	put_unaligned_le32(lvm2_crc(&label->pv_offset,
				    SECTOR_SIZE - offsetof(struct lvm2_label, pv_offset)),
			   &label->crc);
}

static int fuzz_lvm(const u8 *data, size_t size)
{
	static struct ramdisk *ramdisk;
	struct block_device *blk;
	struct lvm_vg *vg;
	char *table;
	size_t i;
	u8 *img;

	if (size < 2 * SECTOR_SIZE)
		return 0;

	if (!ramdisk)
		ramdisk = ramdisk_init(SECTOR_SIZE);
	if (!ramdisk)
		return -ENODEV;

	img = xmemdup(data, size);

	/* Help the fuzzer out by injecting a proper LVM label and
	 * valid CRCs, so it can reach further into the parser.
	 */
	lvm_fuzz_fixup_image(img, size);

	ramdisk_setup_rw(ramdisk, img, size);
	blk = ramdisk_get_block_device(ramdisk);

	if (!lvm_vg_alloc_by_cdev(&blk->cdev, &vg)) {
		for (i = 0; i < vg->num_lvs; i++) {
			table = lvm_lv_dm_ctable(vg->lvs[i]);
			if (!IS_ERR(table))
				free(table);
		}

		lvm_vg_free(vg);
	}

	ramdisk_setup_rw(ramdisk, NULL, 0);
	free(img);
	return 0;
}
fuzz_test("lvm", fuzz_lvm);

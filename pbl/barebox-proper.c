// SPDX-License-Identifier: GPL-2.0-only

#include <pbl/handoff-data.h>
#include <pbl/barebox-proper.h>
#include <compressed-dtb.h>
#include <linux/printk.h>
#include <malloc.h>
#include <string.h>
#include <uncompress.h>

void *barebox_boot_dtb(void)
{
	void *dtb;
	int ret = 0;
	struct barebox_boarddata_compressed_dtb *compressed_dtb;
	static void *boot_dtb;
	void *blob;
	size_t size;

	if (boot_dtb)
		return boot_dtb;

	blob = handoff_data_get_entry(HANDOFF_DATA_INTERNAL_DT, &size);
	if (blob)
		return blob;

	blob = handoff_data_get_entry(HANDOFF_DATA_INTERNAL_DT_Z, &size);
	if (!blob)
		return NULL;

	if (!fdt_blob_can_be_decompressed(blob))
		return NULL;

	compressed_dtb = blob;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(ALIGN(compressed_dtb->datalen_uncompressed, 4));
	if (!dtb)
		return NULL;

	if (IS_ENABLED(CONFIG_IMAGE_COMPRESSION_NONE))
		memcpy(dtb, compressed_dtb->data,
		       compressed_dtb->datalen_uncompressed);
	else
		ret = uncompress(compressed_dtb->data, compressed_dtb->datalen,
				 NULL, NULL, dtb, NULL, NULL);

	if (ret) {
		pr_err("uncompressing dtb failed\n");
		free(dtb);
		return NULL;
	}

	boot_dtb = dtb;

	return boot_dtb;
}

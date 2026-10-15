// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "efi-fdt: " fmt

#include <common.h>
#include <init.h>
#include <libfile.h>
#include <of.h>
#include <efi/payload.h>
#include <efi/payload/init.h>
#include <efi/guid.h>

extern char __dtb_fallback_start[];

/*
 * EFI systems have no device tree, but barebox may still need one for its
 * own purposes, e.g. to describe a state partition. Register a tree that's
 * empty unless populated via CONFIG_EXTERNAL_DTS_FRAGMENTS.
 */
static __maybe_unused int efi_of_init(void)
{
	int ret;

	ret = barebox_register_fdt(__dtb_fallback_start);
	if (ret == -EBUSY) {
		/* architecture code registered a device tree already */
		pr_debug("keeping already registered device tree\n");
		return 0;
	}

	return ret;
}
#ifdef CONFIG_OFDEVICE
core_efi_initcall(efi_of_init);
#endif

static int efi_fdt_probe(void)
{
	struct efi_config_table *ect;

	for_each_efi_config_table(ect) {
		struct fdt_header *oftree;
		u32 magic, size;
		int ret;

		if (efi_guidcmp(ect->guid, EFI_DEVICE_TREE_GUID))
			continue;

		oftree = (void *)ect->table;
		magic = be32_to_cpu(oftree->magic);

		if (magic != FDT_MAGIC) {
			pr_err("table has invalid magic 0x%08x\n", magic);
			return -EILSEQ;
		}

		size = be32_to_cpu(oftree->totalsize);
		ret = write_file("/efi.dtb", oftree, size);
		if (ret) {
			pr_err("error saving /efi.dtb: %pe\n", ERR_PTR(ret));
			return ret;
		}

		return 0;
	}

	return 0;
}
late_efi_initcall(efi_fdt_probe);

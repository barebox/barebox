// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "efi-fdt: " fmt

#include <common.h>
#include <init.h>
#include <libfile.h>
#include <efi/efi-payload.h>
#include <efi/efi-device.h>

static int efi_fdt_probe(void)
{
	efi_config_table_t *ect;

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
late_initcall(efi_fdt_probe);

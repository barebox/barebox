// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "efi-fdt: " fmt

#include <common.h>
#include <init.h>
#include <libfile.h>
#include <of.h>
#include <efi/payload.h>
#include <efi/payload/init.h>
#include <efi/guid.h>
#include <efi/variable.h>

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

/*
 * Export barebox' device tree, so the OS can learn about barebox-specific
 * configuration like the state partition layout. Called just before handing
 * over control, so the variable describes the final device tree.
 */
void efi_export_dtb(void)
{
	struct fdt_header *fdt;
	int ret;

	/* SetVariable() won't change attributes, so drop any existing variable */
	efi_set_variable("barebox-dtb", &efi_barebox_vendor_guid, 0, NULL, 0);

	if (!of_get_root_node())
		return;

	fdt = of_get_flattened_tree(NULL, false);
	if (!fdt)
		return;

	ret = efi_set_variable("barebox-dtb", &efi_barebox_vendor_guid,
			       EFI_VARIABLE_BOOTSERVICE_ACCESS |
			       EFI_VARIABLE_RUNTIME_ACCESS,
			       fdt, fdt32_to_cpu(fdt->totalsize));
	if (ret)
		pr_warn("Cannot export device tree: %pe\n", ERR_PTR(ret));

	free(fdt);
}

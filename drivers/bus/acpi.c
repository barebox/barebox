// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019 Ahmad Fatoum
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <efi/payload.h>
#include <efi/payload/driver.h>
#include <acpi.h>

static struct sig_desc {
	const char sig[4];
	const char *desc;
} signatures[] = {
	/* ACPI 6.3 Table 5-29, Defined DESCRIPTION_HEADER Signatures */
	{ "APIC", "Multiple APIC Description" },
	{ "BERT", "Boot Error Record" },
	{ "BGRT", "Boot Graphics Resource" },
	{ "CPEP", "Corrected Platform Error Polling" },
	{ "DSDT", "Differentiated System Description" },
	{ "ECDT", "Embedded Controller Boot Resource" },
	{ "EINJ", "Error Injection" },
	{ "ERST", "Error Record Serialization" },
	{ "FACP", "Fixed ACPI Description" },
	{ "FACS", "Firmware ACPI Control Structure" },
	{ "FPDT", "Firmware Performance Data" },
	{ "GTDT", "Generic Timer Description" },
	{ "HEST", "Hardware Error Source" },
	{ "MSCT", "Maximum System Characteristics" },
	{ "MPST", "Memory Power State" },
	{ "NFIT", "NVDIMM Firmware Interface" },
	{ "OEM\0", "OEM Specific Information" },
	{ "PCCT", "Platform Communications Channel" },
	{ "PMTT", "Platform Memory Topology" },
	{ "PSDT", "Persistent System Description" },
	{ "RASF", "ACPI RAS Feature" },
	{ "RSDT", "Root System Description" },
	{ "SBST", "Smart Battery Specification" },
	{ "SDEV", "Secure Devices" },
	{ "SLIT", "System Locality Distance Information" },
	{ "SRAT", "System Resource Affinity" },
	{ "SSDT", "Secondary System Description" },
	{ "XSDT", "Extended System Description" },

	/* ACPI 6.3 Table 5-30, Reserved DESCRIPTION_HEADER Signatures */
	{ "BOOT", "Simple BOOT Flag" },
	{ "CSRT", "Core System Resource" },
	{ "DBG2", "Microsoft Debug Port 2" },
	{ "DBGP", "Debug Port" },
	{ "DMAR", "DMA Remapping" },
	{ "DPPT", "DMA Protection Policy" },
	{ "DRTM", "Dynamic Root of Trust for Measurement" },
	{ "ETDT", "(Obsolete) Event Timer Description" },
	{ "HPET", "IA-PC High Precision Event Timer" },
	{ "IBFT", "iSCSI Boot Firmware" },
	{ "IORT", "I/O Remapping" },
	{ "IVRS", "I/O Virtualization Reporting Structure" },
	{ "LPIT", "Low Power Idle" },
	{ "MCFG", "PCI Express memory mapped configuration" },
	{ "MCHI", "Management Controller Host Interface" },
	{ "MSDM", "Microsoft Data Management" },
	{ "SDEI", "Software Delegated Exceptions Interface" },
	{ "SLIC", "Microsoft Software Licensing Specification" },
	{ "SPCR", "Serial Port Console Redirection" },
	{ "SPMI", "Server Platform Management Interface" },
	{ "STAO", "_STA Override" },
	{ "TCPA", "Trusted Computing Platform Alliance Capabilities" },
	{ "TPM2", "Trusted Platform Module 2" },
	{ "UEFI", "UEFI ACPI Data" },
	{ "WAET", "Windows ACPI Emulated Devices" },
	{ "WDAT", "Watch Dog Action" },
	{ "WDRT", "Watchdog Resource" },
	{ "WPBT", "Platform Binary" },
	{ "WSMT", "Windows SMM Security Mitigation" },
	{ "XENV", "Xen Project" },

	/* Others */
	{ "NHLT", "Non-HD Audio" },
	{ "ASF!", "Alert Standard Format" },

	{ /* sentinel */ }
};

static struct acpi_sdt *acpi_get_dev_sdt(struct device *dev)
{
	int i;

	for (i = 0; i < dev->num_resources; i++) {
		if (!strcmp(dev->resource[i].name, "SDT"))
			return (struct acpi_sdt *)dev->resource[i].start;
	}

	return NULL;
}

static void acpi_devinfo(struct device *dev)
{
	struct acpi_sdt *sdt = acpi_get_dev_sdt(dev);
	struct sig_desc *sig_desc;

	printf("Signature: %.4s", sdt->signature);

	for (sig_desc = signatures; sig_desc->desc; sig_desc++) {
		size_t len = strnlen(sig_desc->sig, 4);

		if (!memcmp(sdt->signature, sig_desc->sig, len)) {
			printf(" (%s Table)", sig_desc->desc);
			break;
		}
	}

	printf("\nRevision: %u\n", sdt->revision);
	printf("OemId: %.6s\n", sdt->oem_id);
	printf("OemTableId: %.8s\n", sdt->oem_table_id);
	printf("OemRevision: %u\n", sdt->oem_revision);
	printf("CreatorId: 0x%08x\n", sdt->creator_id);
	printf("CreatorRevision: %u\n", sdt->creator_revision);
}

static int acpi_register_device(struct device *dev, struct acpi_sdt *sdt)
{
	device_add_resource(dev, "SDT", (resource_size_t)sdt, sdt->len,
		IORESOURCE_MEM | IORESOURCE_ROM_COPY | IORESOURCE_ROM_BIOS_COPY);

	dev_dbg(dev, "registering as ACPI device\n");

	return register_device(dev);
}

static struct device *acpi_add_device(struct bus_type *bus,
					acpi_sig_t signature)
{
	struct device *dev;

	dev = xzalloc(sizeof(*dev));

	dev->bus = bus;
	dev->parent = &bus->dev;
	dev->id = DEVICE_ID_DYNAMIC;
	devinfo_add(dev, acpi_devinfo);

	dev_set_name(dev, "acpi-%.4s", signature);

	return dev;
}

static int acpi_register_devices(struct bus_type *bus)
{
	struct efi_config_table *table = bus->dev.priv;
	struct acpi_rsdp *rsdp;
	struct acpi_rsdt *root;
	size_t entry_count;
	const char *sig;
	int i;

	rsdp = (struct acpi_rsdp *)table->table;

	if (!rsdp)
		return -EFAULT;

	/* ACPI specification v6.3
	 * 5.2.5.2 Finding the RSDP on UEFI Enabled Systems
	 */
	if (memcmp("RSD PTR ", rsdp->signature, sizeof(rsdp->signature))) {
		dev_dbg(&bus->dev, "unexpected signature at start of config table: '%.8s'\n",
			rsdp->signature);
		return -ENODEV;
	}

	if (rsdp->revision < 0x02) {
		sig = "RSDT";
		root = (struct acpi_rsdt *)(unsigned long)rsdp->rsdt_addr;
		entry_count = (root->sdt.len - sizeof(struct acpi_rsdt)) / sizeof(u32);
	} else if (sizeof(void *) == 8) {
		sig = "XSDT";
		root = (struct acpi_rsdt *)(uintptr_t)((struct acpi2_rsdp *)rsdp)->xsdt_addr;
		entry_count = (root->sdt.len - sizeof(struct acpi_rsdt)) / sizeof(u64);
	} else {
		return -EIO;
	}

	if (acpi_sigcmp(sig, root->sdt.signature)) {
		dev_err(&bus->dev, "Expected %s, but found '%.4s'.\n",
			sig, root->sdt.signature);
		return -EIO;
	}

	dev_info(&bus->dev, "Found %s (OEM: %.8s) with %zu entries\n",
		sig, root->sdt.oem_id, entry_count);

	for (i = 0; i < entry_count; i++) {
		struct acpi_sdt *sdt = root->entries[i];
		acpi_register_device(acpi_add_device(bus, sdt->signature), sdt);
	}

	return 0;
}

static int acpi_bus_match(struct device *dev, const struct driver *drv)
{
	const struct acpi_driver *acpidrv = to_acpi_driver(drv);
	struct acpi_sdt *sdt = acpi_get_dev_sdt(dev);

	return !acpi_sigcmp(acpidrv->signature, sdt->signature);
}

struct bus_type acpi_bus = {
	.name = "acpi",
	.match = acpi_bus_match,
};

static int efi_acpi_probe(void)
{
	struct efi_config_table *ect, *table = NULL;

	for_each_efi_config_table(ect) {
		/* take ACPI < 2 table only if no ACPI 2.0 is available */
		if (!efi_guidcmp(ect->guid, EFI_ACPI_20_TABLE_GUID)) {
			acpi_bus.name = "acpi2";
			table = ect;
		} else if (!table && !efi_guidcmp(ect->guid, EFI_ACPI_TABLE_GUID)) {
			acpi_bus.name = "acpi1";
			table = ect;
		}
	}

	bus_register(&acpi_bus);

	if (!table)
		return 0;

	acpi_bus.dev.priv = table;
	return acpi_register_devices(&acpi_bus);
}
postcore_efi_initcall(efi_acpi_probe);

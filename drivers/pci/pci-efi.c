// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */
#define pr_fmt(fmt) "pci-efi: " fmt

#include <common.h>
#include <driver.h>
#include <init.h>
#include <xfuncs.h>
#include <efi/payload.h>
#include <efi/payload/driver.h>
#include <efi/protocol/pci.h>
#include <efi/error.h>
#include <linux/pci.h>

struct efi_pci_priv {
	struct efi_pci_root_bridge_io_protocol *protocol;
	struct device *dev;
	struct pci_controller pci;
	struct resource mem;
	struct resource mem_pref;
	struct resource io;
	struct list_head children;
};

struct pci_child_id {
	size_t segmentno;
	size_t busno;
	size_t devno;
	size_t funcno;
};

struct pci_child {
	struct efi_pci_io_protocol *protocol;
	struct device *dev;
	struct list_head list;
	struct pci_child_id id;
};

static inline bool pci_child_id_equal(struct pci_child_id *a, struct pci_child_id *b)
{
	return a->segmentno == b->segmentno
		&& a->busno == b->busno
		&& a->devno == b->devno
		&& a->funcno == b->funcno;
}

#define host_to_efi_pci(host) container_of(host, struct efi_pci_priv, pci)

static inline u64 efi_pci_addr(struct pci_bus *bus, u32 devfn, int where)
{
	return EFI_PCI_ADDRESS(bus->number,
			       PCI_SLOT(devfn), PCI_FUNC(devfn),
			       where);
}

static int efi_pci_rd_conf(struct pci_bus *bus, u32 devfn, int where,
			   int size, u32 *val)
{
	struct efi_pci_priv *priv = host_to_efi_pci(bus->host);
	efi_status_t efiret;
	u32 value;
	enum efi_pci_protocol_width width;

	switch (size) {
	case 4:
		width = EFI_PCI_WIDTH_U32;
		break;
	case 2:
		width = EFI_PCI_WIDTH_U16;
		break;
	case 1:
		width = EFI_PCI_WIDTH_U8;
		break;
	default:
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	efiret = priv->protocol->pci.read(priv->protocol, width,
		efi_pci_addr(bus, devfn, where),
		1, &value);

	*val = 0xFFFFFFFF;

	if (EFI_ERROR(efiret))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	*val = value;

	return PCIBIOS_SUCCESSFUL;
}

static int efi_pci_wr_conf(struct pci_bus *bus, u32 devfn, int where,
			   int size, u32 val)
{
	struct efi_pci_priv *priv = host_to_efi_pci(bus->host);
	efi_status_t efiret;
	enum efi_pci_protocol_width width;

	switch (size) {
	case 4:
		width = EFI_PCI_WIDTH_U32;
		break;
	case 2:
		width = EFI_PCI_WIDTH_U16;
		break;
	case 1:
		width = EFI_PCI_WIDTH_U8;
		break;
	default:
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	efiret = priv->protocol->pci.write(priv->protocol, width,
		efi_pci_addr(bus, devfn, where),
		1, &val);
	if (EFI_ERROR(efiret))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

static inline struct resource build_resource(const char *name,
					     resource_size_t start,
					     resource_size_t len,
					     unsigned long flags)
{
	struct resource res;

	res.name = name;
	res.start = start;
	res.end = start + len - 1;
	res.flags = flags;
	res.parent = NULL;
	INIT_LIST_HEAD(&res.children);
	INIT_LIST_HEAD(&res.sibling);

	return res;
}

static const struct pci_ops efi_pci_ops = {
	.read = efi_pci_rd_conf,
	.write = efi_pci_wr_conf,
};

static u8 *acpi_parse_resource(u8 *next, struct resource *out)
{
	struct efi_acpi_resource *res;
	const char *name = NULL;
	unsigned long flags = 0;

	do {
		if (*next == ACPI_RESOURCE_END_TAG)
			return NULL;

		if (*next != ACPI_RESOURCE_DESC_TAG)
			return ERR_PTR(-EIO);

		res = container_of(next, struct efi_acpi_resource, asd);

		next = (u8 *)&res[1];
	} while (res->addr_len == 0);

	switch (res->restype) {
	case ACPI_RESOURCE_TYPE_MEM:
		if ((res->typflags & ACPI_RESOURCE_TYPFLAG_MTP_MASK)
		    != ACPI_RESOURCE_TYPFLAG_MTP_MEM)
			break;

		name = "NP-MEM";
		flags = IORESOURCE_MEM;

		switch (res->typflags & ACPI_RESOURCE_TYPFLAG_MEM_MASK) {
		case ACPI_RESOURCE_TYPFLAG_MEM_PREF:
			name = "P-MEM";
			flags |= IORESOURCE_PREFETCH;
			fallthrough;
		case ACPI_RESOURCE_TYPFLAG_MEM_WC:
		case ACPI_RESOURCE_TYPFLAG_MEM_CACHEABLE:
			flags |= IORESOURCE_CACHEABLE;
		}

		if (res->typflags & ACPI_RESOURCE_TYPFLAG_RW_MASK)
			flags |= IORESOURCE_MEM_WRITEABLE;

		break;
	case ACPI_RESOURCE_TYPE_IO:
		name = "IO";
		flags = IORESOURCE_IO;
		break;
	case ACPI_RESOURCE_TYPE_BUSNO:
		name = "BUS";
		flags = IORESOURCE_BUS;
		break;
	default:
		return ERR_PTR(-ENXIO);
	}

	*out = build_resource(name, res->addr_min, res->addr_len, flags);

	pr_debug("%s: %llx-%llx (len=%llx, gr=%lld, xlate_off=%llx, resflags=%08lx)\n",
		 out->name,
		 res->addr_min, res->addr_max, res->addr_len,
		 res->addr_granularity, res->addr_xlate_off,
		 flags);

	return next;
}

static struct efi_driver efi_pci_driver;

/* EFI already enumerated the bus for us, match our new pci devices with the efi
 * handles
 */
static void efi_pci_fixup_dev_parent(struct pci_dev *dev)
{
	struct efi_pci_priv *priv;
	struct pci_child *child;
	struct pci_child_id id;

	if (dev->dev.driver != &efi_pci_driver.driver)
		return;

	priv = host_to_efi_pci(dev->bus->host);

	id.segmentno = priv->protocol->segmentno;
	id.busno = dev->bus->number;
	id.devno = PCI_SLOT(dev->devfn);
	id.funcno = PCI_FUNC(dev->devfn);

	list_for_each_entry(child, &priv->children, list) {
		if (IS_ERR(child->protocol))
			continue;

		if (!child->protocol) {
			struct efi_device *efichild = to_efi_device(child->dev);
			efi_status_t efiret;

			BS->handle_protocol(efichild->handle, &EFI_PCI_IO_PROTOCOL_GUID,
					    (void **)&child->protocol);
			if (!child->protocol) {
				child->protocol = ERR_PTR(-ENODEV);
				continue;
			}

			efiret = child->protocol->get_location(child->protocol,
						       &child->id.segmentno,
						       &child->id.busno,
						       &child->id.devno,
						       &child->id.funcno);

			if (EFI_ERROR(efiret)) {
				child->protocol = ERR_PTR(-efi_errno(efiret));
				continue;
			}
		}

		if (pci_child_id_equal(&child->id, &id)) {
			dev->dev.priv = child->protocol;
			dev->dev.parent = child->dev;
			return;
		}
	}
}
DECLARE_PCI_FIXUP_EARLY(PCI_ANY_ID, PCI_ANY_ID, efi_pci_fixup_dev_parent);

static int efi_pci_probe(struct efi_device *efidev)
{
	struct device *child;
	struct efi_pci_priv *priv;
	efi_status_t efiret;
	void *resources;
	struct resource resource;
	u8 *res;

	priv = xzalloc(sizeof(*priv));

	priv->pci.parent = &efidev->dev;
	pci_controller_init(&priv->pci);

	BS->handle_protocol(efidev->handle, &EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID,
			(void **)&priv->protocol);
	if (!priv->protocol)
		return -ENODEV;

	efiret = priv->protocol->configuration(priv->protocol, &resources);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	res = resources;

	while (1) {
		res = acpi_parse_resource(res, &resource);

		if (IS_ERR(res))
			return PTR_ERR(res);

		if (!res)
			break;

		if ((resource.flags & (IORESOURCE_MEM | IORESOURCE_PREFETCH))
		    == (IORESOURCE_MEM | IORESOURCE_PREFETCH)) {
			priv->pci.mem_pref_resource = &priv->mem_pref;
			priv->mem_pref = resource;
		} else if (resource.flags & IORESOURCE_MEM) {
			priv->pci.mem_resource = &priv->mem;
			priv->mem = resource;
		} else if (resource.flags & IORESOURCE_IO) {
			priv->pci.io_resource = &priv->io;
			priv->io = resource;
		}
	}

	priv->pci.pci_ops = &efi_pci_ops;

	INIT_LIST_HEAD(&priv->children);

	device_for_each_child(&efidev->dev, child) {
		struct pci_child *pci_child;
		struct efi_device *efichild = to_efi_device(child);

		if (!efi_device_has_guid(efichild, EFI_PCI_IO_PROTOCOL_GUID))
			continue;

		pci_child = xzalloc(sizeof(*pci_child));

		pci_child->dev = &efichild->dev;

		/*
		 * regiser_pci_controller can reconfigure bridge bus numbers,
		 * thus we only collect the child node handles here, but
		 * don't yet call GetLocation on them
		 */
		list_add_tail(&pci_child->list, &priv->children);
	};

	register_pci_controller(&priv->pci);

	return 0;
}

static struct efi_driver efi_pci_driver = {
        .driver = {
		.name  = "efi-pci",
	},
        .probe = efi_pci_probe,
	.guid = EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID,
};
device_efi_driver(efi_pci_driver);

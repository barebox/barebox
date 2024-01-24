// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt)  "pci: " fmt

#include <common.h>
#include <linux/sizes.h>
#include <linux/pci.h>
#include <linux/bitfield.h>

static unsigned int pci_scan_bus(struct pci_bus *bus);

LIST_HEAD(pci_root_buses);
EXPORT_SYMBOL(pci_root_buses);
static u8 bus_index;
static resource_size_t last_mem;
static resource_size_t last_mem_pref;
static resource_size_t last_io;

static struct pci_bus *pci_alloc_bus(void)
{
	struct pci_bus *b;

	b = xzalloc(sizeof(*b));

	INIT_LIST_HEAD(&b->node);
	INIT_LIST_HEAD(&b->children);
	INIT_LIST_HEAD(&b->devices);

	return b;
}

static void pci_bus_register_devices(struct pci_bus *bus)
{
	struct pci_dev *dev;
	struct pci_bus *child_bus;

	/* activate all devices on this bus */
	list_for_each_entry(dev, &bus->devices, bus_list)
		pci_register_device(dev);

	/* walk down the hierarchy */
	list_for_each_entry(child_bus, &bus->children, node)
		pci_bus_register_devices(child_bus);
}

void register_pci_controller(struct pci_controller *hose)
{
	struct pci_bus *bus;

	bus = pci_alloc_bus();
	hose->bus = bus;
	bus->parent = hose->parent;
	bus->host = hose;
	bus->resource[PCI_BUS_RESOURCE_MEM] = hose->mem_resource;
	bus->resource[PCI_BUS_RESOURCE_MEM_PREF] = hose->mem_pref_resource;
	bus->resource[PCI_BUS_RESOURCE_IO] = hose->io_resource;

	if (pcibios_assign_all_busses()) {
		bus->number = bus_index++;
		if (hose->set_busno)
			hose->set_busno(hose, bus->number);

		if (bus->resource[PCI_BUS_RESOURCE_MEM])
			last_mem = bus->resource[PCI_BUS_RESOURCE_MEM]->start;
		else
			last_mem = 0;

		if (bus->resource[PCI_BUS_RESOURCE_MEM_PREF])
			last_mem_pref = bus->resource[PCI_BUS_RESOURCE_MEM_PREF]->start;
		else
			last_mem_pref = 0;

		if (bus->resource[PCI_BUS_RESOURCE_IO])
			last_io = bus->resource[PCI_BUS_RESOURCE_IO]->start;
		else
			last_io = 0;
	}

	pci_scan_bus(bus);
	pci_bus_register_devices(bus);

	list_add_tail(&bus->node, &pci_root_buses);

	return;
}

/*
 *  Wrappers for all PCI configuration access functions.  They just check
 *  alignment, do locking and call the low-level functions pointed to
 *  by pci_dev->ops.
 */

#define PCI_byte_BAD 0
#define PCI_word_BAD (pos & 1)
#define PCI_dword_BAD (pos & 3)

#define PCI_OP_READ(size,type,len) \
int pci_bus_read_config_##size \
	(struct pci_bus *bus, unsigned int devfn, int pos, type *value)	\
{									\
	int res;							\
	u32 data = 0;							\
	if (PCI_##size##_BAD) return PCIBIOS_BAD_REGISTER_NUMBER;	\
	res = bus->host->pci_ops->read(bus, devfn, pos, len, &data);	\
	*value = (type)data;						\
	return res;							\
}

#define PCI_OP_WRITE(size,type,len) \
int pci_bus_write_config_##size \
	(struct pci_bus *bus, unsigned int devfn, int pos, type value)	\
{									\
	int res;							\
	if (PCI_##size##_BAD) return PCIBIOS_BAD_REGISTER_NUMBER;	\
	res = bus->host->pci_ops->write(bus, devfn, pos, len, value);	\
	return res;							\
}

PCI_OP_READ(byte, u8, 1)
PCI_OP_READ(word, u16, 2)
PCI_OP_READ(dword, u32, 4)
PCI_OP_WRITE(byte, u8, 1)
PCI_OP_WRITE(word, u16, 2)
PCI_OP_WRITE(dword, u32, 4)

EXPORT_SYMBOL(pci_bus_read_config_byte);
EXPORT_SYMBOL(pci_bus_read_config_word);
EXPORT_SYMBOL(pci_bus_read_config_dword);
EXPORT_SYMBOL(pci_bus_write_config_byte);
EXPORT_SYMBOL(pci_bus_write_config_word);
EXPORT_SYMBOL(pci_bus_write_config_dword);

static struct pci_dev *alloc_pci_dev(void)
{
	struct pci_dev *dev;

	dev = xzalloc(sizeof(struct pci_dev));
	INIT_LIST_HEAD(&dev->bus_list);

	return dev;
}

static u32 pci_size(u32 base, u32 maxbase, u32 mask)
{
	u32 size = maxbase & mask;
	if (!size)
		return 0;

	size = (size & ~(size-1)) - 1;

	if (base == maxbase && ((base | size) & mask) != mask)
		return 0;

	return size + 1;
}

static unsigned long pci_ea_flags(struct pci_dev *dev, u8 prop)
{
	unsigned long flags = IORESOURCE_PCI_FIXED;

	switch (prop) {
	case PCI_EA_P_MEM:
	case PCI_EA_P_VF_MEM:
		flags |= IORESOURCE_MEM;
		break;
	case PCI_EA_P_MEM_PREFETCH:
	case PCI_EA_P_VF_MEM_PREFETCH:
		flags |= IORESOURCE_MEM | IORESOURCE_PREFETCH;
		break;
	case PCI_EA_P_IO:
		flags |= IORESOURCE_IO;
		break;
	default:
		return 0;
	}

	return flags;
}

static struct resource *pci_ea_get_resource(struct pci_dev *dev, u8 bei,
					    u8 prop)
{
	if (bei <= PCI_EA_BEI_BAR5 && prop <= PCI_EA_P_IO)
		return &dev->resource[bei];
	else if (bei == PCI_EA_BEI_ROM)
		return &dev->resource[PCI_ROM_RESOURCE];
	else
		return NULL;
}

/* Read an Enhanced Allocation (EA) entry */
static int pci_ea_read(struct pci_dev *dev, int offset)
{
	struct resource *res;
	int ent_size, ent_offset = offset;
	resource_size_t start, end;
	unsigned long flags;
	u32 dw0, bei, base, max_offset;
	u8 prop;
	bool support_64 = (sizeof(resource_size_t) >= 8);

	pci_read_config_dword(dev, ent_offset, &dw0);
	ent_offset += 4;

	/* Entry size field indicates DWORDs after 1st */
	ent_size = (FIELD_GET(PCI_EA_ES, dw0) + 1) << 2;

	if (!(dw0 & PCI_EA_ENABLE)) /* Entry not enabled */
		goto out;

	bei = FIELD_GET(PCI_EA_BEI, dw0);
	prop = FIELD_GET(PCI_EA_PP, dw0);

	/*
	 * If the Property is in the reserved range, try the Secondary
	 * Property instead.
	 */
	if (prop > PCI_EA_P_BRIDGE_IO && prop < PCI_EA_P_MEM_RESERVED)
		prop = FIELD_GET(PCI_EA_SP, dw0);
	if (prop > PCI_EA_P_BRIDGE_IO)
		goto out;

	res = pci_ea_get_resource(dev, bei, prop);
	if (!res) {
		dev_dbg(&dev->dev, "Unsupported EA entry BEI: %u\n", bei);
		goto out;
	}

	flags = pci_ea_flags(dev, prop);
	if (!flags) {
		dev_err(&dev->dev, "Unsupported EA properties: %#x\n", prop);
		goto out;
	}

	/* Read Base */
	pci_read_config_dword(dev, ent_offset, &base);
	start = (base & PCI_EA_FIELD_MASK);
	ent_offset += 4;

	/* Read MaxOffset */
	pci_read_config_dword(dev, ent_offset, &max_offset);
	ent_offset += 4;

	/* Read Base MSBs (if 64-bit entry) */
	if (base & PCI_EA_IS_64) {
		u32 base_upper;

		pci_read_config_dword(dev, ent_offset, &base_upper);
		ent_offset += 4;

		flags |= IORESOURCE_MEM_64;

		/* entry starts above 32-bit boundary, can't use */
		if (!support_64 && base_upper)
			goto out;

		if (support_64)
			start |= ((u64)base_upper << 32);
	}

	end = start + (max_offset | 0x03);

	/* Read MaxOffset MSBs (if 64-bit entry) */
	if (max_offset & PCI_EA_IS_64) {
		u32 max_offset_upper;

		pci_read_config_dword(dev, ent_offset, &max_offset_upper);
		ent_offset += 4;

		flags |= IORESOURCE_MEM_64;

		/* entry too big, can't use */
		if (!support_64 && max_offset_upper)
			goto out;

		if (support_64)
			end += ((u64)max_offset_upper << 32);
	}

	if (end < start) {
		dev_err(&dev->dev, "EA Entry crosses address boundary\n");
		goto out;
	}

	if (ent_size != ent_offset - offset) {
		dev_err(&dev->dev, "EA Entry Size (%d) does not match length read (%d)\n",
			ent_size, ent_offset - offset);
		goto out;
	}

	res->start = start;
	res->end = end;
	res->flags = flags;

	if (bei <= PCI_EA_BEI_BAR5)
		dev_dbg(&dev->dev, "BAR %d: %pR (from Enhanced Allocation, properties %#02x)\n",
			   bei, res, prop);
	else if (bei == PCI_EA_BEI_ROM)
		dev_dbg(&dev->dev, "ROM: %pR (from Enhanced Allocation, properties %#02x)\n",
			   res, prop);
	else if (bei >= PCI_EA_BEI_VF_BAR0 && bei <= PCI_EA_BEI_VF_BAR5)
		dev_dbg(&dev->dev, "VF BAR %d: %pR (from Enhanced Allocation, properties %#02x)\n",
			   bei - PCI_EA_BEI_VF_BAR0, res, prop);
	else
		dev_dbg(&dev->dev, "BEI %d res: %pR (from Enhanced Allocation, properties %#02x)\n",
			   bei, res, prop);

out:
	return offset + ent_size;
}

/* Enhanced Allocation Initialization */
static void pci_ea_init(struct pci_dev *dev)
{
	int ea;
	u8 num_ent;
	int offset;
	int i;

	/* find PCI EA capability in list */
	ea = pci_find_capability(dev, PCI_CAP_ID_EA);
	if (!ea)
		return;

	/* determine the number of entries */
	pci_bus_read_config_byte(dev->bus, dev->devfn, ea + PCI_EA_NUM_ENT,
					&num_ent);
	num_ent &= PCI_EA_NUM_ENT_MASK;

	offset = ea + PCI_EA_FIRST_ENT;

	/* Skip DWORD 2 for type 1 functions */
	if (dev->hdr_type == PCI_HEADER_TYPE_BRIDGE)
		offset += 4;

	/* parse each EA entry */
	for (i = 0; i < num_ent; ++i)
		offset = pci_ea_read(dev, offset);
}

static void setup_device(struct pci_dev *dev, int max_bar)
{
	int bar;
	u8 cmd;

	pci_read_config_byte(dev, PCI_COMMAND, &cmd);

	if (pcibios_assign_all_busses())
		pci_write_config_byte(dev, PCI_COMMAND,
				      cmd & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY));


	for (bar = 0; bar < max_bar; bar++) {
		const int pci_base_address_0 = PCI_BASE_ADDRESS_0 + bar * 4;
		const int pci_base_address_1 = PCI_BASE_ADDRESS_1 + bar * 4;
		resource_size_t *last_addr, start;
		u32 orig, mask, size;
		unsigned long flags;
		const char *kind;
		int busres;

		pci_read_config_dword(dev, pci_base_address_0, &orig);
		pci_write_config_dword(dev, pci_base_address_0, 0xfffffffe);
		pci_read_config_dword(dev, pci_base_address_0, &mask);
		pci_write_config_dword(dev, pci_base_address_0, orig);

		if (mask == 0 || mask == 0xffffffff) {
			pr_debug("pbar%d set bad mask\n", bar);
			continue;
		}

		if (mask & PCI_BASE_ADDRESS_SPACE_IO) { /* IO */
			size      = pci_size(orig, mask, 0xfffffffe);
			flags     = IORESOURCE_IO;
			kind      = "IO";
			last_addr = &last_io;
			busres    = PCI_BUS_RESOURCE_IO;
		} else if ((mask & PCI_BASE_ADDRESS_MEM_PREFETCH) &&
		           last_mem_pref) /* prefetchable MEM */ {
			size      = pci_size(orig, mask, 0xfffffff0);
			flags     = IORESOURCE_MEM | IORESOURCE_PREFETCH;
			kind      = "P-MEM";
			last_addr = &last_mem_pref;
			busres    = PCI_BUS_RESOURCE_MEM_PREF;
		} else { /* non-prefetch MEM */
			size      = pci_size(orig, mask, 0xfffffff0);
			flags     = IORESOURCE_MEM;
			kind      = "NP-MEM";
			last_addr = &last_mem;
			busres    = PCI_BUS_RESOURCE_MEM;
		}

		if (!size) {
			pr_debug("pbar%d bad %s mask\n", bar, kind);
			continue;
		}

		pr_debug("pbar%d: mask=%08x %s %d bytes\n", bar, mask, kind,
			 size);

		if (pcibios_assign_all_busses()) {
			if (ALIGN(*last_addr, size) + size >
			    dev->bus->resource[busres]->end) {
				pr_debug("BAR does not fit within bus %s res\n", kind);
				return;
			}

			*last_addr = ALIGN(*last_addr, size);
			pci_write_config_dword(dev, pci_base_address_0,
					       lower_32_bits(*last_addr));
			if (mask & PCI_BASE_ADDRESS_MEM_TYPE_64)
				pci_write_config_dword(dev, pci_base_address_1,
						       upper_32_bits(*last_addr));
			start = *last_addr;
			*last_addr += size;
		} else {
			u32 tmp;
			pci_read_config_dword(dev, pci_base_address_0, &tmp);
			tmp &= mask & PCI_BASE_ADDRESS_SPACE_IO ? PCI_BASE_ADDRESS_IO_MASK
				                                : PCI_BASE_ADDRESS_MEM_MASK;
			start = tmp;

			if (mask & PCI_BASE_ADDRESS_MEM_TYPE_64) {
				pci_read_config_dword(dev, pci_base_address_1, &tmp);
				start |= (u64)tmp << 32;
			}
		}

		dev->resource[bar].flags = flags;
		dev->resource[bar].start = start;
		dev->resource[bar].end = start + size - 1;

		if (mask & PCI_BASE_ADDRESS_MEM_TYPE_64) {
			dev->resource[bar].flags |= IORESOURCE_MEM_64;
			bar++;
		}
	}

	pci_ea_init(dev);

	pci_fixup_device(pci_fixup_header, dev);

	if (pcibios_assign_all_busses())
		pci_write_config_byte(dev, PCI_COMMAND, cmd);

	list_add_tail(&dev->bus_list, &dev->bus->devices);
}

static void prescan_setup_bridge(struct pci_dev *dev)
{
	u16 cmdstat;

	if (!pcibios_assign_all_busses()) {
		pci_read_config_byte(dev, PCI_PRIMARY_BUS, &dev->bus->number);
		pci_read_config_byte(dev, PCI_SECONDARY_BUS, &dev->subordinate->number);
		return;
	}

	pci_read_config_word(dev, PCI_COMMAND, &cmdstat);

	/* Configure bus number registers */
	pci_write_config_byte(dev, PCI_PRIMARY_BUS, dev->bus->number);
	pci_write_config_byte(dev, PCI_SECONDARY_BUS, dev->subordinate->number);
	pci_write_config_byte(dev, PCI_SUBORDINATE_BUS, 0xff);

	if (last_mem) {
		/* Set up memory and I/O filter limits, assume 32-bit I/O space */
		last_mem = ALIGN(last_mem, SZ_1M);
		pci_write_config_word(dev, PCI_MEMORY_BASE,
				      (last_mem & 0xfff00000) >> 16);
		cmdstat |= PCI_COMMAND_MEMORY;
	}

	if (last_mem_pref) {
		/* Set up memory and I/O filter limits, assume 32-bit I/O space */
		last_mem_pref = ALIGN(last_mem_pref, SZ_1M);
		pci_write_config_word(dev, PCI_PREF_MEMORY_BASE,
				      (last_mem_pref & 0xfff00000) >> 16);
		cmdstat |= PCI_COMMAND_MEMORY;
	} else {

		/* We don't support prefetchable memory for now, so disable */
		pci_write_config_word(dev, PCI_PREF_MEMORY_BASE, 0x1000);
		pci_write_config_word(dev, PCI_PREF_MEMORY_LIMIT, 0x0);
	}

	if (last_io) {
		last_io = ALIGN(last_io, SZ_4K);
		pci_write_config_byte(dev, PCI_IO_BASE,
				      (last_io & 0x0000f000) >> 8);
		pci_write_config_word(dev, PCI_IO_BASE_UPPER16,
				      (last_io & 0xffff0000) >> 16);
		cmdstat |= PCI_COMMAND_IO;
	}

	/* Enable memory and I/O accesses, enable bus master */
	pci_write_config_word(dev, PCI_COMMAND, cmdstat | PCI_COMMAND_MASTER);
}

static void postscan_setup_bridge(struct pci_dev *dev)
{
	if (!pcibios_assign_all_busses())
		return;

	/* limit subordinate to last used bus number */
	pci_write_config_byte(dev, PCI_SUBORDINATE_BUS, bus_index - 1);

	if (last_mem) {
		last_mem = ALIGN(last_mem, SZ_1M);
		pr_debug("bridge NP limit at %pa\n", &last_mem);
		pci_write_config_word(dev, PCI_MEMORY_LIMIT,
				      ((last_mem - 1) & 0xfff00000) >> 16);
	}

	if (last_mem_pref) {
		last_mem_pref = ALIGN(last_mem_pref, SZ_1M);
		pr_debug("bridge P limit at %pa\n", &last_mem_pref);
		pci_write_config_word(dev, PCI_PREF_MEMORY_LIMIT,
				      ((last_mem_pref - 1) & 0xfff00000) >> 16);
	}

	if (last_io) {
		last_io = ALIGN(last_io, SZ_4K);
		pr_debug("bridge IO limit at %pa\n", &last_io);
		pci_write_config_byte(dev, PCI_IO_LIMIT,
				((last_io - 1) & 0x0000f000) >> 8);
		pci_write_config_word(dev, PCI_IO_LIMIT_UPPER16,
				((last_io - 1) & 0xffff0000) >> 16);
	}
}

static struct device_node *
pci_of_match_device(struct device *parent, unsigned int devfn)
{
	struct device_node *np;
	u32 reg;

	if (!IS_ENABLED(CONFIG_OFTREE) || !parent || !parent->of_node)
		return NULL;

	for_each_child_of_node(parent->of_node, np) {
		if (!of_property_read_u32_array(np, "reg", &reg, 1)) {
			/*
			 * Only match device/function pair of the device
			 * address, other properties are defined by the
			 * PCI/OF node topology.
			 */
			reg = (reg >> 8) & 0xffff;
			if (reg == devfn)
				return np;
		}
	}

	return NULL;
}

/**
 * pcie_flr - initiate a PCIe function level reset
 * @dev:	device to reset
 *
 * Initiate a function level reset on @dev.
 */
int pci_flr(struct pci_dev *pdev)
{
	u16 val;
	int pcie_off;
	u32 cap;

	/* look for PCI Express Capability */
	pcie_off = pci_find_capability(pdev, PCI_CAP_ID_EXP);
	if (!pcie_off)
		return -ENOENT;

	/* check FLR capability */
	pci_read_config_dword(pdev, pcie_off + PCI_EXP_DEVCAP, &cap);
	if (!(cap & PCI_EXP_DEVCAP_FLR))
		return -ENOENT;

	pci_read_config_word(pdev, pcie_off + PCI_EXP_DEVCTL, &val);
	val |= PCI_EXP_DEVCTL_BCR_FLR;
	pci_write_config_word(pdev, pcie_off + PCI_EXP_DEVCTL, val);

	/* wait 100ms, per PCI spec */
	mdelay(100);

	return 0;
}

static unsigned int pci_scan_bus(struct pci_bus *bus)
{
	struct pci_dev *dev;
	struct pci_bus *child_bus;
	unsigned int devfn, l, max, class;
	unsigned char cmd, tmp, hdr_type, is_multi = 0;

	pr_debug("pci_scan_bus for bus %d\n", bus->number);
	pr_debug(" last_io = %pa, last_mem = %pa, last_mem_pref = %pa\n",
	    &last_io, &last_mem, &last_mem_pref);

	max = bus->secondary;

	for (devfn = 0; devfn < 0xff; ++devfn) {
		if (PCI_FUNC(devfn) && !is_multi) {
			/* not a multi-function device */
			continue;
		}
		if (pci_bus_read_config_byte(bus, devfn, PCI_HEADER_TYPE, &hdr_type))
			continue;
		if (!PCI_FUNC(devfn))
			is_multi = hdr_type & 0x80;

		if (pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &l) ||
		    /* some broken boards return 0 if a slot is empty: */
		    l == 0xffffffff || l == 0x00000000 || l == 0x0000ffff || l == 0xffff0000)
			continue;

		dev = alloc_pci_dev();
		dev->bus = bus;
		dev->devfn = devfn;
		dev->vendor = l & 0xffff;
		dev->device = (l >> 16) & 0xffff;
		dev->dev.parent = bus->parent;
		dev->dev.of_node = pci_of_match_device(bus->parent, devfn);
		if (dev->dev.of_node)
			pr_debug("found DT node %pOF for device %04x:%04x\n",
				 dev->dev.of_node,
				 dev->vendor, dev->device);

		/* non-destructively determine if device can be a master: */
		pci_read_config_byte(dev, PCI_COMMAND, &cmd);
		pci_write_config_byte(dev, PCI_COMMAND, cmd | PCI_COMMAND_MASTER);
		pci_read_config_byte(dev, PCI_COMMAND, &tmp);
		pci_write_config_byte(dev, PCI_COMMAND, cmd);

		pci_read_config_dword(dev, PCI_CLASS_REVISION, &class);
		dev->revision = class & 0xff;
		class >>= 8;				    /* upper 3 bytes */
		dev->class = class;
		class >>= 8;
		dev->hdr_type = hdr_type;

		pci_fixup_device(pci_fixup_early, dev);
		/* the fixup may have changed the device class */
		class = dev->class >> 8;

		pr_debug("class = %08x, hdr_type = %08x\n", class, hdr_type);
		pr_debug("%02x:%02x [%04x:%04x]\n", bus->number, dev->devfn,
		    dev->vendor, dev->device);

		switch (hdr_type & PCI_HEADER_TYPE_MASK) {
		case PCI_HEADER_TYPE_NORMAL:
			if (class == PCI_CLASS_BRIDGE_PCI)
				goto bad;

			setup_device(dev, 6);

			pci_read_config_word(dev, PCI_SUBSYSTEM_ID, &dev->subsystem_device);
			pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID, &dev->subsystem_vendor);
			break;
		case PCI_HEADER_TYPE_BRIDGE:
			child_bus = pci_alloc_bus();
			/* inherit parent properties */
			child_bus->host = bus->host;
			child_bus->parent_bus = bus;
			child_bus->resource[PCI_BUS_RESOURCE_MEM] =
				bus->resource[PCI_BUS_RESOURCE_MEM];
			child_bus->resource[PCI_BUS_RESOURCE_MEM_PREF] =
				bus->resource[PCI_BUS_RESOURCE_MEM_PREF];
			child_bus->resource[PCI_BUS_RESOURCE_IO] =
				bus->resource[PCI_BUS_RESOURCE_IO];

			child_bus->parent = &dev->dev;

			if (pcibios_assign_all_busses()) {
				child_bus->number = bus_index++;
				child_bus->primary = bus->number;
			}
			list_add_tail(&child_bus->node, &bus->children);
			dev->subordinate = child_bus;

			/* scan pci hierarchy behind bridge */
			prescan_setup_bridge(dev);
			pci_scan_bus(child_bus);
			postscan_setup_bridge(dev);

			setup_device(dev, 2);
			break;
		default:
		bad:
			printk(KERN_ERR "PCI: %02x:%02x [%04x/%04x/%06x] has unknown header type %02x, ignoring.\n",
			       bus->number, dev->devfn, dev->vendor, dev->device, class, hdr_type);
			continue;
		}
	}

	/*
	 * We've scanned the bus and so we know all about what's on
	 * the other side of any bridges that may be on this bus plus
	 * any devices.
	 *
	 * Return how far we've got finding sub-buses.
	 */
	max = bus_index;
	pr_debug("pci_scan_bus returning with max=%02x\n", max);

	return max;
}

static void __pci_set_master(struct pci_dev *dev, bool enable)
{
	u16 old_cmd, cmd;

	pci_read_config_word(dev, PCI_COMMAND, &old_cmd);
	if (enable)
		cmd = old_cmd | PCI_COMMAND_MASTER;
	else
		cmd = old_cmd & ~PCI_COMMAND_MASTER;
	if (cmd != old_cmd) {
		dev_dbg(&dev->dev, "%s bus mastering\n",
			enable ? "enabling" : "disabling");
		pci_write_config_word(dev, PCI_COMMAND, cmd);
	}
}

/**
 * pci_set_master - enables bus-mastering for device dev
 * @dev: the PCI device to enable
 */
void pci_set_master(struct pci_dev *dev)
{
	__pci_set_master(dev, true);
}
EXPORT_SYMBOL(pci_set_master);

/**
 * pci_clear_master - disables bus-mastering for device dev
 * @dev: the PCI device to disable
 */
void pci_clear_master(struct pci_dev *dev)
{
	__pci_set_master(dev, false);
}
EXPORT_SYMBOL(pci_clear_master);

/**
 * pci_enable_device - Initialize device before it's used by a driver.
 * @dev: PCI device to be initialized
 */
int pci_enable_device(struct pci_dev *dev)
{
	int ret;
	u32 t;

	pci_read_config_dword(dev, PCI_COMMAND, &t);
	ret = pci_write_config_dword(dev, PCI_COMMAND,
				     t | PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
	if (ret)
		return ret;

	pci_fixup_device(pci_fixup_enable, dev);

	return 0;
}
EXPORT_SYMBOL(pci_enable_device);

/**
 * pci_select_bars - Make BAR mask from the type of resource
 * @dev: the PCI device for which BAR mask is made
 * @flags: resource type mask to be selected
 *
 * This helper routine makes bar mask from the type of resource.
 */
int pci_select_bars(struct pci_dev *dev, unsigned long flags)
{
	int i, bars = 0;
	for (i = 0; i < PCI_NUM_RESOURCES; i++)
		if (pci_resource_flags(dev, i) & flags)
			bars |= (1 << i);
	return bars;
}

static u8 __pci_find_next_cap_ttl(struct pci_bus *bus, unsigned int devfn,
				  u8 pos, int cap, int *ttl)
{
	u8 id;
	u16 ent;

	pci_bus_read_config_byte(bus, devfn, pos, &pos);

	while ((*ttl)--) {
		if (pos < 0x40)
			break;
		pos &= ~3;
		pci_bus_read_config_word(bus, devfn, pos, &ent);

		id = ent & 0xff;
		if (id == 0xff)
			break;
		if (id == cap)
			return pos;
		pos = (ent >> 8);
	}
	return 0;
}

static u8 __pci_find_next_cap(struct pci_bus *bus, unsigned int devfn,
			      u8 pos, int cap)
{
	int ttl = PCI_FIND_CAP_TTL;

	return __pci_find_next_cap_ttl(bus, devfn, pos, cap, &ttl);
}

u8 pci_find_next_capability(struct pci_dev *dev, u8 pos, int cap)
{
	return __pci_find_next_cap(dev->bus, dev->devfn,
				   pos + PCI_CAP_LIST_NEXT, cap);
}
EXPORT_SYMBOL_GPL(pci_find_next_capability);

static u8 __pci_bus_find_cap_start(struct pci_bus *bus,
				    unsigned int devfn, u8 hdr_type)
{
	u16 status;

	pci_bus_read_config_word(bus, devfn, PCI_STATUS, &status);
	if (!(status & PCI_STATUS_CAP_LIST))
		return 0;

	switch (hdr_type) {
	case PCI_HEADER_TYPE_NORMAL:
	case PCI_HEADER_TYPE_BRIDGE:
		return PCI_CAPABILITY_LIST;
	case PCI_HEADER_TYPE_CARDBUS:
		return PCI_CB_CAPABILITY_LIST;
	}

	return 0;
}

/**
 * pci_find_capability - query for devices' capabilities
 * @dev: PCI device to query
 * @cap: capability code
 *
 * Tell if a device supports a given PCI capability.
 * Returns the address of the requested capability structure within the
 * device's PCI configuration space or 0 in case the device does not
 * support it.  Possible values for @cap include:
 *
 *  %PCI_CAP_ID_PM           Power Management
 *  %PCI_CAP_ID_AGP          Accelerated Graphics Port
 *  %PCI_CAP_ID_VPD          Vital Product Data
 *  %PCI_CAP_ID_SLOTID       Slot Identification
 *  %PCI_CAP_ID_MSI          Message Signalled Interrupts
 *  %PCI_CAP_ID_CHSWP        CompactPCI HotSwap
 *  %PCI_CAP_ID_PCIX         PCI-X
 *  %PCI_CAP_ID_EXP          PCI Express
 */
u8 pci_find_capability(struct pci_dev *dev, int cap)
{
	u8 pos;

	pos = __pci_bus_find_cap_start(dev->bus, dev->devfn, dev->hdr_type & PCI_HEADER_TYPE_MASK);
	if (pos)
		pos = __pci_find_next_cap(dev->bus, dev->devfn, pos, cap);

	return pos;
}
EXPORT_SYMBOL(pci_find_capability);

static void pci_do_fixups(struct pci_dev *dev, struct pci_fixup *f,
			  struct pci_fixup *end)
{
	for (; f < end; f++)
		if ((f->class == (u32) (dev->class >> f->class_shift) ||
		     f->class == (u32) PCI_ANY_ID) &&
		    (f->vendor == dev->vendor ||
		     f->vendor == (u16) PCI_ANY_ID) &&
		    (f->device == dev->device ||
		     f->device == (u16) PCI_ANY_ID)) {
			f->hook(dev);
		}
}

extern struct pci_fixup __start_pci_fixups_early[];
extern struct pci_fixup __end_pci_fixups_early[];
extern struct pci_fixup __start_pci_fixups_header[];
extern struct pci_fixup __end_pci_fixups_header[];
extern struct pci_fixup __start_pci_fixups_enable[];
extern struct pci_fixup __end_pci_fixups_enable[];

void pci_fixup_device(enum pci_fixup_pass pass, struct pci_dev *dev)
{
	struct pci_fixup *start, *end;

	switch (pass) {
	case pci_fixup_early:
		start = __start_pci_fixups_early;
		end = __end_pci_fixups_early;
		break;
	case pci_fixup_header:
		start = __start_pci_fixups_header;
		end = __end_pci_fixups_header;
		break;
	case pci_fixup_enable:
		start = __start_pci_fixups_enable;
		end = __end_pci_fixups_enable;
		break;
	default:
		unreachable();
	}
	pci_do_fixups(dev, start, end);
}

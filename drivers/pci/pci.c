#define pr_fmt(fmt)  "pci: " fmt

#include <common.h>
#include <linux/sizes.h>
#include <linux/pci.h>

static struct pci_controller *hose_head, **hose_tail = &hose_head;

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
	INIT_LIST_HEAD(&b->slots);
	INIT_LIST_HEAD(&b->resources);

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

	*hose_tail = hose;
	hose_tail = &hose->next;

	bus = pci_alloc_bus();
	hose->bus = bus;
	bus->parent = hose->parent;
	bus->host = hose;
	bus->ops = hose->pci_ops;
	bus->resource[PCI_BUS_RESOURCE_MEM] = hose->mem_resource;
	bus->resource[PCI_BUS_RESOURCE_MEM_PREF] = hose->mem_pref_resource;
	bus->resource[PCI_BUS_RESOURCE_IO] = hose->io_resource;
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
	res = bus->ops->read(bus, devfn, pos, len, &data);		\
	*value = (type)data;						\
	return res;							\
}

#define PCI_OP_WRITE(size,type,len) \
int pci_bus_write_config_##size \
	(struct pci_bus *bus, unsigned int devfn, int pos, type value)	\
{									\
	int res;							\
	if (PCI_##size##_BAD) return PCIBIOS_BAD_REGISTER_NUMBER;	\
	res = bus->ops->write(bus, devfn, pos, len, value);		\
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

	dev = kzalloc(sizeof(struct pci_dev), GFP_KERNEL);
	if (!dev)
		return NULL;

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


static void setup_device(struct pci_dev *dev, int max_bar)
{
	int bar;
	u8 cmd;

	pci_read_config_byte(dev, PCI_COMMAND, &cmd);
	pci_write_config_byte(dev, PCI_COMMAND,
			      cmd & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY));

	for (bar = 0; bar < max_bar; bar++) {
		resource_size_t last_addr;
		u32 orig, mask, size;

		pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &orig);
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, 0xfffffffe);
		pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &mask);
		pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, orig);

		if (mask == 0 || mask == 0xffffffff) {
			pr_debug("pbar%d set bad mask\n", bar);
			continue;
		}

		if (mask & 0x01) { /* IO */
			size = pci_size(orig, mask, 0xfffffffe);
			if (!size) {
				pr_debug("pbar%d bad IO mask\n", bar);
				continue;
			}
			pr_debug("pbar%d: mask=%08x io %d bytes\n", bar, mask, size);
			if (ALIGN(last_io, size) + size >
			    dev->bus->resource[PCI_BUS_RESOURCE_IO]->end) {
				pr_debug("BAR does not fit within bus IO res\n");
				return;
			}
			last_io = ALIGN(last_io, size);
			pr_debug("pbar%d: allocated at 0x%08x\n", bar, last_io);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_io);
			dev->resource[bar].flags = IORESOURCE_IO;
			last_addr = last_io;
			last_io += size;
		} else if ((mask & PCI_BASE_ADDRESS_MEM_PREFETCH) &&
		           last_mem_pref) /* prefetchable MEM */ {
			size = pci_size(orig, mask, 0xfffffff0);
			if (!size) {
				pr_debug("pbar%d bad P-MEM mask\n", bar);
				continue;
			}
			pr_debug("pbar%d: mask=%08x P memory %d bytes\n",
			    bar, mask, size);
			if (ALIGN(last_mem_pref, size) + size >
			    dev->bus->resource[PCI_BUS_RESOURCE_MEM_PREF]->end) {
				pr_debug("BAR does not fit within bus p-mem res\n");
				return;
			}
			last_mem_pref = ALIGN(last_mem_pref, size);
			pr_debug("pbar%d: allocated at 0x%08x\n", bar, last_mem_pref);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_mem_pref);
			dev->resource[bar].flags = IORESOURCE_MEM |
			                           IORESOURCE_PREFETCH;
			last_addr = last_mem_pref;
			last_mem_pref += size;
		} else { /* non-prefetch MEM */
			size = pci_size(orig, mask, 0xfffffff0);
			if (!size) {
				pr_debug("pbar%d bad NP-MEM mask\n", bar);
				continue;
			}
			pr_debug("pbar%d: mask=%08x NP memory %d bytes\n",
			    bar, mask, size);
			if (ALIGN(last_mem, size) + size >
			    dev->bus->resource[PCI_BUS_RESOURCE_MEM]->end) {
				pr_debug("BAR does not fit within bus np-mem res\n");
				return;
			}
			last_mem = ALIGN(last_mem, size);
			pr_debug("pbar%d: allocated at 0x%08x\n", bar, last_mem);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_mem);
			dev->resource[bar].flags = IORESOURCE_MEM;
			last_addr = last_mem;
			last_mem += size;
		}

		dev->resource[bar].start = last_addr;
		dev->resource[bar].end = last_addr + size - 1;

		if ((mask & PCI_BASE_ADDRESS_MEM_TYPE_64)) {
			dev->resource[bar].flags |= IORESOURCE_MEM_64;
			pci_write_config_dword(dev,
			       PCI_BASE_ADDRESS_1 + bar * 4, 0);
			bar++;
		}
	}

	pci_write_config_byte(dev, PCI_COMMAND, cmd);
	list_add_tail(&dev->bus_list, &dev->bus->devices);
}

static void prescan_setup_bridge(struct pci_dev *dev)
{
	u16 cmdstat;

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
	/* limit subordinate to last used bus number */
	pci_write_config_byte(dev, PCI_SUBORDINATE_BUS, bus_index - 1);

	if (last_mem) {
		last_mem = ALIGN(last_mem, SZ_1M);
		pr_debug("bridge NP limit at 0x%08x\n", last_mem);
		pci_write_config_word(dev, PCI_MEMORY_LIMIT,
				      ((last_mem - 1) & 0xfff00000) >> 16);
	}

	if (last_mem_pref) {
		last_mem_pref = ALIGN(last_mem_pref, SZ_1M);
		pr_debug("bridge P limit at 0x%08x\n", last_mem_pref);
		pci_write_config_word(dev, PCI_PREF_MEMORY_LIMIT,
				      ((last_mem_pref - 1) & 0xfff00000) >> 16);
	}

	if (last_io) {
		last_io = ALIGN(last_io, SZ_4K);
		pr_debug("bridge IO limit at 0x%08x\n", last_io);
		pci_write_config_byte(dev, PCI_IO_LIMIT,
				((last_io - 1) & 0x0000f000) >> 8);
		pci_write_config_word(dev, PCI_IO_LIMIT_UPPER16,
				((last_io - 1) & 0xffff0000) >> 16);
	}
}

unsigned int pci_scan_bus(struct pci_bus *bus)
{
	struct pci_dev *dev;
	struct pci_bus *child_bus;
	unsigned int devfn, l, max, class;
	unsigned char cmd, tmp, hdr_type, is_multi = 0;

	pr_debug("pci_scan_bus for bus %d\n", bus->number);
	pr_debug(" last_io = 0x%08x, last_mem = 0x%08x, last_mem_pref = 0x%08x\n",
	    last_io, last_mem, last_mem_pref);

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
		if (!dev)
			return 0;

		dev->bus = bus;
		dev->devfn = devfn;
		dev->vendor = l & 0xffff;
		dev->device = (l >> 16) & 0xffff;
		dev->dev.parent = bus->parent;

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

		pr_debug("class = %08x, hdr_type = %08x\n", class, hdr_type);
		pr_debug("%02x:%02x [%04x:%04x]\n", bus->number, dev->devfn,
		    dev->vendor, dev->device);

		switch (hdr_type & 0x7f) {
		case PCI_HEADER_TYPE_NORMAL:
			if (class == PCI_CLASS_BRIDGE_PCI)
				goto bad;

			/*
			 * read base address registers, again pcibios_fixup() can
			 * tweak these
			 */
			pci_read_config_dword(dev, PCI_ROM_ADDRESS, &l);
			dev->rom_address = (l == 0xffffffff) ? 0 : l;

			setup_device(dev, 6);
			break;
		case PCI_HEADER_TYPE_BRIDGE:
			child_bus = pci_alloc_bus();
			/* inherit parent properties */
			child_bus->host = bus->host;
			child_bus->ops = bus->host->pci_ops;
			child_bus->parent_bus = bus;
			child_bus->resource[PCI_BUS_RESOURCE_MEM] =
				bus->resource[PCI_BUS_RESOURCE_MEM];
			child_bus->resource[PCI_BUS_RESOURCE_MEM_PREF] =
				bus->resource[PCI_BUS_RESOURCE_MEM_PREF];
			child_bus->resource[PCI_BUS_RESOURCE_IO] =
				bus->resource[PCI_BUS_RESOURCE_IO];

			child_bus->parent = &dev->dev;
			child_bus->number = bus_index++;
			child_bus->primary = bus->number;
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
	u32 t;

	pci_read_config_dword(dev, PCI_COMMAND, &t);
	return pci_write_config_dword(dev, PCI_COMMAND, t
				| PCI_COMMAND_IO
				| PCI_COMMAND_MEMORY
				);
}
EXPORT_SYMBOL(pci_enable_device);

#include <common.h>
#include <linux/pci.h>

#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

static struct pci_controller *hose_head, **hose_tail = &hose_head;

LIST_HEAD(pci_root_buses);
EXPORT_SYMBOL(pci_root_buses);
static u8 bus_index;

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

void register_pci_controller(struct pci_controller *hose)
{
	struct pci_bus *bus;

	*hose_tail = hose;
	hose_tail = &hose->next;

	bus = pci_alloc_bus();
	hose->bus = bus;
	bus->host = hose;
	bus->ops = hose->pci_ops;
	bus->resource[0] = hose->mem_resource;
	bus->resource[1] = hose->io_resource;
	bus->number = bus_index++;

	if (hose->set_busno)
		hose->set_busno(hose, bus->number);
	pci_scan_bus(bus);

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

unsigned int pci_scan_bus(struct pci_bus *bus)
{
	unsigned int devfn, l, max, class;
	unsigned char cmd, tmp, hdr_type, is_multi = 0;
	struct pci_dev *dev;
	resource_size_t last_mem;
	resource_size_t last_io;

	/* FIXME: use res_start() */
	last_mem = bus->resource[0]->start;
	last_io = bus->resource[1]->start;

	DBG("pci_scan_bus for bus %d\n", bus->number);
	DBG(" last_io = 0x%08x, last_mem = 0x%08x\n", last_io, last_mem);
	max = bus->secondary;

	for (devfn = 0; devfn < 0xff; ++devfn) {
		int bar;
		u32 old_bar, mask;
		int size;

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

		DBG("PCI: class = %08x, hdr_type = %08x\n", class, hdr_type);

		switch (hdr_type & 0x7f) {		    /* header type */
		case PCI_HEADER_TYPE_NORMAL:		    /* standard header */
			if (class == PCI_CLASS_BRIDGE_PCI)
				goto bad;

			/*
			 * read base address registers, again pcibios_fixup() can
			 * tweak these
			 */
			pci_read_config_dword(dev, PCI_ROM_ADDRESS, &l);
			dev->rom_address = (l == 0xffffffff) ? 0 : l;
			break;
		default:				    /* unknown header */
		bad:
			printk(KERN_ERR "PCI: %02x:%02x [%04x/%04x/%06x] has unknown header type %02x, ignoring.\n",
			       bus->number, dev->devfn, dev->vendor, dev->device, class, hdr_type);
			continue;
		}

		DBG("PCI: %02x:%02x [%04x/%04x]\n", bus->number, dev->devfn, dev->vendor, dev->device);

		if (class == PCI_CLASS_BRIDGE_HOST) {
			DBG("PCI: skip pci host bridge\n");
			continue;
		}

		pci_read_config_byte(dev, PCI_COMMAND, &cmd);
		pci_write_config_byte(dev, PCI_COMMAND,
				      cmd & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY));

		for (bar = 0; bar < 6; bar++) {
			resource_size_t last_addr;

			pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &old_bar);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, 0xfffffffe);
			pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &mask);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, old_bar);

			if (mask == 0 || mask == 0xffffffff) {
				DBG("  PCI: pbar%d set bad mask\n", bar);
				continue;
			}

			if (mask & 0x01) { /* IO */
				size = -(mask & 0xfffffffe);
				DBG("  PCI: pbar%d: mask=%08x io %d bytes\n", bar, mask, size);
				pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_io);
				dev->resource[bar].flags = IORESOURCE_IO;
				last_addr = last_io;
				last_io += size;
			} else { /* MEM */
				size = -(mask & 0xfffffff0);
				DBG("  PCI: pbar%d: mask=%08x memory %d bytes\n", bar, mask, size);
				pci_write_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, last_mem);
				dev->resource[bar].flags = IORESOURCE_MEM;
				last_addr = last_mem;
				last_mem += size;

				if ((mask & PCI_BASE_ADDRESS_MEM_TYPE_MASK) ==
				    PCI_BASE_ADDRESS_MEM_TYPE_64) {
					dev->resource[bar].flags |= IORESOURCE_MEM_64;
					pci_write_config_dword(dev,
					       PCI_BASE_ADDRESS_1 + bar * 4, 0);
				}
			}

			dev->resource[bar].start = last_addr;
			dev->resource[bar].end = last_addr + size - 1;
			if (dev->resource[bar].flags & IORESOURCE_MEM_64)
				bar++;
		}

		pci_write_config_byte(dev, PCI_COMMAND, cmd);
		list_add_tail(&dev->bus_list, &bus->devices);
		pci_register_device(dev);
	}

	/*
	 * We've scanned the bus and so we know all about what's on
	 * the other side of any bridges that may be on this bus plus
	 * any devices.
	 *
	 * Return how far we've got finding sub-buses.
	 */
	DBG("PCI: pci_scan_bus returning with max=%02x\n", max);

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

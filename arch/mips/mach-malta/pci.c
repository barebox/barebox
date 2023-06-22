// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/addrspace.h>

#include <linux/pci.h>
#include <asm/gt64120.h>

#include <mach/mach-gt64120.h>

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

static struct resource gt64120_mem_resource = {
	.name	= "GT-64120 PCI MEM",
	.flags	= IORESOURCE_MEM,
};

static struct resource gt64120_io_resource = {
	.name	= "GT-64120 PCI I/O",
	.flags	= IORESOURCE_IO,
};

static int gt64xxx_pci0_pcibios_config_access(unsigned char access_type,
		struct pci_bus *bus, unsigned int devfn, int where, u32 *data)
{
	unsigned char busnum = bus->number;
	u32 intr;

	if ((busnum == 0) && (devfn >= PCI_DEVFN(31, 0)))
		return -1;	/* Because of a bug in the galileo (for slot 31). */

	/* Clear cause register bits */
	GT_WRITE(GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT |
					GT_INTRCAUSE_TARABORT0_BIT));

	/* Setup address */
	GT_WRITE(GT_PCI0_CFGADDR_OFS,
		 (busnum << GT_PCI0_CFGADDR_BUSNUM_SHF) |
		 (devfn << GT_PCI0_CFGADDR_FUNCTNUM_SHF) |
		 ((where / 4) << GT_PCI0_CFGADDR_REGNUM_SHF) |
		 GT_PCI0_CFGADDR_CONFIGEN_BIT);

	if (access_type == PCI_ACCESS_WRITE) {
		if (busnum == 0 && PCI_SLOT(devfn) == 0) {
			/*
			 * The Galileo system controller is acting
			 * differently than other devices.
			 */
			GT_WRITE(GT_PCI0_CFGDATA_OFS, *data);
		} else
			__GT_WRITE(GT_PCI0_CFGDATA_OFS, *data);
	} else {
		if (busnum == 0 && PCI_SLOT(devfn) == 0) {
			/*
			 * The Galileo system controller is acting
			 * differently than other devices.
			 */
			*data = GT_READ(GT_PCI0_CFGDATA_OFS);
		} else
			*data = __GT_READ(GT_PCI0_CFGDATA_OFS);
	}

	/* Check for master or target abort */
	intr = GT_READ(GT_INTRCAUSE_OFS);

	if (intr & (GT_INTRCAUSE_MASABORT0_BIT | GT_INTRCAUSE_TARABORT0_BIT)) {
		/* Error occurred */

		/* Clear bits */
		GT_WRITE(GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT |
						GT_INTRCAUSE_TARABORT0_BIT));

		return -1;
	}

	return 0;
}

/*
 * We can't address 8 and 16 bit words directly. Instead we have to
 * read/write a 32bit word and mask/modify the data we actually want.
 */
static int gt64xxx_pci0_pcibios_read(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 *val)
{
	u32 data = 0;

	if (gt64xxx_pci0_pcibios_config_access(PCI_ACCESS_READ, bus, devfn,
						where, &data))
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (size == 1)
		*val = (data >> ((where & 3) << 3)) & 0xff;
	else if (size == 2)
		*val = (data >> ((where & 3) << 3)) & 0xffff;
	else
		*val = data;

	return PCIBIOS_SUCCESSFUL;
}

static int gt64xxx_pci0_pcibios_write(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 val)
{
	u32 data = 0;

	if (size == 4)
		data = val;
	else {
		if (gt64xxx_pci0_pcibios_config_access(PCI_ACCESS_READ, bus,
							devfn, where, &data))
			return PCIBIOS_DEVICE_NOT_FOUND;

		if (size == 1)
			data = (data & ~(0xff << ((where & 3) << 3))) |
				(val << ((where & 3) << 3));
		else if (size == 2)
			data = (data & ~(0xffff << ((where & 3) << 3))) |
				(val << ((where & 3) << 3));
	}

	if (gt64xxx_pci0_pcibios_config_access(PCI_ACCESS_WRITE, bus, devfn,
						where, &data))
		return PCIBIOS_DEVICE_NOT_FOUND;

	return PCIBIOS_SUCCESSFUL;
}

/* function returns memory address for begin of pci resource */
static resource_size_t gt64xxx_res_start(struct pci_bus *bus,
					 resource_size_t res_addr)
{
	return CKSEG0ADDR(res_addr);
}

struct pci_ops gt64xxx_pci0_ops = {
	.read	= gt64xxx_pci0_pcibios_read,
	.write	= gt64xxx_pci0_pcibios_write,

	.res_start = gt64xxx_res_start,
};

static struct pci_controller gt64120_controller = {
	.pci_ops	= &gt64xxx_pci0_ops,
	.io_resource	= &gt64120_io_resource,
	.mem_resource	= &gt64120_mem_resource,
};

static int pcibios_init(void)
{
	resource_size_t start, end, map, start1, end1, map1, mask;

	/*
	 * Due to a bug in the Galileo system controller, we need
	 * to setup the PCI BAR for the Galileo internal registers.
	 * This should be done in the bios/bootprom and will be
	 * fixed in a later revision of YAMON (the MIPS boards
	 * boot prom).
	 */
	GT_WRITE(GT_PCI0_CFGADDR_OFS,
		 (0 << GT_PCI0_CFGADDR_BUSNUM_SHF) | /* Local bus */
		 (0 << GT_PCI0_CFGADDR_DEVNUM_SHF) | /* GT64120 dev */
		 (0 << GT_PCI0_CFGADDR_FUNCTNUM_SHF) | /* Function 0*/
		 ((0x20/4) << GT_PCI0_CFGADDR_REGNUM_SHF) | /* BAR 4*/
		 GT_PCI0_CFGADDR_CONFIGEN_BIT);

	/* Perform the write */
	GT_WRITE(GT_PCI0_CFGDATA_OFS, CPHYSADDR(MIPS_GT_BASE));

	/* Here is linux code. It assumes, that firmware
	(pbl in case of barebox) made the work... */

	/* Set up resource ranges from the controller's registers. */
	start = GT_READ(GT_PCI0M0LD_OFS);
	end = GT_READ(GT_PCI0M0HD_OFS);
	map = GT_READ(GT_PCI0M0REMAP_OFS);
	end = (end & GT_PCI_HD_MSK) | (start & ~GT_PCI_HD_MSK);
	start1 = GT_READ(GT_PCI0M1LD_OFS);
	end1 = GT_READ(GT_PCI0M1HD_OFS);
	map1 = GT_READ(GT_PCI0M1REMAP_OFS);
	end1 = (end1 & GT_PCI_HD_MSK) | (start1 & ~GT_PCI_HD_MSK);

	mask = ~(start ^ end);

	/* We don't support remapping with a discontiguous mask. */
	BUG_ON((start & GT_PCI_HD_MSK) != (map & GT_PCI_HD_MSK) &&
		mask != ~((mask & -mask) - 1));
	gt64120_mem_resource.start = start;
	gt64120_mem_resource.end = end;
	gt64120_controller.mem_offset = (start & mask) - (map & mask);
	/* Addresses are 36-bit, so do shifts in the destinations. */
	gt64120_mem_resource.start <<= GT_PCI_DCRM_SHF;
	gt64120_mem_resource.end <<= GT_PCI_DCRM_SHF;
	gt64120_mem_resource.end |= (1 << GT_PCI_DCRM_SHF) - 1;
	gt64120_controller.mem_offset <<= GT_PCI_DCRM_SHF;

	start = GT_READ(GT_PCI0IOLD_OFS);
	end = GT_READ(GT_PCI0IOHD_OFS);
	map = GT_READ(GT_PCI0IOREMAP_OFS);
	end = (end & GT_PCI_HD_MSK) | (start & ~GT_PCI_HD_MSK);
	mask = ~(start ^ end);

	/* We don't support remapping with a discontiguous mask. */
	BUG_ON((start & GT_PCI_HD_MSK) != (map & GT_PCI_HD_MSK) &&
		mask != ~((mask & -mask) - 1));
	gt64120_io_resource.start = map & mask;
	gt64120_io_resource.end = (map & mask) | ~mask;
	gt64120_controller.io_offset = 0;
	/* Addresses are 36-bit, so do shifts in the destinations. */
	gt64120_io_resource.start <<= GT_PCI_DCRM_SHF;
	gt64120_io_resource.end <<= GT_PCI_DCRM_SHF;
	gt64120_io_resource.end |= (1 << GT_PCI_DCRM_SHF) - 1;

#ifdef CONFIG_CPU_LITTLE_ENDIAN
	GT_WRITE(GT_PCI0_CMD_OFS, GT_PCI0_CMD_MBYTESWAP_BIT |
		GT_PCI0_CMD_SBYTESWAP_BIT);
#else
	GT_WRITE(GT_PCI0_CMD_OFS, 0);
#endif

	/* Fix up PCI I/O mapping if necessary (for Atlas). */
	start = GT_READ(GT_PCI0IOLD_OFS);
	map = GT_READ(GT_PCI0IOREMAP_OFS);
	if ((start & map) != 0) {
		map &= ~start;
		GT_WRITE(GT_PCI0IOREMAP_OFS, map);
	}

	register_pci_controller(&gt64120_controller);

	return 0;
}
postcore_initcall(pcibios_init);

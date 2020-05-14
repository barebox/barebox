// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011-2014 Antony Pavlov <antonynpavlov@gmail.com>

#include <common.h>
#include <command.h>
#include <complete.h>
#include <linux/pci.h>

static void traverse_bus(struct pci_bus *bus)
{
	struct pci_dev *dev;

	list_for_each_entry(dev, &bus->devices, bus_list) {
		printf("%02x:%02x.%1x %04x: %04x:%04x (rev %02x)\n",
		       dev->bus->number, PCI_SLOT(dev->devfn),
		       PCI_FUNC(dev->devfn), (dev->class >> 8) & 0xffff,
		       dev->vendor, dev->device, dev->revision);

		if (dev->subordinate)
			traverse_bus(dev->subordinate);
	}
}

static int do_lspci(int argc, char *argv[])
{
	struct pci_bus *root_bus;

	if (list_empty(&pci_root_buses)) {
		printf("No PCI bus detected\n");
		return 1;
	}

	list_for_each_entry(root_bus, &pci_root_buses, node) {
		traverse_bus(root_bus);
	}

	return 0;
}

BAREBOX_CMD_START(lspci)
	.cmd            = do_lspci,
	BAREBOX_CMD_DESC("show PCI info")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

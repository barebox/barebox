/*
 * Copyright (C) 2011-2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <linux/pci.h>

static int do_lspci(int argc, char *argv[])
{
	struct pci_bus *root_bus;
	struct pci_dev *dev;

	if (list_empty(&pci_root_buses)) {
		printf("No PCI bus detected\n");
		return 1;
	}

	list_for_each_entry(root_bus, &pci_root_buses, node) {
		list_for_each_entry(dev, &root_bus->devices, bus_list) {
			printf("%02x: %04x: %04x:%04x (rev %02x)\n",
				      dev->devfn,
				      (dev->class >> 8) & 0xffff,
				      dev->vendor,
				      dev->device,
				      dev->revision);
		}
	}

	return 0;
}

BAREBOX_CMD_START(lspci)
	.cmd            = do_lspci,
	BAREBOX_CMD_DESC("show PCI info")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END

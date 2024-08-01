/* SPDX-License-Identifier: GPL-2.0-only */

#include <asm/io.h>
#include <driver.h>
#include <of.h>

unsigned char __pci_iobase[IO_SPACE_LIMIT];

static int ioport_bus_probe(struct device *dev)
{
	struct device_node *bus = dev->of_node;
	u64 arr[3] = {
		0,				/* child bus address */
		virt_to_phys(__pci_iobase),	/* parent bus address */
		sizeof(__pci_iobase),		/* resource size */
	};

	of_property_write_u64_array(bus, "reg", &arr[1], 2);
	of_property_write_u64_array(bus, "ranges", &arr[0], 3);

	return of_platform_populate(bus, of_default_bus_match_table, dev);
}

static __maybe_unused struct of_device_id sandbox_ioport_dt_ids[] = {
	{ .compatible = "barebox,sandbox-ioport-bus", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sandbox_ioport_dt_ids);

static struct driver sandbox_ioport_driver = {
	.name  = "sandbox-ioport-bus",
	.of_compatible = sandbox_ioport_dt_ids,
	.probe = ioport_bus_probe,
};
postcore_platform_driver(sandbox_ioport_driver);

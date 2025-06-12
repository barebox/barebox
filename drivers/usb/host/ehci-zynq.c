// SPDX-License-Identifier: GPL-2.0+
/*
 * USB Low level initialization(Specific to Zynq 7000)
 */

#include <common.h>
#include <linux/usb/ulpi.h>

static int zynq_ehci_probe(struct device_d *dev)
{
	struct resource *res;
	void __iomem *base;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	base = IOMEM(res->start);

	ulpi_setup(base + 0x170, 1);
	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, (unsigned int)base, NULL);

	return 0;
}

static const struct of_device_id zynq_ehci_dt_ids[] = {
	{ .compatible = "xlnx,zynq-usb-2.20a" },
	{ /* sentinel */ }
};

static struct driver_d zynq_ehci_driver = {
	.name = "zynq-ehci",
	.probe = zynq_ehci_probe,
	.of_compatible = DRV_OF_COMPAT(zynq_ehci_dt_ids),
};
device_platform_driver(zynq_ehci_driver);

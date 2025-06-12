// SPDX-License-Identifier: GPL-2.0-only

#include <driver.h>
#include <tlv/tlv.h>
#include <init.h>
#include <of.h>
#include <of_device.h>
#include <stdio.h>

static int barebox_tlv_probe(struct device *dev)
{
	const struct of_device_id *match;
	struct tlv_device *tlvdev;
	struct cdev *cdev;
	char *backend_path;

	match = of_match_device(dev->driver->of_match_table, dev);
	/*
	 * We only match with the device if this driver is the most specific match
	 * because we don't want to incorrectly bind to a device that has a more
	 * specific driver.
	 */
	if (match && of_property_match_string(dev->of_node, "compatible",
					      match->compatible) != 0)
		return -ENODEV;

	cdev = cdev_by_device_node(dev->of_node);
	if (!cdev)
		return -EINVAL;

	backend_path = basprintf("/dev/%s", cdev_name(cdev));

	tlvdev = tlv_register_device_by_path(backend_path, dev);
	free(backend_path);

	return PTR_ERR_OR_ZERO(tlvdev);
}

static const __maybe_unused struct of_device_id tlv_ids[] = {
	{ .compatible = "barebox,tlv" },
	{ /* sentinel */ }
};

static struct driver barebox_tlv_driver = {
	.name = "tlv",
	.probe = barebox_tlv_probe,
	.of_compatible = tlv_ids,
};
core_platform_driver(barebox_tlv_driver);

// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <driver.h>
#include <init.h>
#include <tlv/tlv.h>
#include <linux/err.h>
#include <of.h>

static void tlv_devinfo(struct device *dev)
{
	struct tlv_device *tlvdev = to_tlv_device(dev);

	printf("Magic: %08x\n", tlvdev->magic);
}

struct tlv_device *tlv_register_device(struct tlv_header *header,
				       struct device *parent)
{
	struct tlv_device *tlvdev;
	const char *name = NULL;
	char *buf = NULL;
	struct device *dev;
	static int id = 0;

	tlvdev = xzalloc(sizeof(*tlvdev));

	dev = &tlvdev->dev;

	dev->bus = &tlv_bus;
	devinfo_add(dev, tlv_devinfo);
	dev->platform_data = header;
	tlvdev->magic = be32_to_cpu(header->magic);
	dev->parent = parent ?: tlv_bus.dev;
	dev->id = DEVICE_ID_SINGLE;

	if (parent)
		name = of_alias_get(parent->device_node);
	if (!name)
		name = buf = basprintf("tlv%u", id++);

	dev->device_node = of_new_node(of_new_node(NULL, NULL), name);
	dev->device_node->dev = dev;
	dev_set_name(dev, name);
	register_device(dev);

	free(buf);
	return tlvdev;
}

void tlv_free_device(struct tlv_device *tlvdev)
{
	tlv_of_unregister_fixup(tlvdev);

	devinfo_del(&tlvdev->dev, tlv_devinfo);
	unregister_device(&tlvdev->dev);

	free(tlvdev->dev.platform_data);
	of_delete_node(tlvdev->dev.device_node);

	free(tlvdev);
}

static int tlv_bus_match(struct device *dev, const struct driver *drv)
{
	const struct tlv_decoder *decoder = to_tlv_decoder(drv);
	struct tlv_device *tlvdev = to_tlv_device(dev);

	return decoder->magic == tlvdev->magic;
}

static int tlv_bus_probe(struct device *dev)
{
	return dev->driver->probe(dev);
}

static void tlv_bus_remove(struct device *dev)
{
	if (dev->driver->remove)
		dev->driver->remove(dev);
}

struct bus_type tlv_bus = {
	.name = "barebox-tlv",
	.match = tlv_bus_match,
	.probe = tlv_bus_probe,
	.remove = tlv_bus_remove,
};

struct tlv_device *tlv_ensure_probed_by_alias(const char *alias)
{
	struct device_node *np;
	struct device *dev;

	np = of_find_node_by_alias(NULL, alias);
	if (!np)
		return ERR_PTR(-EINVAL);

	of_partition_ensure_probed(np);

	bus_for_each_device(&tlv_bus, dev) {
		if (dev->parent && dev->parent->device_node == np)
			return to_tlv_device(dev);
	}

	return ERR_PTR(-EPROBE_DEFER);
}

static void tlv_bus_info(struct device *dev)
{
	struct driver *drv;

	puts("Registered Magic:\n");
	bus_for_each_driver(&tlv_bus, drv) {
		struct tlv_decoder *tlvdrv = to_tlv_decoder(drv);

		printf("  %08x (%s)\n", tlvdrv->magic, tlvdrv->driver.name);
	}
}

static int tlv_bus_register(void)
{
	int ret;

	ret = bus_register(&tlv_bus);
	if (ret)
		return ret;

	devinfo_add(tlv_bus.dev, tlv_bus_info);

	return 0;
}
postcore_initcall(tlv_bus_register);

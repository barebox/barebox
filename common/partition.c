
#include <common.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <partition.h>
#include <xfuncs.h>

struct device_d *dev_add_partition(struct device_d *dev, unsigned long offset, size_t size, char *name)
{
	struct partition *part;

	if (offset + size > dev->size)
		return NULL;

	part = xzalloc(sizeof(struct partition));

	strcpy(part->device.name, "partition");
	part->device.map_base = dev->map_base + offset;
	part->device.size = size;
	part->device.type_data = part;
	get_free_deviceid(part->device.id, name);

	part->offset = offset;
	part->physdev = dev;

	register_device(&part->device);

	if (part->device.driver)
		return &part->device;

	free(part);
	return 0;
}

static int part_erase(struct device_d *dev, size_t count, unsigned long offset)
{
	struct partition *part = dev->type_data;

	if (part->physdev->driver->erase)
		return part->physdev->driver->erase(part->physdev, count, offset + part->offset);

	return -1;
}

static ssize_t part_read(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
	struct partition *part = dev->type_data;

	return dev_read(part->physdev, buf, count, offset + part->offset, flags);
}

static ssize_t part_write(struct device_d *dev, const void *buf, size_t count, unsigned long offset, ulong flags)
{
	struct partition *part = dev->type_data;

	if (part->readonly)
		return -EROFS;
	else
		return dev_write(part->physdev, buf, count, offset + part->offset, flags);
}

static int part_probe(struct device_d *dev)
{
	struct partition *part = dev->type_data;

	printf("registering partition %s on device %s (size=0x%08x, name=%s)\n",
			dev->id, part->physdev->id, dev->size, part->name);
	return 0;
}

static int part_remove(struct device_d *dev)
{
	return 0;
}

struct driver_d part_driver = {
	.name  	= "partition",
	.probe 	= part_probe,
	.remove = part_remove,
	.read  	= part_read,
	.write 	= part_write,
	.erase 	= part_erase,
};

static int partition_init(void)
{
	return register_driver(&part_driver);
}

device_initcall(partition_init);

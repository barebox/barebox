
#include <common.h>
#include <command.h>
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

static void dev_del_partitions(struct device_d *physdev)
{
        struct device_d *dev;
        char buf[MAX_DRIVER_NAME];
        int i = 0;

        /* This is lame. Devices should to able to have children */
        while (1) {
                sprintf(buf, "%s.%d", physdev->id, i);
                dev = device_from_spec_str(buf, NULL);
                if (dev) {
			struct partition *part = dev->type_data;
                        unregister_device(dev);
			free(part);
		}
                else
                        break;
                i++;
        }
}

int mtd_part_do_parse_one (struct partition *part, const char *str, char **endp)
{
        ulong size;
        char *end;
        char buf[MAX_DRIVER_NAME];

	memset(buf, 0, MAX_DRIVER_NAME);

        if (*str == '-') {
                size = part->physdev->size - part->offset;
		end = (char *)str + 1;
        } else {
		size = strtoul_suffix(str, &end, 0);
	}

	if (size + part->offset > part->physdev->size) {
		printf("partition end is beyond device\n");
		return -EINVAL;
	}

        str = end;

        if (*str == '(') {
		str++;
                end = strchr(str, ')');
                if (!end) {
                        printf("could not find matching ')'\n");
                        return -EINVAL;
                }
                if (end - str >= MAX_DRIVER_NAME) {
                        printf("device name too long\n");
                        return -EINVAL;
                }

                memcpy(part->name, str, end - str);
                end++;
        }

        str = end;

        if (*str == 'r' && *(str + 1) == 'o') {
                part->readonly = 1;
                end = (char *)(str + 2);
        }

        if (endp)
                *endp = end;

        strcpy(part->device.name, "partition");
        part->device.size = size;

        return 0;
}

int do_addpart ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct partition *part;
        struct device_d *dev;
        char *endp;
        int num = 0;
        unsigned long offset;

        if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], &endp);
        if (!dev) {
                printf("no such device: %s\n",argv[1]);
                return 1;
        }

	dev_del_partitions(dev);

        offset = 0;

        while (1) {
                part = xzalloc(sizeof(struct partition));

                part->offset = offset;
                part->physdev = dev;
                part->num = num;
                part->device.map_base = dev->map_base + offset;

                if(mtd_part_do_parse_one(part, endp, &endp)) {
			dev_del_partitions(dev);
                        free(part);
                        return 1;
                }

                offset += part->device.size;

                part->device.type_data = part;

                sprintf(part->device.id, "%s.%d", dev->id, num);
                register_device(&part->device);
                num++;

                if(!*endp)
                        break;
                if(*endp != ',') {
                        printf("parse error\n");
                        return 1;
                }
                endp++;
        }

        return 0;
}

static __maybe_unused char cmd_addpart_help[] =
"Usage: addpart <partition description>\n"
"addpart adds a partition description to a device. The partition description\n"
"has the form\n"
"dev:size1(name1)[ro],size2(name2)[ro],...\n"
"<dev> is the device name under /dev. Size can be given in decimal or if\n"
"prefixed with 0x in hex. Sizes can have an optional suffix K,M,G. The size\n"
"of the last partition can be specified as '-' for the remaining space of the\n"
"device.\n"
"This format is the same as used in the Linux kernel for cmdline mtd partitions.\n"
"Note That this command has to be reworked and will probably change it's API.";

U_BOOT_CMD_START(addpart)
	.maxargs	= 2,
	.cmd		= do_addpart,
	.usage		= "add a partition table to a device",
	U_BOOT_CMD_HELP(cmd_addpart_help)
U_BOOT_CMD_END

int do_delpart ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev;

        if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], NULL);
        if (!dev) {
                printf("no such device: %s\n",argv[1]);
                return 1;
        }

	dev_del_partitions(dev);

        return 0;
}

static __maybe_unused char cmd_delpart_help[] =
"Usage: delpart <dev>\n"
"Delete partitions previously added to a device with addpart.\n"
"Note: You have to specify the device as 'devid', _not_ as '/dev/devid'. This\n"
"will likely change soon.\n";

U_BOOT_CMD_START(delpart)
	.maxargs	= 2,
	.cmd		= do_delpart,
	.usage		= "delete a partition table from a device",
	U_BOOT_CMD_HELP(cmd_delpart_help)
U_BOOT_CMD_END

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

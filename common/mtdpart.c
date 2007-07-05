
#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>

struct partition {
        int num;

        unsigned long offset;

        struct device_d *parent;
        struct device_d device;

        char name[16];
};

#if 0
static void dev_del_partitions(struct device_d *dev)
{
        struct partition *part = dev->part;
        struct partition *oldpart;

        while (part) {
                unregister_device(&part->device);
                oldpart = part;
                part = part->next;
                free(oldpart);
        }

        dev->part = NULL;
}
#endif

int mtd_part_do_parse_one (struct partition *part, const char *str, char **endp)
{
        ulong size;
        char *end;
        char buf[MAX_DRIVER_NAME];
        int ro = 0;

	memset(buf, 0, MAX_DRIVER_NAME);

        if (*str == '-') {
                /* FIXME: The rest of the device */
                return -1;
        }

        size = strtoul_suffix(str, &end, 0);

        str = end;

        if (*str == '(') {
		str++;
                end = strchr(str, ')');
                if (!end) {
                        printf("could not find matching ')'\n");
                        return -1;
                }
                if (end - str >= MAX_DRIVER_NAME) {
                        printf("device name too long\n");
                        return -1;
                }

                memcpy(part->name, str, end - str);
                end++;
        }

        str = end;

        if (*str == 'r' && *(str + 1) == 'o') {
                ro = 1;
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

//        dev_del_partitions(dev);

        offset = 0;

        while (1) {
                part = malloc(sizeof(struct partition));
                if(!part) {
                        printf("-ENOMEM\n");
                        return 1;
                }

                part->offset = offset;
                part->parent = dev;
                part->num = num;
                part->device.map_base = dev->map_base + offset;

                if(mtd_part_do_parse_one(part, endp, &endp)) {
//                        dev_del_partitions(dev);
                        free(part);
                        return 1;
                }

                offset += part->device.size;

                part->device.platform_data = part;

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

int do_delpart ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev, *p;
        int i = 0;
        char buf[MAX_DRIVER_NAME];

        if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], NULL);
        if (!dev) {
                printf("no such device: %s\n",argv[1]);
                return 1;
        }

        /* This is lame. Devices should to able to have children */
        while(1) {
                sprintf(buf, "%s.%d", dev->id, i);
                p = device_from_spec_str(buf, NULL);
                if (p)
                        unregister_device(p);
                else
                        break;
                i++;
        }

        return 0;
}

U_BOOT_CMD(
	addpart,     2,     0,      do_addpart,
	"addpart     - add a partition table to a device\n",
	""
);

U_BOOT_CMD(
	delpart,     2,     0,      do_delpart,
	"delpart     - delete a partition table from a device\n",
	""
);

int part_probe (struct device_d *dev)
{
        struct partition *part = dev->platform_data;

        printf("registering partition %s on device %s (size=0x%08x, name=%s)\n",
                        dev->id, part->parent->id, dev->size, part->name);
        return 0;
}

int part_erase(struct device_d *dev, size_t count, unsigned long offset)
{
        struct partition *part = dev->platform_data;

        if (part->parent->driver->erase)
                return part->parent->driver->erase(part->parent, count, offset + part->offset);

        return -1;
}

ssize_t part_read(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
        struct partition *part = dev->platform_data;

        return read(part->parent, buf, count, offset + part->offset, flags);
}

ssize_t part_write(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
        struct partition *part = dev->platform_data;

        return write(part->parent, buf, count, offset + part->offset, flags);
}

static struct driver_d part_driver = {
        .name  = "partition",
        .probe = part_probe,
        .read  = part_read,
        .write = part_write,
        .erase = part_erase,
};

static int partition_init(void)
{
        return register_driver(&part_driver);
}

device_initcall(partition_init);

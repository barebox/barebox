
#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <partition.h>

static int dev_add_partition(struct device_d *dev, struct partition *new_part)
{
        struct partition *part = dev->part;

        new_part->next = NULL;

        if (!part) {
                dev->part = new_part;
                return 0;
        }

        while (part->next)
                part = part->next;

        part->next = new_part;
        return 0;
}

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

                memcpy(buf, str, end - str);
                end++;
        }

        str = end;

        if (*str == 'r' && *(str + 1) == 'o') {
                ro = 1;
                end = (char *)(str + 2);
        }

        if (endp)
                *endp = end;

        sprintf(part->device.name, "%s.%d", part->parent->name, part->num);
        part->device.size = size;

//        printf("part: name=%10s size=0x%08x %s\n", part->device.name, size, ro ? "ro":"");
        return 0;
}

int do_addpart ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct partition *part;
        struct device_d *dev;
        char *endp;
        int num = 0;
        unsigned long base;

        if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        dev = device_from_spec_str(argv[1], &endp);
        if (!dev) {
                printf("no such device: %s\n",argv[1]);
                return 1;
        }

        dev_del_partitions(dev);

        base = dev->map_base;

        while (1) {
                part = malloc(sizeof(struct partition));
                if(!part) {
                        printf("-ENOMEM\n");
                        return 1;
                }

                part->parent = dev;
                part->num = num++;
                part->device.map_base = base;
                part->device.map_flags = dev->map_flags;

                if(mtd_part_do_parse_one(part, endp, &endp)) {
                        dev_del_partitions(dev);
                        free(part);
                        return 1;
                }

                base += part->device.size;
                dev_add_partition(dev, part);

                /* FIXME: Frameork should handle this */
                part->device.driver = dev->driver;

                register_device(&part->device);

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
        struct device_d *dev;

        dev = device_from_spec_str(argv[1], NULL);
        if (!dev) {
                printf("no such device: %s\n",argv[1]);
                return 1;
        }

        dev_del_partitions(dev);

        return 0;
}

U_BOOT_CMD(
	addpart,     2,     0,      do_addpart,
	"addpart     - add a partition table to a device\n",
	""
);

U_BOOT_CMD(
	delpart,     2,     0,      do_delpart,
	"addpart     - delete a partition table from a device\n",
	""
);


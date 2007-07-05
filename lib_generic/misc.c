
#include <common.h>
#include <command.h>
#include <driver.h>
#include <linux/ctype.h>

int cmd_get_data_size(char* arg, int default_size)
{
	/* Check for a size specification .b, .w or .l.
	 */
	int len = strlen(arg);
	if (len > 2 && arg[len-2] == '.') {
		switch(arg[len-1]) {
		case 'b':
			return 1;
		case 'w':
			return 2;
		case 'l':
			return 4;
		case 's':
			return -2;
		default:
			return -1;
		}
	}
	return default_size;
}

static struct device_d *first_device = NULL;
static struct driver_d *first_driver = NULL;

int do_devinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev = first_device;
        struct driver_d *drv = first_driver;

        printf("devices:\n");

        while(dev) {
                printf("%10s: base=0x%08x size=0x%08x\n",dev->name, dev->map_base, dev->size);
                dev = dev->next;
        }

        printf("drivers:\n");
        while(drv) {
                printf("%10s\n",drv->name);
                drv = drv->next;
        }
        return 0;
}

U_BOOT_CMD(
	devinfo,     1,     0,      do_devinfo,
	"devinfo     - display info about devices and drivers\n",
	""
);

static struct device_d *match_driver(struct driver_d *d, struct device_d *first_dev)
{
        struct device_d *device;

        if (first_dev)
                device = first_dev->next;
        else
                device = first_device;

        while (device) {
                if (!strcmp(device->name, d->name))
                        return device;
                device = device->next;
        }
        return 0;
}

static struct driver_d *match_device(struct device_d *d)
{
        struct driver_d *driver = first_driver;

        while (driver) {
                if (!strcmp(driver->name, d->name))
                        return driver;
                driver = driver->next;
        }
        return 0;
}

int register_device(struct device_d *new_device)
{
	struct driver_d *drv;
        struct device_d *dev;

	dev = first_device;

        printf("register_device: %s\n",new_device->name);

	if(!dev) {
		first_device = new_device;
	} else {
		while(dev->next)
			dev = dev->next;
	}

	dev->next = new_device;
	new_device->next = 0;

        drv = match_device(new_device);
        if (drv) {
                if (!drv->probe(new_device)) {
                        printf("setting driver to 0x%08x\n",drv);
                        new_device->driver = drv;
                }
        }

	return 0;
}

void unregister_device(struct device_d *old_dev)
{
        struct device_d *dev;
        printf("unregister_device: %s\n",old_dev->name);

	dev = first_device;

        while (dev) {
                if (!strcmp(dev->next->name, old_dev->name)) {
                        dev->next = old_dev->next;
                        return;
                }
                dev = dev->next;
        }
}

struct driver_d *get_driver_by_name(char *name)
{
	struct driver_d *d;

	d = first_driver;

	while(d) {
		if(!strcmp(name, d->name))
			break;
		d = d->next;
	}

	return d;
}

struct device_d *get_device_by_name(char *name)
{
	struct device_d *d;

	d = first_device;

	while(d) {
		if(!strcmp(name, d->name))
			break;
		d = d->next;
	}

	return d;
}

int register_driver(struct driver_d *new_driver)
{
	struct driver_d *drv;
        struct device_d *dev = NULL;

	drv = first_driver;

        printf("register_driver: %s\n",new_driver->name);

	if(!drv) {
		first_driver = new_driver;
	} else {
		while(drv->next)
			drv = drv->next;
	}

	drv->next = new_driver;
	new_driver->next = 0;

        while((dev = match_driver(new_driver, dev))) {
                if(!new_driver->probe(dev))
                        dev->driver = new_driver;
        }

	return 0;
}

static char devicename_from_spec_str_buf[MAX_DRIVER_NAME];

char *devicename_from_spec_str(const char *str, char **endp)
{
        char *buf = devicename_from_spec_str_buf;
        const char *end;
	int i = 0;

        if (isdigit(*str)) {
                /* No device name given, use default driver mem */
                sprintf(buf, "mem");
                end = str;
        } else {
                /* OK, we have a device name, parse it */
	        while (*str) {
		        if (*str == ':') {
                                str++;
			        buf[i] = 0;
			        break;
        		}

	        	buf[i++] = *str++;
		        buf[i] = 0;
		        if (i == MAX_DRIVER_NAME)
			        return NULL;
        	}
                end = str;
        }

        if (endp)
                *endp = (char *)end;

        return buf;
}

/* Get a device struct from the beginning of the string. Default to mem if no
 * device is given, return NULL if a unknown device is given.
 * If endp is not NULL, this function stores a pointer to the first character
 * after the device name in *endp.
 */
struct device_d *device_from_spec_str(const char *str, char **endp)
{
        char *name;
        name = devicename_from_spec_str(str, endp);
        return get_device_by_name(name);
}

unsigned long strtoul_suffix(const char *str, char **endp, int base)
{
        unsigned long val;
        char *end;

        val = simple_strtoul(str, &end, base);

        switch (*end) {
        case 'G':
                val *= 1024;
        case 'M':
		val *= 1024;
        case 'k':
        case 'K':
                val *= 1024;
                end++;
        }

        if (endp)
                *endp = (char *)end;

        return val;
}

/*
 * Parse a string representing a memory area and fill in a struct memarea_info.
 * FIXME: Check for end of devices
 */
int spec_str_to_info(const char *str, struct memarea_info *info)
{
	char *endp;

	memset(info, 0, sizeof(struct memarea_info));

        info->device = device_from_spec_str(str, &endp);
        if (!info->device) {
                printf("unknown device: %s\n", devicename_from_spec_str(str, NULL));
                return 1;
        }

        str = endp;

        if (!*str) {
                /* no area specification given, use whole device */
                info->size = info->device->size;
                info->end  = info->size - 1;
                goto out;
        }

        if (*str == '+') {
                /* no beginning given but size so start is 0 */
                info->size = strtoul_suffix(str + 1, &endp, 0);
                info->end  = info->size - 1;
                info->flags = MEMAREA_SIZE_SPECIFIED;
                goto out;
        }

        if (*str == '-') {
                /* no beginning given but end, so start is 0 */
                info->end = strtoul_suffix(str + 1, &endp, 0);
                info->size  = info->end + 1;
                info->flags = MEMAREA_SIZE_SPECIFIED;
                goto out;
        }

        str = endp;
        info->start = strtoul_suffix(str, &endp, 0);
        str = endp;

        if (info->start >= info->device->size) {
                printf("Start address is beyond device\n");
                return -1;
        }

        if (!*str) {
                /* beginning given, but no size, pad to the and of the device */
                info->end = info->device->size - 1;
                info->size = info->end - info->start + 1;
                goto out;
        }

	if (*str == '-') {
                /* beginning and end given */
		info->end = strtoul_suffix(str + 1, NULL, 0);
		info->size = info->end - info->start + 1;
                if (info->end < info->start) {
                        printf("end < start\n");
                        return -1;
                }
                info->flags = MEMAREA_SIZE_SPECIFIED;
                goto out;
	}

	if (*str == '+') {
                /* beginning and size given */
		info->size = strtoul_suffix(str + 1, NULL, 0);
                info->end = info->start + info->size - 1;
                info->flags = MEMAREA_SIZE_SPECIFIED;
                goto out;
	}

out:
        /* check for device boundaries */
        if (info->end > info->device->size) {
                info->end = info->device->size - 1;
                info->size = info->end - info->start + 1;
        }
#if 1
	printf("start: 0x%08x\n",info->start);
	printf("size: 0x%08x\n",info->size);
	printf("end: 0x%08x\n",info->end);
#endif
	return 0;
}



#include <common.h>
#include <command.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <fs.h>

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

struct device_d *get_device_by_id(const char *_id)
{
	struct device_d *d;
	char *id, *colon;

	id = strdup(_id);
	if ((colon = strchr(id, ':')))
		*colon = 0;

	d = first_device;

	while(d) {
		if(!strcmp(id, d->id))
			break;
		d = d->next;
	}

	free(id);
	return d;
}

static int match(struct driver_d *drv, struct device_d *dev)
{
        if (strcmp(dev->name, drv->name))
                return -1;
        if (dev->type != drv->type)
                return -1;
        if(drv->probe(dev))
                return -1;

        dev->driver = drv;

        return 0;
}

int register_device(struct device_d *new_device)
{
	struct driver_d *drv;
        struct device_d *dev;

	dev = first_device;

	if(*new_device->id && get_device_by_id(new_device->id)) {
		printf("device %s already exists\n", new_device->id);
		return -EINVAL;
	}
//        printf("register_device: %s\n",new_device->name);

	if(!dev) {
		first_device = new_device;
	} else {
		while(dev->next)
			dev = dev->next;
	}

	dev->next = new_device;
	new_device->next = 0;

        drv = first_driver;

        while(drv) {
                if (!match(drv, new_device))
                        break;
                drv = drv->next;
        }

	return 0;
}

void unregister_device(struct device_d *old_dev)
{
        struct device_d *dev;
//        printf("unregister_device: %s\n",old_dev->name);

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

static void noinfo(struct device_d *dev)
{
        printf("no info available for %s\n", dev->id);
}

static void noshortinfo(struct device_d *dev)
{
}

int register_driver(struct driver_d *new_driver)
{
	struct driver_d *drv;
        struct device_d *dev = NULL;

	drv = first_driver;

//        printf("register_driver: %s\n",new_driver->name);

	if(!drv) {
		first_driver = new_driver;
	} else {
		while(drv->next)
			drv = drv->next;
	}

	drv->next = new_driver;
	new_driver->next = 0;

        if (!new_driver->info)
                new_driver->info = noinfo;
        if (!new_driver->shortinfo)
                new_driver->shortinfo = noshortinfo;

        dev = first_device;
        while (dev) {
                match(new_driver, dev);
                dev = dev->next;
        }

	return 0;
}

static char devicename_from_spec_str_buf[PATH_MAX];

char *deviceid_from_spec_str(const char *str, char **endp)
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
        name = deviceid_from_spec_str(str, endp);
        return get_device_by_id(name);
}

/* Get devices from their type.
 * If last is NULL the first device of this type is given.
 * If last is not NULL, the next device of this type starting
 * from last is given.
 */
struct device_d *get_device_by_type(ulong type, struct device_d *last)
{
	struct device_d *dev;

	if (!last)
		dev = first_device;
	else
		dev = last->next;

	while (dev) {
		if (dev->type == type)
			return dev;
		dev = dev->next;
	}

	return NULL;
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

int parse_area_spec(const char *str, ulong *start, ulong *size)
{
	char *endp;

	if (*str == '+') {
		/* no beginning given but size so start is 0 */
		*start = 0;
		*size = strtoul_suffix(str + 1, &endp, 0);
		return 0;
	}

	if (*str == '-') {
		/* no beginning given but end, so start is 0 */
		*start = 0;
		*size = strtoul_suffix(str + 1, &endp, 0) + 1;
		return 0;
	}

	*start = strtoul_suffix(str, &endp, 0);

	str = endp;

	if (!*str) {
		/* beginning given, but no size, assume maximum size */
		*size = ~0;
		return 0;
	}

	if (*str == '-') {
                /* beginning and end given */
		*size = strtoul_suffix(str + 1, NULL, 0) + 1;
		return 0;
	}

	if (*str == '+') {
                /* beginning and size given */
		*size = strtoul_suffix(str + 1, NULL, 0);
		return 0;
	}

	return -1;
}
int spec_str_to_info(const char *str, struct memarea_info *info)
{
	char *endp;

	info->device = device_from_spec_str(str, &endp);
	if (!info->device) {
		printf("unknown device: %s\n", deviceid_from_spec_str(str, NULL));
		return -ENODEV;
	}

	return parse_area_spec(endp, &info->start, &info->size);
}

ssize_t dev_read(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
        if (dev->driver->read)
                return dev->driver->read(dev, buf, count, offset, flags);
	errno = -ENOSYS;
        return -ENOSYS;
}

ssize_t dev_write(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
        if (dev->driver->write)
                return dev->driver->write(dev, buf, count, offset, flags);
	errno = -ENOSYS;
        return -ENOSYS;
}

ssize_t dev_erase(struct device_d *dev, size_t count, unsigned long offset)
{
        if (dev->driver->erase)
                return dev->driver->erase(dev, count, offset);
	errno = -ENOSYS;
        return -ENOSYS;
}

int dummy_probe(struct device_d *dev)
{
        return 0;
}

struct param_d *get_param_by_name(struct device_d *dev, char *name)
{
        struct param_d *param = dev->param;

        while (param) {
                if (!strcmp(param->name, name))
                        return param;
                param = param->next;
        }

        return NULL;
}

void print_param(struct param_d *param) {
	switch (param->type) {
	case PARAM_TYPE_STRING:
		printf("%s", param->value.val_str);
		break;
	case PARAM_TYPE_ULONG:
		printf("%ld", param->value.val_ulong);
		break;
	case PARAM_TYPE_IPADDR:
		print_IPaddr(param->value.val_ip);
		break;
	}
}

struct param_d* dev_get_param(struct device_d *dev, char *name)
{
        struct param_d *param = get_param_by_name(dev, name);

	if (!param) {
		errno = -EINVAL;
		return NULL;
	}

        if (param->get)
                return param->get(dev, param);

        return param;
}

IPaddr_t dev_get_param_ip(struct device_d *dev, char *name)
{
	struct param_d *param = dev_get_param(dev, name);

	if (!param || param->type != PARAM_TYPE_IPADDR) {
		errno = -EINVAL;
		return 0;
	}

	return param->value.val_ip;
}

int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip)
{
	value_t val;

	val.val_ip = ip;

	return dev_set_param(dev, name, val);
}

int dev_set_param(struct device_d *dev, char *name, value_t val)
{
        struct param_d *param = get_param_by_name(dev, name);

	if (!param) {
		errno = -EINVAL;
		return -EINVAL;
	}

	if (param->flags & PARAM_FLAG_RO) {
		errno = -EACCES;
		return -EACCES;
	}

        if (param->set)
                return param->set(dev, param, val);

	if (param->type == PARAM_TYPE_STRING) {
		if (param->value.val_str)
			free(param->value.val_str);
		param->value.val_str = strdup(val.val_str);
		return 0;
	}

        param->value = val;
	return 0;
}

int dev_add_parameter(struct device_d *dev, struct param_d *newparam)
{
        struct param_d *param = dev->param;

        newparam->next = 0;

        if (param) {
                while (param->next)
                        param = param->next;
                        param->next = newparam;
        } else {
                dev->param = newparam;
        }

        return 0;
}

int do_devinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
        struct device_d *dev = first_device;
        struct driver_d *drv = first_driver;
        struct param_d *param;

        if (argc == 1) {
                printf("devices:\n");

                while(dev) {
                        printf("%10s: base=0x%08x size=0x%08x (driver %s)\n",
                                        dev->id, dev->map_base, dev->size, dev->driver ? dev->driver->name : "none");
                        dev = dev->next;
                }

                printf("drivers:\n");
                while(drv) {
                        printf("%10s\n",drv->name);
                        drv = drv->next;
                }
        } else {
                struct device_d *dev = get_device_by_id(argv[1]);

                if (!dev) {
                        printf("no such device: %s\n",argv[1]);
                        return -1;
                }

                if (dev->driver)
                        dev->driver->info(dev);

                param = dev->param;

                printf("%s\n", param ? "Parameters:" : "no parameters available");

                while (param) {
			printf("%16s = ", param->name);
			print_param(param);
			printf("\n");
                        param = param->next;
                }

        }

        return 0;
}

U_BOOT_CMD(
	devinfo,     2,     0,      do_devinfo,
	"devinfo     - display info about devices and drivers\n",
	""
);


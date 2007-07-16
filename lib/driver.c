/*
 * driver.c - U-Boot driver model
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <errno.h>
#include <fs.h>

static struct device_d *first_device = NULL;
static struct driver_d *first_driver = NULL;

struct device_d *get_first_device(void)
{
	return first_device;
}

struct device_d *get_device_by_id(const char *_id)
{
	struct device_d *d;
	char *id, *colon;

	/* FIXME: is this still needed? */
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

int get_free_deviceid(char *id, char *id_template)
{
	int i = 0;

	while (1) {
		sprintf(id, "%s%d", id_template, i);
		if (!get_device_by_id(id))
			return 0;
		i++;
	};

	return -1;
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
	debug ("register_device: %s\n",new_device->name);

	if(!dev) {
		first_device = new_device;
		dev = first_device;
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

	debug("unregister_device: %s:%s\n",old_dev->name, old_dev->id);

	dev = first_device;

        while (dev) {
                if (dev->next == old_dev) {
			if (old_dev->driver)
				old_dev->driver->remove(old_dev);
                        dev->next = old_dev->next;
                        return;
                }
                dev = dev->next;
        }
}

struct driver_d *get_driver_by_name(const char *name)
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

	debug("register_driver: %s\n",new_driver->name);

	if(!drv) {
		first_driver = new_driver;
		drv = first_driver;
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

ssize_t dev_read(struct device_d *dev, void *buf, size_t count, unsigned long offset, ulong flags)
{
        if (dev->driver->read)
                return dev->driver->read(dev, buf, count, offset, flags);
	errno = -ENOSYS;
        return -ENOSYS;
}

ssize_t dev_write(struct device_d *dev, const void *buf, size_t count, unsigned long offset, ulong flags)
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

int dev_protect(struct device_d *dev, size_t count, unsigned long offset, int prot)
{
        if (dev->driver->protect)
                return dev->driver->protect(dev, count, offset, prot);
	errno = -ENOSYS;
        return -ENOSYS;
}

ssize_t dev_memmap(struct device_d *dev, void **map, int flags)
{
        if (dev->driver->memmap)
                return dev->driver->memmap(dev, map, flags);
	errno = -ENOSYS;
        return -ENOSYS;
}

int generic_memmap_ro(struct device_d *dev, void **map, int flags)
{
	if (flags & PROT_WRITE)
		return -EACCES;
	*map = (void *)dev->map_base;
	return 0;
}

int generic_memmap_rw(struct device_d *dev, void **map, int flags)
{
	*map = (void *)dev->map_base;
	return 0;
}

int dummy_probe(struct device_d *dev)
{
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
                                        dev->id, dev->map_base, dev->size,
					dev->driver ? 
						dev->driver->name : "none");
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

                printf("%s\n", param ?
				"Parameters:" : "no parameters available");

                while (param) {
			printf("%16s = %s\n", param->name, param->value);
                        param = param->next;
                }

        }

        return 0;
}

static __maybe_unused char cmd_devinfo_help[] =
"Usage: devinfo [DEVICE]\n"
"If called without arguments devinfo shows a summary about known devices and\n"
"drivers. If called with a device id as argument devinfo shows more detailed\n"
"informations about this device and its parameters.\n";

U_BOOT_CMD_START(devinfo)
	.maxargs	= 2,
	.cmd		= do_devinfo,
	.usage		= "display info about devices and drivers",
	U_BOOT_CMD_HELP(cmd_devinfo_help)
U_BOOT_CMD_END

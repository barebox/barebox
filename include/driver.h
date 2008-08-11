/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <list.h>

#define MAX_DRIVER_NAME 16

#define DEVICE_TYPE_UNKNOWN     0
#define DEVICE_TYPE_ETHER       1
#define DEVICE_TYPE_CONSOLE     2
#define DEVICE_TYPE_DRAM	3
#define DEVICE_TYPE_BLOCK	4
#define DEVICE_TYPE_FS		5
#define DEVICE_TYPE_MIIPHY	6
#define DEVICE_TYPE_NAND	7
#define DEVICE_TYPE_NAND_BB	8
#define MAX_DEVICE_TYPE         8

#include <param.h>

/**
 * @file
 * @brief Main description of the device/driver model
 */

/** @page driver_model Main description of the device/driver model
 *
 * We follow a rather simplistic driver model here. There is a
 * @code struct device_d @endcode
 * which describes a particular device present in the system.
 *
 * On the other side a
 * @code struct driver_d @endcode
 * represents a driver present in the system.
 *
 * Both structs find together via the members 'type' (int) and 'name' (char *).
 * If both members match, the driver's probe function is called with the
 * struct device_d as argument.
 *
 * People familiar with the Linux platform bus will recognize this behaviour
 * and in fact many things were stolen from there. Some selected members of the
 * structs will be described in this document.
 */

/*@{*/	/* do not delete, doxygen relevant */

struct filep;

/** @brief Describes a particular device present in the system */
struct device_d {
	/*! This member (and 'type' described below) is used to match with a
	 * driver. This is a descriptive name and could be MPC5XXX_ether or
	 * imx_serial. */
	char name[MAX_DRIVER_NAME];
	/*! The id is used to uniquely identify a device in the system. The id
	 * will show up under /dev/ as the device's name. Usually this is
	 * something like eth0 or nor0. */
	char id[MAX_DRIVER_NAME];

	/*! FIXME */
	unsigned long size;

	/*! For devices which are directly mapped into memory, i.e. NOR
	 * Flash or SDRAM. */
	unsigned long map_base;

	void *platform_data; /*! board specific information about this device */

	/*! Devices of a particular class normaly need to store more
	 * information than struct device holds. This entry holds a pointer to
	 * the type specific struct, so a a device of type DEVICE_TYPE_ETHER
	 * sets this to a struct eth_device. */
	void *priv;
	void *type_data;     /*! In case this device is a specific device, this pointer
			      * points to the type specific device, i.e. eth_device
			      */

	struct driver_d *driver; /*! The driver for this device */

	struct list_head list;     /* The list of all devices */
	struct list_head children; /* our children            */
	struct list_head sibling;

	struct device_d *parent;   /* our parent, NULL if not present */

	/*! This describes the type (or class) of this device. Have a look at
	 * include/driver.h to see a list of known device types. Currently this
	 * includes DEVICE_TYPE_ETHER, DEVICE_TYPE_CONSOLE and others. */
	unsigned long type;

	/*! The parameters for this device. This is used to carry information
	 * of board specific data from the board code to the device driver. */
	struct param_d *param;
};

/** @brief Describes a driver present in the system */
struct driver_d {
	/*! The name of this driver. Used to match to
	 * the corresponding device. */
	char name[MAX_DRIVER_NAME];

	struct list_head list;

	/*! Called if an instance of a device is found */
	int     (*probe) (struct device_d *);

	/*! Called if an instance of a device is gone. */
	int     (*remove)(struct device_d *);

	/*! Called in response of reading from this device. Required */
	ssize_t (*read)  (struct device_d*, void* buf, size_t count, ulong offset, ulong flags);

	/*! Called in response of write to this device. Required */
	ssize_t (*write) (struct device_d*, const void* buf, size_t count, ulong offset, ulong flags);

	int (*ioctl) (struct device_d*, int, void *);

	off_t (*lseek) (struct device_d*, off_t);
	int (*open) (struct device_d*, struct filep*);
	int (*close) (struct device_d*, struct filep*);

	int     (*erase) (struct device_d*, size_t count, unsigned long offset);
	int     (*protect)(struct device_d*, size_t count, unsigned long offset, int prot);
	int     (*memmap)(struct device_d*, void **map, int flags);

	void    (*info) (struct device_d *);
	void    (*shortinfo) (struct device_d *);

	unsigned long type;

	/*! This is somewhat redundant with the type data in struct device.
	 * Currently the filesystem implementation uses this field while
	 * ethernet drivers use the same field in struct device. Probably
	 * one of both should be removed. */
	void *type_data;
};

/*@}*/	/* do not delete, doxygen relevant */

#define RW_SIZE(x)      (x)
#define RW_SIZE_MASK    0x7

/* Register devices and drivers.
 */
int register_driver(struct driver_d *);
int register_device(struct device_d *);

/* Unregister a device. This function can fail, e.g. when the device
 * has children.
 */
int unregister_device(struct device_d *);

/* Organize devices in a tree. These functions do _not_ register or
 * unregister a device. Only registered devices are allowed here.
 */
int dev_add_child(struct device_d *dev, struct device_d *child);

/* Iterate over a devices children
 */
#define device_for_each_child(dev, child) \
	list_for_each_entry(child, &dev->children, sibling)

/* Iterate over a devices children - Safe against removal version
 */
#define device_for_each_child_safe(dev, tmpdev, child) \
	list_for_each_entry_safe(child, tmpdev, &dev->children, sibling)

/* Iterate through the devices of a given type. if last is NULL, the
 * first device of this type is returned. Put this pointer in as
 * 'last' to get the next device. This functions returns NULL if no
 * more devices are found.
 */
struct device_d *get_device_by_type(ulong type, struct device_d *last);
struct device_d *get_device_by_id(const char *id);
struct device_d *get_device_by_path(const char *path);

/* Find a free device id from the given template. This is archieved by
 * appending a number to the template. Dynamically created devices should
 * use this function rather than filling the id field themselves.
 */
int get_free_deviceid(char *id, const char *id_template);

char *deviceid_from_spec_str(const char *str, char **endp);

/* linear list over all available devices
 */
extern struct list_head device_list;

/* linear list over all available drivers
 */
extern struct list_head driver_list;

/* Iterate over all devices
 */
#define for_each_device(dev) list_for_each_entry(dev, &device_list, list)

/* Iterate over all drivers
 */
#define for_each_driver(drv) list_for_each_entry(drv, &driver_list, list)

/* Find a driver with the given name. Currently the filesystem implementation
 * uses this to get the driver from the name the user specifies with the
 * mount command
 */
struct driver_d *get_driver_by_name(const char *name);

ssize_t dev_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t dev_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags);
int     dev_open(struct device_d *dev, struct filep *);
int     dev_close(struct device_d *dev, struct filep *);
int     dev_ioctl(struct device_d *dev, int, void *);
off_t   dev_lseek(struct device_d *dev, off_t offset);
int     dev_erase(struct device_d *dev, size_t count, unsigned long offset);
int     dev_protect(struct device_d *dev, size_t count, unsigned long offset, int prot);
int     dev_memmap(struct device_d *dev, void **map, int flags);

/* These are used by drivers which work with direct memory accesses */
ssize_t mem_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t mem_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags);

/* Use this if you have nothing to do in your drivers probe function */
int dummy_probe(struct device_d *);

int generic_memmap_ro(struct device_d *dev, void **map, int flags);
int generic_memmap_rw(struct device_d *dev, void **map, int flags);

static inline off_t dev_lseek_default(struct device_d *dev, off_t ofs)
{
	return ofs;
}

static inline int dev_open_default(struct device_d *dev, struct filep *f)
{
	return 0;
}

static inline int dev_close_default(struct device_d *dev, struct filep *f)
{
	return 0;
}

#endif /* DRIVER_H */


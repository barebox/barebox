/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <linux/list.h>
#include <linux/ioport.h>
#include <linux/uuid.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <device.h>
#include <of.h>
#include <init.h>
#include <filetype.h>

#define FORMAT_DRIVER_NAME_ID	"%s%d"

#include <param.h>

struct filep;
struct bus_type;
struct generic_pm_domain;

struct platform_device_id {
	const char *name;
	unsigned long driver_data;
};

/** @brief Describes a driver present in the system */
struct driver {
	/*! The name of this driver. Used to match to
	 * the corresponding device. */
	const char *name;

	struct list_head list;
	struct list_head bus_list; /* our bus            */

	/*! Called if an instance of a device is found */
	int     (*probe) (struct device *);

	/*! Called if an instance of a device is gone. */
	void     (*remove)(struct device *);

	struct bus_type *bus;

	const struct platform_device_id *id_table;
	union {
		const struct of_device_id *of_compatible;
		const struct of_device_id *of_match_table;
	};
};

/*@}*/	/* do not delete, doxygen relevant */

/* Legacy naming for out-of-tree patches. Will be phased out in future. */
#define device_d device
#define driver_d driver

/* dynamically assign the next free id */
#define DEVICE_ID_DYNAMIC	-2
/* do not use an id (only one device available */
#define DEVICE_ID_SINGLE	-1

/* Register devices and drivers.
 */
int register_driver(struct driver *);
void unregister_driver(struct driver *drv);

int register_device(struct device *);

/* manualy probe a device
 * the driver need to be specified
 */
int device_probe(struct device *dev);

/**
 * device_remove - Remove a device from its bus and driver
 *
 * @dev: Device
 *
 * Returns true if there was any bus or driver specific removal
 * code that was executed and false if the function was a no-op.
 */
bool device_remove(struct device *dev);

/* detect devices attached to this device (cards, disks,...) */
int device_detect(struct device *dev);
int device_detect_by_name(const char *devname);
void device_detect_all(void);

/* Unregister a device. This function can fail, e.g. when the device
 * has children.
 */
int unregister_device(struct device *);

static inline void put_device(struct device *dev) {}

void free_device_res(struct device *dev);
void free_device(struct device *dev);

static inline void device_rescan(struct device *dev)
{
	if (dev->rescan)
		dev->rescan(dev);
}

/* Iterate over a devices children
 */
#define device_for_each_child(dev, child) \
	list_for_each_entry(child, &(dev)->children, sibling)

/* Iterate over a devices children - Safe against removal version
 */
#define device_for_each_child_safe(dev, tmpdev, child) \
	list_for_each_entry_safe(child, tmpdev, &(dev)->children, sibling)

/* Iterate through the devices of a given type. if last is NULL, the
 * first device of this type is returned. Put this pointer in as
 * 'last' to get the next device. This functions returns NULL if no
 * more devices are found.
 */
struct device *get_device_by_type(ulong type, struct device *last);
struct device *get_device_by_id(const char *id);
struct device *get_device_by_name(const char *name);

/* Find a device by name and if not found look up by device tree path
 * or alias
 */
struct device *find_device(const char *str);

/* Find a free device id from the given template. This is archieved by
 * appending a number to the template. Dynamically created devices should
 * use this function rather than filling the id field themselves.
 */
int get_free_deviceid(const char *name_template);

char *deviceid_from_spec_str(const char *str, char **endp);

static inline const char *dev_id(const struct device *dev)
{
	if (!dev)
		return NULL;
	return (dev->id != DEVICE_ID_SINGLE) ? dev->unique_name : dev->name;
}

static inline const char *dev_name(const struct device *dev)
{
	if (!dev)
		return NULL;
	return dev_id(dev) ?: dev->name;
}

int dev_set_name(struct device *dev, const char *fmt, ...);
int dev_add_alias(struct device *dev, const char *fmt, ...);

/*
 * get resource 'num' for a device
 */
struct resource *dev_get_resource(struct device *dev, unsigned long type,
				  int num);
/*
 * get resource base 'name' for a device
 */
struct resource *dev_get_resource_by_name(struct device *dev,
					  unsigned long type,
					  const char *name);

int dev_request_resource(struct device *dev,
			 const struct resource *res);

/*
 * exlusively request register base 'name' for a device
 */
void __iomem *dev_request_mem_region_by_name(struct device *dev,
					     const char *name);

/*
 * get register base 'num' for a device
 */
void *dev_get_mem_region(struct device *dev, int num);

/*
 * exlusively request register base 'num' for a device
 * deprecated, use dev_request_mem_resource instead
 */
void __iomem *dev_request_mem_region(struct device *dev, int num);

/*
 * exlusively request resource 'num' for a device
 */
struct resource *dev_request_mem_resource(struct device *dev, int num);

/*
 * exlusively request resource 'name' for a device
 */
struct resource *dev_request_mem_resource_by_name(struct device *dev,
						  const char *name);

void __iomem *dev_platform_get_and_ioremap_resource(struct device *dev,
						    int num,
						    struct resource **out_res);

/*
 * exlusively request register base 'num' for a device
 * will return NULL on error
 * only used on platform like at91 where the Ressource address collision with
 * PTR errno
 */
void __iomem *dev_request_mem_region_err_null(struct device *dev, int num);

struct device *device_alloc(const char *devname, int id);

int device_add_resources(struct device *dev, const struct resource *res,
			 int num);

int device_add_resource(struct device *dev, const char *resname,
			resource_size_t start, resource_size_t size,
			unsigned int flags);

int device_add_data(struct device *dev, const void *data, size_t size);

struct device *add_child_device(struct device *parent,
				const char* devname, int id, const char *resname,
				resource_size_t start, resource_size_t size, unsigned int flags,
				void *pdata);

/*
 * register a generic device
 * with only one resource
 */
static inline struct device *add_generic_device(const char* devname, int id, const char *resname,
		resource_size_t start, resource_size_t size, unsigned int flags,
		void *pdata)
{
	return add_child_device(NULL, devname, id, resname, start, size, flags, pdata);
}

/*
 * register a generic device
 * with multiple resources
 */
struct device *add_generic_device_res(const char* devname, int id,
		struct resource *res, int nb, void *pdata);

/*
 * register a memory device
 */
static inline struct device *add_mem_device(const char *name, resource_size_t start,
		resource_size_t size, unsigned int flags)
{
	return add_generic_device("mem", DEVICE_ID_DYNAMIC, name, start, size,
				  IORESOURCE_MEM | flags, NULL);
}

static inline struct device *add_cfi_flash_device(int id, resource_size_t start,
		resource_size_t size, unsigned int flags)
{
	return add_generic_device("cfi_flash", id, NULL, start, size,
				  IORESOURCE_MEM | flags, NULL);
}

struct NS16550_plat;
static inline struct device *add_ns16550_device(int id, resource_size_t start,
		resource_size_t size, int flags, struct NS16550_plat *pdata)
{
	return add_generic_device("ns16550_serial", id, NULL, start, size,
				  flags, pdata);
}

#ifdef CONFIG_DRIVER_NET_DM9K
struct device *add_dm9000_device(int id, resource_size_t base,
		resource_size_t data, int flags, void *pdata);
#else
static inline struct device *add_dm9000_device(int id, resource_size_t base,
		resource_size_t data, int flags, void *pdata)
{
	return NULL;
}
#endif

#ifdef CONFIG_USB_EHCI
struct device *add_usb_ehci_device(int id, resource_size_t hccr,
		resource_size_t hcor, void *pdata);
#else
static inline struct device *add_usb_ehci_device(int id, resource_size_t hccr,
		resource_size_t hcor, void *pdata)
{
	return NULL;
}
#endif

#ifdef CONFIG_DRIVER_NET_KS8851_MLL
struct device *add_ks8851_device(int id, resource_size_t addr,
		resource_size_t addr_cmd, int flags, void *pdata);
#else
static inline struct device *add_ks8851_device(int id, resource_size_t addr,
		resource_size_t addr_cmd, int flags, void *pdata)
{
	return NULL;
}
#endif

static inline struct device *add_generic_usb_ehci_device(int id,
		resource_size_t base, void *pdata)
{
	return add_usb_ehci_device(id, base + 0x100, base + 0x140, pdata);
}

static inline struct device *add_gpio_keys_device(int id, void *pdata)
{
	return add_generic_device_res("gpio_keys", id, 0, 0, pdata);
}

/* linear list over all available devices
 */
extern struct list_head device_list;

/* linear list over all available drivers
 */
extern struct list_head driver_list;

/* linear list over all active devices
 */
extern struct list_head active_device_list;

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
struct driver *get_driver_by_name(const char *name);

struct cdev;

/* These are used by drivers which work with direct memory accesses */
ssize_t mem_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags);
ssize_t mem_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags);
ssize_t mem_copy(struct device *dev, void *dst, const void *src,
		 resource_size_t count, resource_size_t offset,
		 unsigned long flags);

int generic_memmap_ro(struct cdev *dev, void **map, int flags);
int generic_memmap_rw(struct cdev *dev, void **map, int flags);

static inline int dev_open_default(struct device *dev, struct filep *f)
{
	return 0;
}

static inline int dev_close_default(struct device *dev, struct filep *f)
{
	return 0;
}

struct bus_type {
	char *name;
	int (*match)(struct device *dev, struct driver *drv);
	int (*probe)(struct device *dev);
	void (*remove)(struct device *dev);

	struct device *dev;

	struct list_head list;
	struct list_head device_list;
	struct list_head driver_list;
};

int bus_register(struct bus_type *bus);
int device_match(struct device *dev, struct driver *drv);

extern struct list_head bus_list;

/* Iterate over all buses
 */
#define for_each_bus(bus) list_for_each_entry(bus, &bus_list, list)

/* Iterate over all devices of a bus
 */
#define bus_for_each_device(bus, dev) list_for_each_entry(dev, &(bus)->device_list, bus_list)

/* Iterate over all drivers of a bus
 */
#define bus_for_each_driver(bus, drv) list_for_each_entry(drv, &(bus)->driver_list, bus_list)

extern struct bus_type platform_bus;

int platform_driver_register(struct driver *drv);

/* register_driver_macro() - Helper macro for drivers that don't do
 * anything special in module registration. This eliminates a lot of
 * boilerplate. Each module may only use this macro once.
 */
#define register_driver_macro(level,bus,drv)		\
	static int __init drv##_register(void)		\
	{						\
		return bus##_driver_register(&drv);	\
	}						\
	level##_initcall(drv##_register)

#define core_platform_driver(drv)	\
	register_driver_macro(core,platform,drv)
#define postcore_platform_driver(drv)	\
	register_driver_macro(postcore,platform,drv)
#define coredevice_platform_driver(drv)	\
	register_driver_macro(coredevice,platform,drv)
#define device_platform_driver(drv)	\
	register_driver_macro(device,platform,drv)
#define console_platform_driver(drv)	\
	register_driver_macro(console,platform,drv)
#define fs_platform_driver(drv)	\
	register_driver_macro(fs,platform,drv)
#define late_platform_driver(drv)	\
	register_driver_macro(late,platform,drv)

#define mem_platform_driver(drv)						\
	static int __init drv##_init(void)					\
	{									\
		int ret;							\
		ret = platform_driver_register(&drv);				\
		if (ret)							\
			return ret;						\
		return of_devices_ensure_probed_by_dev_id(drv.of_compatible);	\
	}									\
	mem_initcall(drv##_init);

int platform_device_register(struct device *new_device);

struct cdev_operations {
	/*! Called in response of reading from this device. Required */
	ssize_t (*read)(struct cdev*, void* buf, size_t count, loff_t offset, ulong flags);

	/*! Called in response of write to this device. Required */
	ssize_t (*write)(struct cdev*, const void* buf, size_t count, loff_t offset, ulong flags);

	int (*ioctl)(struct cdev*, unsigned int, void *);
	int (*lseek)(struct cdev*, loff_t);
	int (*open)(struct cdev*, unsigned long flags);
	int (*close)(struct cdev*);
	int (*flush)(struct cdev*);
	int (*erase)(struct cdev*, loff_t count, loff_t offset);
	int (*protect)(struct cdev*, size_t count, loff_t offset, int prot);
	int (*discard_range)(struct cdev*, loff_t count, loff_t offset);
	int (*memmap)(struct cdev*, void **map, int flags);
	int (*truncate)(struct cdev*, size_t size);
};

#define MAX_UUID_STR	sizeof("00112233-4455-6677-8899-AABBCCDDEEFF")

struct cdev {
	const struct cdev_operations *ops;
	void *priv;
	struct device *dev;
	struct device_node *device_node;
	struct list_head list;
	struct list_head devices_list;
	char *name; /* filename under /dev/ */
	char *partname; /* the partition name, usually the above without the
			 * device part, i.e. name = "nand0.barebox" -> partname = "barebox"
			 */
	union {
		char diskuuid[MAX_UUID_STR];	/* GPT Header DiskGUID or
						 * MBR Header NT Disk Signature
						 */
		char partuuid[MAX_UUID_STR];	/* GPT Partition Entry UniquePartitionGUID or
						 * MBR Partition Entry "${nt_signature}-${partno}"
						 */
	};

	loff_t offset;
	loff_t size;
	unsigned int flags;
	u16 typeflags; /* GPT type-specific attributes */
	int open;
	struct mtd_info *mtd;
	struct cdev *link;
	struct list_head link_entry, links;
	struct list_head partition_entry, partitions;
	struct cdev *master;
	enum filetype filetype;
	union {
		u8 dos_partition_type;
		guid_t typeuuid;
	};
};

static inline struct device_node *cdev_of_node(const struct cdev *cdev)
{
	return IS_ENABLED(CONFIG_OFDEVICE) ? cdev->device_node : NULL;
}

static inline void cdev_set_of_node(struct cdev *cdev, struct device_node *np)
{
	if (IS_ENABLED(CONFIG_OFDEVICE))
		cdev->device_node = np;
}

static inline const char *cdev_name(struct cdev *cdev)
{
	return cdev ? cdev->name : NULL;
}

int devfs_create(struct cdev *);
int devfs_create_link(struct cdev *, const char *name);
int devfs_remove(struct cdev *);
int cdev_find_free_index(const char *);
struct cdev *device_find_partition(struct device *dev, const char *name);
struct cdev *cdev_by_name(const char *filename);
struct cdev *lcdev_by_name(const char *filename);
struct cdev *cdev_readlink(struct cdev *cdev);
struct cdev *cdev_by_device_node(struct device_node *node);
struct cdev *cdev_by_partuuid(const char *partuuid);
struct cdev *cdev_by_diskuuid(const char *partuuid);
struct cdev *cdev_open_by_name(const char *name, unsigned long flags);
struct cdev *cdev_create_loop(const char *path, ulong flags, loff_t offset);
void cdev_remove_loop(struct cdev *cdev);
int cdev_open(struct cdev *, unsigned long flags);
int cdev_fdopen(struct cdev *cdev, unsigned long flags);
int cdev_close(struct cdev *cdev);
int cdev_flush(struct cdev *cdev);
ssize_t cdev_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags);
ssize_t cdev_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags);
int cdev_ioctl(struct cdev *cdev, unsigned int cmd, void *buf);
int cdev_erase(struct cdev *cdev, loff_t count, loff_t offset);
int cdev_lseek(struct cdev*, loff_t);
int cdev_protect(struct cdev*, size_t count, loff_t offset, int prot);
int cdev_discard_range(struct cdev*, loff_t count, loff_t offset);
int cdev_memmap(struct cdev*, void **map, int flags);
int cdev_truncate(struct cdev*, size_t size);
loff_t cdev_unallocated_space(struct cdev *cdev);
static inline bool cdev_is_partition(const struct cdev *cdev)
{
	return cdev->master != NULL;
}

extern struct list_head cdev_list;
#define for_each_cdev(cdev) \
	list_for_each_entry((cdev), &cdev_list, list)

#define for_each_cdev_partition(partcdev, cdev) \
	list_for_each_entry((partcdev), &(cdev)->partitions, partition_entry)

#define DEVFS_PARTITION_FIXED		(1U << 0)
#define DEVFS_PARTITION_READONLY	(1U << 1)
#define DEVFS_IS_CHARACTER_DEV		(1U << 3)
#define DEVFS_IS_MCI_MAIN_PART_DEV	(1U << 4)
#define DEVFS_PARTITION_FROM_OF		(1U << 5)
#define DEVFS_PARTITION_FROM_TABLE	(1U << 6)
#define DEVFS_IS_MBR_PARTITIONED	(1U << 7)
#define DEVFS_IS_GPT_PARTITIONED	(1U << 8)
#define DEVFS_PARTITION_REQUIRED	(1U << 9)
#define DEVFS_PARTITION_NO_EXPORT	(1U << 10)
#define DEVFS_PARTITION_BOOTABLE_LEGACY	(1U << 11)
#define DEVFS_PARTITION_BOOTABLE_ESP	(1U << 12)
#define DEVFS_PARTITION_FOR_FIXUP	(1U << 13)

static inline bool cdev_is_mbr_partitioned(const struct cdev *master)
{
	return master && (master->flags & DEVFS_IS_MBR_PARTITIONED);
}

static inline bool cdev_is_gpt_partitioned(const struct cdev *master)
{
	return master && (master->flags & DEVFS_IS_GPT_PARTITIONED);
}

static inline struct cdev *
cdev_find_child_by_gpt_typeuuid(struct cdev *cdev, guid_t *typeuuid)
{
	struct cdev *partcdev;

	if (!cdev_is_gpt_partitioned(cdev))
		return ERR_PTR(-EINVAL);

	for_each_cdev_partition(partcdev, cdev) {
		if (guid_equal(&partcdev->typeuuid, typeuuid))
			return partcdev;
	}

	return ERR_PTR(-ENOENT);
}

#ifdef CONFIG_FS_AUTOMOUNT
void cdev_create_default_automount(struct cdev *cdev);
#else
static inline void cdev_create_default_automount(struct cdev *cdev)
{
}
#endif

static inline bool cdev_is_mci_main_part_dev(struct cdev *cdev)
{
	return cdev->flags & DEVFS_IS_MCI_MAIN_PART_DEV;
}

#define DEVFS_PARTITION_APPEND		0

/**
 * struct devfs_partition - defines parameters for a single partition
 * @offset: start of partition
 * 	a negative offset requests to start the partition relative to the
 * 	device's end. DEVFS_PARTITION_APPEND (i.e. 0) means start directly at
 * 	the end of the previous partition.
 * @size: size of partition
 * 	a non-positive value requests to use a size that keeps -size free space
 * 	after the current partition. A special case of this is passing 0, which
 * 	means "until end of device".
 * @flags: flags passed to devfs_add_partition
 * @name: name passed to devfs_add_partition
 * @bbname: if non-NULL also dev_add_bb_dev() is called for the partition during
 * 	devfs_create_partitions().
 */
struct devfs_partition {
	loff_t offset;
	loff_t size;
	unsigned int flags;
	const char *name;
	const char *bbname;
};
/**
 * devfs_create_partitions - create a set of partitions for a device
 * @devname: name of the device to partition
 * @partinfo: array of partition parameters
 * 	The array is processed until an entry with .name = NULL is found.
 */
int devfs_create_partitions(const char *devname,
		const struct devfs_partition partinfo[]);

struct cdev *devfs_add_partition(const char *devname, loff_t offset,
		loff_t size, unsigned int flags, const char *name);
int devfs_del_partition(const char *name);

struct cdev *cdevfs_add_partition(struct cdev *cdev,
		const struct devfs_partition *partinfo);
int cdevfs_del_partition(struct cdev *cdev);

#define of_match_ptr(compat) \
	IS_ENABLED(CONFIG_OFDEVICE) ? (compat) : NULL

#define DRV_OF_COMPAT(compat) of_match_ptr(compat)

/**
 * dev_get_drvdata - get driver match data associated with device
 * @dev: device instance
 * @data: pointer to void *, where match data is stored
 *
 * Returns 0 on success and error code otherwise.
 *
 * DEPRECATED: use device_get_match_data instead, which avoids
 * common pitfalls due to explicit pointer casts
 */
int dev_get_drvdata(struct device *dev, const void **data);

/**
 * device_get_match_data - get driver match data associated with device
 * @dev: device instance
 *
 * Returns match data on success and NULL otherwise
 */
const void *device_get_match_data(struct device *dev);

int device_match_of_modalias(struct device *dev, struct driver *drv);

struct device *device_find_child(struct device *parent, void *data,
				 int (*match)(struct device *dev, void *data));

static inline void *dev_get_priv(const struct device *dev)
{
	return dev->priv;
}

static inline bool dev_is_probed(const struct device *dev)
{
	return dev->driver ? true : false;
}

#endif /* DRIVER_H */

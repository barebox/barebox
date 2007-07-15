#ifndef DRIVER_H
#define DRIVER_H

#define MAX_DRIVER_NAME 16

#define DEVICE_TYPE_UNKNOWN     0
#define DEVICE_TYPE_ETHER       1
#define DEVICE_TYPE_CONSOLE     2
#define DEVICE_TYPE_DRAM	3
#define DEVICE_TYPE_BLOCK	4
#define DEVICE_TYPE_FS		5
#define DEVICE_TYPE_MIIPHY	6
#define MAX_DEVICE_TYPE         6

#include <param.h>

struct device_d {
	char name[MAX_DRIVER_NAME]; /* The name of this device. Used to match
				     * to the corresponding driver.
				     */
	char id[MAX_DRIVER_NAME];

	unsigned long size;

	/* For devices which are directly mapped into memory, i.e. NOR Flash or
	 * SDRAM.
	 */
	unsigned long map_base;

	void *platform_data; /* board specific information about this device */
	void *priv;          /* data private to the driver */
	void *type_data;     /* In case this device is a specific device, this pointer
			      * points to the type specific device, i.e. eth_device
			      */

	struct driver_d *driver; /* The driver for this device */

	struct device_d *next;

	unsigned long type;

	struct param_d *param;
};

struct driver_d {
	char name[MAX_DRIVER_NAME]; /* The name of this driver. Used to match to
				     * the corresponding device.
				     */
	struct driver_d *next;

	int     (*probe) (struct device_d *);
	int     (*remove)(struct device_d *);
	ssize_t (*read)  (struct device_d*, void* buf, size_t count, ulong offset, ulong flags);
	ssize_t (*write) (struct device_d*, const void* buf, size_t count, ulong offset, ulong flags);
	ssize_t (*erase) (struct device_d*, size_t count, unsigned long offset);
	int     (*memmap)(struct device_d*, void **map, int flags);

	void    (*info) (struct device_d *);
	void    (*shortinfo) (struct device_d *);

	unsigned long type;
	void *type_data; /* In case this driver is of a specific type, i.e. a filesystem
			  * driver, this pointer points to the corresponding data struct
			  */
};

#define RW_SIZE(x)      (x)
#define RW_SIZE_MASK    0x7

/* Register/unregister devices and drivers. Since we don't have modules
 * we do not need a driver_unregister function.
 */
int register_driver(struct driver_d *);
int register_device(struct device_d *);
void unregister_device(struct device_d *);

/* Iterate through the devices of a given type. if last is NULL, the
 * first device of this type is returned. Put this pointer in as
 * 'last' to get the next device. This functions returns NULL if no
 * more devices are found.
 */
struct device_d *get_device_by_type(ulong type, struct device_d *last);
struct device_d *get_device_by_id(const char *id);
struct device_d *get_first_device(void);

/* Find a free device id from the given template. This is archieved by
 * appending a number to the template. Dynamically created devices should
 * use this function rather than filling the id field themselves.
 */
int get_free_deviceid(char *id, char *id_template);

struct device_d *device_from_spec_str(const char *str, char **endp);
char *deviceid_from_spec_str(const char *str, char **endp);

/* Find a driver with the given name. Currently the filesystem implementation
 * uses this to get the driver from the name the user specifies with the
 * mount command
 */
struct driver_d *get_driver_by_name(const char *name);

ssize_t dev_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t dev_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags);
ssize_t dev_erase(struct device_d *dev, size_t count, unsigned long offset);
int     dev_memmap(struct device_d *dev, void **map, int flags);

/* These are used by drivers which work with direct memory accesses */
ssize_t mem_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t mem_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags);

/* Use this if you have nothing to do in your drivers probe function */
int dummy_probe(struct device_d *);

int generic_memmap_ro(struct device_d *dev, void **map, int flags);
int generic_memmap_rw(struct device_d *dev, void **map, int flags);

#endif /* DRIVER_H */


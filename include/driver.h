#ifndef DRIVER_H
#define DRIVER_H

#define MAX_DRIVER_NAME 16

#define MAP_READ        1
#define MAP_WRITE       2

#define DEVICE_TYPE_UNKNOWN     0
#define DEVICE_TYPE_ETHER       1
#define DEVICE_TYPE_STDIO       2
#define DEVICE_TYPE_DRAM	3
#define DEVICE_TYPE_BLOCK	4
#define DEVICE_TYPE_FS		5
#define MAX_DEVICE_TYPE         5

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

        void    (*info) (struct device_d *);
        void    (*shortinfo) (struct device_d *);

        unsigned long type;
        void *type_data; /* In case this driver is of a specific type, i.e. a filesystem
			  * driver, this pointer points to the corresponding data struct
			  */
};

#define RW_SIZE(x)      (x)
#define RW_SIZE_MASK    0x7

int register_driver(struct driver_d *);
int register_device(struct device_d *);
void unregister_device(struct device_d *);

struct device_d *device_from_spec_str(const char *str, char **endp);
struct device_d *get_device_by_name(char *name);
struct device_d *get_device_by_type(ulong type, struct device_d *last);
struct device_d *get_device_by_id(const char *id);
struct device_d *get_first_device(void);
int get_free_deviceid(char *id, char *id_template);

struct driver_d *get_driver_by_name(char *name);

ssize_t dev_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t dev_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags);
ssize_t dev_erase(struct device_d *dev, size_t count, unsigned long offset);

ssize_t mem_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t mem_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags);

int dummy_probe(struct device_d *);

#endif /* DRIVER_H */


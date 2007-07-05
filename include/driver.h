#ifndef DRIVER_H
#define DRIVER_H

#define MAX_DRIVER_NAME 16

#define MAP_READ        1
#define MAP_WRITE       2

struct memarea_info;

struct device_d {
	char name[MAX_DRIVER_NAME];

	unsigned long size;

        /* For devices which are directly mapped into memory, i.e. NOR Flash or
         * SDRAM.
         */
        unsigned long map_base;
        unsigned long map_flags;

        void *platform_data;

        /* The driver for this device */
        struct driver_d *driver;

	struct device_d *next;

        struct partition *part;
};

struct driver_d {
	char name[MAX_DRIVER_NAME];

        void *priv;

	struct driver_d *next;

        int     (*probe) (struct device_d *);
        ssize_t (*read)  (struct device_d*, void* buf, size_t count, unsigned long offset);
        ssize_t (*write) (struct device_d*, void* buf, size_t count, unsigned long offset);
        ssize_t (*erase) (struct device_d*, struct memarea_info *);
};

int register_driver(struct driver_d *);
int register_device(struct device_d *);
void unregister_device(struct device_d *);

struct device_d *device_from_spec_str(const char *str, char **endp);
struct device_d *get_device_by_name(char *name);
#endif /* DRIVER_H */


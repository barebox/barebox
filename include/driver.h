#ifndef DRIVER_H
#define DRIVER_H

#define MAX_DRIVER_NAME 16

#define MAP_READ        1
#define MAP_WRITE       2

struct param_d {
        char *(*get)(struct device_d *, ulong cookie);
        int  (*set)(struct device_d *, ulong cookie, char *val);
        char *name;
        ulong cookie;
        struct param_d *next;
};

#define DEVICE_TYPE_UNKNOWN     0
#define DEVICE_TYPE_ETHER       1
#define DEVICE_TYPE_STDIO       2
#define MAX_DEVICE_TYPE         2

struct device_d {
	char name[MAX_DRIVER_NAME];
        char id[MAX_DRIVER_NAME];

	unsigned long size;

        /* For devices which are directly mapped into memory, i.e. NOR Flash or
         * SDRAM.
         */
        unsigned long map_base;

        void *platform_data;
        void *priv;

        /* The driver for this device */
        struct driver_d *driver;

	struct device_d *next;

        unsigned long type;

        struct param_d *param;
};

struct driver_d {
	char name[MAX_DRIVER_NAME];

	struct driver_d *next;

        int     (*probe) (struct device_d *);
        ssize_t (*read)  (struct device_d*, void* buf, size_t count, ulong offset, ulong flags);
        ssize_t (*write) (struct device_d*, void* buf, size_t count, ulong offset, ulong flags);
        ssize_t (*erase) (struct device_d*, size_t count, unsigned long offset);

        void    (*info) (struct device_d *);
        void    (*shortinfo) (struct device_d *);

	struct param_d* (*get) (struct device_d*, int no);
	int	(*set) (struct device_d*, struct param_d *, char *val);

        unsigned long type;
        void *type_data;
};

#define RW_SIZE(x)      (x)
#define RW_SIZE_MASK    0x7

int register_driver(struct driver_d *);
int register_device(struct device_d *);
void unregister_device(struct device_d *);

struct device_d *device_from_spec_str(const char *str, char **endp);
struct device_d *get_device_by_name(char *name);

ssize_t read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t write(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t erase(struct device_d *dev, size_t count, unsigned long offset);
char *dev_get_param(struct device_d *dev, char *name);
int dev_set_param(struct device_d *dev, char *name, char *val);

int dev_add_parameter(struct device_d *dev, struct param_d *par);

ssize_t mem_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);
ssize_t mem_write(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags);

int register_device_type_handler(int(*handle)(struct device_d *), ulong device_type);
//void unregister_device_type_handler(struct device_d *);

int dummy_probe(struct device_d *);

int global_add_parameter(struct param_d *param);

#endif /* DRIVER_H */


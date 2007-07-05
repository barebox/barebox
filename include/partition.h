#ifndef __PARTITION_H
#define __PARTITION_H

struct device_d;

struct partition {
        int num;

	int readonly;

        unsigned long offset;

        struct device_d *physdev;
        struct device_d device;

        char name[16];
};

struct device_d *dev_add_partition(struct device_d *dev, unsigned long offset,
		size_t size, char *name);
/* FIXME: counterpart missing */

#endif /* __PARTITION_H */


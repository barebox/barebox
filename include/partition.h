#ifndef __PARTITION_H
#define __PARTITION_H

struct device_d;

#define PARTITION_FIXED		(1 << 0)
#define PARTITION_READONLY	(1 << 1)

struct partition {
        int num;

	int flags;

        unsigned long offset;

        struct device_d *physdev;
        struct device_d device;

        char name[16];
};

struct device_d *dev_add_partition(struct device_d *dev, unsigned long offset,
		size_t size, int flags, const char *name);
/* FIXME: counterpart missing */

#endif /* __PARTITION_H */


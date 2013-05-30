#ifndef __PARTITION_H
#define __PARTITION_H

#include <driver.h>

struct partition {
        int num;

	int flags;

        loff_t offset;

        struct device_d *physdev;
        struct device_d device;

        char name[16];
	struct cdev cdev;
};

#endif /* __PARTITION_H */

#ifndef __PARTITION_H
#define __PARTITION_H

struct device_d;

struct partition {
        int num;

        struct device_d *parent;

        struct device_d device;

        struct partition *next;
};

#endif /* __PARTITION_H */


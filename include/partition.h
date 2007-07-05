#ifndef __PARTITION_H
#define __PARTITION_H

struct device_d;

struct partition {
        int num;

        unsigned long offset;

        struct device_d *parent;
        struct device_d device;

        char name[16];
};

#endif /* __PARTITION_H */


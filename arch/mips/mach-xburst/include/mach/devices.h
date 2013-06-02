#ifndef __MACH_XBURST_DEVICES_H
#define __MACH_XBURST_DEVICES_H

#include <driver.h>

struct device_d *jz_add_uart(int id, unsigned long base, unsigned int clock);

#endif /* __MACH_XBURST_DEVICES_H */

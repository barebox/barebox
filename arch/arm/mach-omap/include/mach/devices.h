#ifndef __MACH_OMAP_DEVICES_H
#define __MACH_OMAP_DEVICES_H

#include <mach/omap_hsmmc.h>

void omap_add_ram0(resource_size_t size);

void omap_add_sram0(resource_size_t base, resource_size_t size);

struct device_d *omap_add_uart(int id, unsigned long base);

#endif /* __MACH_OMAP_DEVICES_H */

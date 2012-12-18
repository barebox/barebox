#ifndef __MACH_OMAP_DEVICES_H
#define __MACH_OMAP_DEVICES_H

#include <mach/omap_hsmmc.h>

struct device_d *omap_add_uart(int id, unsigned long base);

struct device_d *omap_add_mmc(int id, unsigned long base,
		struct omap_hsmmc_platform_data *pdata);

struct device_d *omap_add_i2c(int id, unsigned long base, void *pdata);

#endif /* __MACH_OMAP_DEVICES_H */

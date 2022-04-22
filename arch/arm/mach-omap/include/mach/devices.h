/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_OMAP_DEVICES_H
#define __MACH_OMAP_DEVICES_H

#include <mach/omap_hsmmc.h>
#include <video/omap-fb.h>

void omap_add_ram0(resource_size_t size);

void omap_add_sram0(resource_size_t base, resource_size_t size);

struct device_d *omap_add_uart(int id, unsigned long base);

struct device_d *omap_add_display(struct omapfb_platform_data *o_pdata);

#endif /* __MACH_OMAP_DEVICES_H */

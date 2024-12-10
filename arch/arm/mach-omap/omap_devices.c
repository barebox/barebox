// SPDX-License-Identifier: GPL-2.0-only

#include <driver.h>
#include <asm/armlinux.h>

#include <mach/omap/generic.h>
#include <mach/omap/omap3-devices.h>

void omap_add_ram0(resource_size_t size)
{
	arm_add_mem_device("ram0", OMAP_DRAM_ADDR_SPACE_START, size);
}

void omap_add_sram0(resource_size_t base, resource_size_t size)
{
	add_mem_device("sram0", base, size, IORESOURCE_MEM_WRITEABLE);
}

struct device *omap_add_uart(int id, unsigned long base)
{
	return add_generic_device("omap-uart", id, NULL, base, 1024,
				  IORESOURCE_MEM | IORESOURCE_MEM_8BIT, NULL);
}

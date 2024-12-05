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

#if defined(CONFIG_DRIVER_VIDEO_OMAP)
static struct resource omapfb_resources[] = {
	{
		.name	= "omap4_dss",
		.start	= 0x48040000,
		.end	= 0x48040000 + 512 - 1,
		.flags	= IORESOURCE_MEM | IORESOURCE_MEM_32BIT,
	}, {
		.name	= "omap4_dispc",
		.start	= 0x48041000,
		.end	= 0x48041000 + 3072 - 1,
		.flags	= IORESOURCE_MEM | IORESOURCE_MEM_32BIT,
	},
};

struct device *omap_add_display(struct omapfb_platform_data *o_pdata)
{
	return add_generic_device_res("omap_fb", -1,
				      omapfb_resources,
				      ARRAY_SIZE(omapfb_resources),
				      o_pdata);
}
#else
struct device *omap_add_display(struct omapfb_platform_data *o_pdata)
{
	return NULL;
}
#endif

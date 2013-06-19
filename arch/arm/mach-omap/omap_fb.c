#include <driver.h>
#include <common.h>
#include <linux/ioport.h>
#include <mach/omap-fb.h>

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

struct device_d *omap_add_display(struct omapfb_platform_data *o_pdata)
{
	return add_generic_device_res("omap_fb", -1,
				      omapfb_resources,
				      ARRAY_SIZE(omapfb_resources),
				      o_pdata);
}
#else
struct device_d *omap_add_display(struct omapfb_platform_data *o_pdata)
{
	return NULL;
}
#endif
EXPORT_SYMBOL(omap_add_display);

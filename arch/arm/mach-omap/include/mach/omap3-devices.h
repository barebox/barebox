#include <driver.h>
#include <sizes.h>

#include <mach/mcspi.h>

/* the device numbering is the same as in the device tree */

static inline struct device_d *omap3_add_spi(int id, resource_size_t start)
{
	return add_generic_device("omap3_spi", id, NULL, start, SZ_4K,
				   IORESOURCE_MEM, NULL);
}

static inline struct device_d *omap3_add_spi1(void)
{
	return omap3_add_spi(1, OMAP3_MCSPI1_BASE);
}

static inline struct device_d *omap3_add_spi2(void)
{
	return omap3_add_spi(2, OMAP3_MCSPI2_BASE);
}

static inline struct device_d *omap3_add_spi3(void)
{
	return omap3_add_spi(3, OMAP3_MCSPI3_BASE);
}

static inline struct device_d *omap3_add_spi4(void)
{
	return omap3_add_spi(4, OMAP3_MCSPI4_BASE);
}

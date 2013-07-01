#ifndef __MACH_OMAP3_DEVICES_H
#define __MACH_OMAP3_DEVICES_H

#include <driver.h>
#include <sizes.h>
#include <mach/omap3-silicon.h>
#include <mach/devices.h>
#include <mach/mcspi.h>
#include <mach/omap_hsmmc.h>


static inline void omap3_add_sram0(void)
{
	return omap_add_sram0(OMAP3_SRAM_BASE, 64 * SZ_1K);
}

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

static inline struct device_d *omap3_add_uart1(void)
{
	return omap_add_uart(0, OMAP3_UART1_BASE);
}

static inline struct device_d *omap3_add_uart2(void)
{
	return omap_add_uart(1, OMAP3_UART2_BASE);
}

static inline struct device_d *omap3_add_uart3(void)
{
	return omap_add_uart(2, OMAP3_UART3_BASE);
}

static inline struct device_d *omap3_add_mmc1(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap3-hsmmc", 0, NULL,
			OMAP3_MMC1_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap3_add_mmc2(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap3-hsmmc", 1, NULL,
			OMAP3_MMC2_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap3_add_mmc3(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap3-hsmmc", 2, NULL,
			OMAP3_MMC3_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap3_add_i2c1(void *pdata)
{
	return add_generic_device("i2c-omap3", 0, NULL, OMAP3_I2C1_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap3_add_i2c2(void *pdata)
{
	return add_generic_device("i2c-omap3", 1, NULL, OMAP3_I2C2_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap3_add_i2c3(void *pdata)
{
	return add_generic_device("i2c-omap3", 2, NULL, OMAP3_I2C3_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap3_add_ehci(void *pdata)
{
	return add_usb_ehci_device(DEVICE_ID_DYNAMIC, OMAP3_EHCI_BASE,
					OMAP3_EHCI_BASE + 0x10, pdata);
}

#endif /* __MACH_OMAP3_DEVICES_H */

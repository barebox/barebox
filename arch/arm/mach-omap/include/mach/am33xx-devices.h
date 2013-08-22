#ifndef __MACH_OMAP3_DEVICES_H
#define __MACH_OMAP3_DEVICES_H

#include <driver.h>
#include <sizes.h>
#include <mach/am33xx-silicon.h>
#include <mach/devices.h>
#include <mach/omap_hsmmc.h>
#include <mach/cpsw.h>

/* the device numbering is the same as in the TRM memory map (SPRUH73G) */

static inline struct device_d *am33xx_add_uart0(void)
{
	return omap_add_uart(0, AM33XX_UART0_BASE);
}

static inline struct device_d *am33xx_add_uart1(void)
{
	return omap_add_uart(1, AM33XX_UART1_BASE);
}

static inline struct device_d *am33xx_add_uart2(void)
{
	return omap_add_uart(2, AM33XX_UART2_BASE);
}

static inline struct device_d *am33xx_add_mmc0(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 0, NULL,
			AM33XX_MMCHS0_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *am33xx_add_mmc1(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 1, NULL,
			AM33XX_MMC1_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *am33xx_add_cpsw(struct cpsw_platform_data *cpsw_data)
{
	return add_generic_device("cpsw", 0, NULL,
			AM335X_CPSW_BASE, SZ_32K, IORESOURCE_MEM, cpsw_data);
}

static inline struct device_d *am33xx_add_spi(int id, resource_size_t start)
{
	return add_generic_device("omap3_spi", id, NULL, start + 0x100, SZ_4K - 0x100,
				   IORESOURCE_MEM, NULL);
}

static inline struct device_d *am33xx_add_spi0(void)
{
	return am33xx_add_spi(0, AM33XX_MCSPI0_BASE);
}

static inline struct device_d *am33xx_add_spi1(void)
{
	return am33xx_add_spi(1, AM33XX_MCSPI1_BASE);
}

static inline struct device_d *am33xx_add_i2c0(void *pdata)
{
	return add_generic_device("i2c-am33xx", 0, NULL, AM33XX_I2C0_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *am33xx_add_i2c1(void *pdata)
{
	return add_generic_device("i2c-am33xx", 1, NULL, AM33XX_I2C1_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *am33xx_add_i2c2(void *pdata)
{
	return add_generic_device("i2c-am33xx", 2, NULL, AM33XX_I2C2_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

#endif /* __MACH_OMAP3_DEVICES_H */

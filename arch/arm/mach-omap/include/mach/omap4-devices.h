#ifndef __MACH_OMAP4_DEVICES_H
#define __MACH_OMAP4_DEVICES_H

#include <driver.h>
#include <sizes.h>
#include <mach/devices.h>
#include <mach/omap4-silicon.h>
#include <mach/mcspi.h>
#include <mach/omap_hsmmc.h>

static inline void omap44xx_add_sram0(void)
{
	return omap_add_sram0(OMAP44XX_SRAM_BASE, 48 * SZ_1K);
}

static inline struct device_d *omap44xx_add_uart1(void)
{
	return omap_add_uart(0, OMAP44XX_UART1_BASE);
}

static inline struct device_d *omap44xx_add_uart2(void)
{
	return omap_add_uart(1, OMAP44XX_UART2_BASE);
}

static inline struct device_d *omap44xx_add_uart3(void)
{
	return omap_add_uart(2, OMAP44XX_UART3_BASE);
}

static inline struct device_d *omap44xx_add_mmc1(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 0, NULL,
			OMAP44XX_MMC1_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_mmc2(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 1, NULL,
			OMAP44XX_MMC2_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_mmc3(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 2, NULL,
			OMAP44XX_MMC3_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_mmc4(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 3, NULL,
			OMAP44XX_MMC4_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_mmc5(struct omap_hsmmc_platform_data *pdata)
{
	return add_generic_device("omap4-hsmmc", 4, NULL,
			OMAP44XX_MMC5_BASE, SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_i2c1(void *pdata)
{
	return add_generic_device("i2c-omap4", 0, NULL, OMAP44XX_I2C1_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_i2c2(void *pdata)
{
	return add_generic_device("i2c-omap4", 1, NULL, OMAP44XX_I2C2_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_i2c3(void *pdata)
{
	return add_generic_device("i2c-omap4", 2, NULL, OMAP44XX_I2C3_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_i2c4(void *pdata)
{
	return add_generic_device("i2c-omap4", 3, NULL, OMAP44XX_I2C4_BASE,
			SZ_4K, IORESOURCE_MEM, pdata);
}

static inline struct device_d *omap44xx_add_ehci(void *pdata)
{
	return add_usb_ehci_device(DEVICE_ID_DYNAMIC, OMAP44XX_EHCI_BASE,
				OMAP44XX_EHCI_BASE + 0x10, pdata);
}

#endif /* __MACH_OMAP4_DEVICES_H */

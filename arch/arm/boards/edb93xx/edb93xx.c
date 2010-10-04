/*
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <init.h>
#include <partition.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <generated/mach-types.h>
#include <mach/ep93xx-regs.h>
#include "edb93xx.h"

#define DEVCFG_U1EN (1 << 18)

/*
 * Up to 32MiB NOR type flash, connected to
 * CS line 6, data width is 16 bit
 */
static struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",
	.map_base = 0x60000000,
	.size     = EDB93XX_CFI_FLASH_SIZE,
};

static struct memory_platform_data ram_dev_pdata0 = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram0_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = CONFIG_EP93XX_SDRAM_BANK0_BASE,
	.size     = CONFIG_EP93XX_SDRAM_BANK0_SIZE,
	.platform_data = &ram_dev_pdata0,
};

#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 2)
static struct memory_platform_data ram_dev_pdata1 = {
	.name = "ram1",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram1_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = CONFIG_EP93XX_SDRAM_BANK1_BASE,
	.size     = CONFIG_EP93XX_SDRAM_BANK1_SIZE,
	.platform_data = &ram_dev_pdata1,
};
#endif

#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 3)
static struct memory_platform_data ram_dev_pdata2 = {
	.name = "ram2",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram2_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = CONFIG_EP93XX_SDRAM_BANK2_BASE,
	.size     = CONFIG_EP93XX_SDRAM_BANK2_SIZE,
	.platform_data = &ram_dev_pdata2,
};
#endif

#if (CONFIG_EP93XX_SDRAM_NUM_BANKS == 4)
static struct memory_platform_data ram_dev_pdata3 = {
	.name = "ram3",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram3_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = CONFIG_EP93XX_SDRAM_BANK3_BASE,
	.size     = CONFIG_EP93XX_SDRAM_BANK3_SIZE,
	.platform_data = &ram_dev_pdata3,
};
#endif

static struct device_d eth_dev = {
	.id	  = -1,
	.name     = "ep93xx_eth",
};

static int ep93xx_devices_init(void)
{
	register_device(&cfi_dev);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	register_device(&sdram0_dev);
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 2)
	register_device(&sdram1_dev);
#endif
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 3)
	register_device(&sdram2_dev);
#endif
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS == 4)
	register_device(&sdram3_dev);
#endif

	armlinux_add_dram(&sdram0_dev);
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 2)
	armlinux_add_dram(&sdram1_dev);
#endif
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 3)
	armlinux_add_dram(&sdram2_dev);
#endif
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS == 4)
	armlinux_add_dram(&sdram3_dev);
#endif

	register_device(&eth_dev);

	armlinux_set_bootparams((void *)CONFIG_EP93XX_SDRAM_BANK0_BASE + 0x100);

	armlinux_set_architecture(MACH_TYPE);

	return 0;
}

device_initcall(ep93xx_devices_init);

static struct device_d edb93xx_serial_device = {
	.id	  = -1,
	.name     = "pl010_serial",
	.map_base = UART1_BASE,
	.size     = 4096,
};

static int edb93xx_console_init(void)
{
	struct syscon_regs *syscon = (struct syscon_regs *)SYSCON_BASE;

	/*
	 * set UARTBAUD bit to drive UARTs with 14.7456MHz instead of
	 * 14.7456/2 MHz
	 */
	uint32_t value = readl(&syscon->pwrcnt);
	value |= SYSCON_PWRCNT_UART_BAUD;
	writel(value, &syscon->pwrcnt);

	/* Enable UART1 */
	value = readl(&syscon->devicecfg);
	value |= DEVCFG_U1EN;
	writel(0xAA, &syscon->sysswlock);
	writel(value, &syscon->devicecfg);

	register_device(&edb93xx_serial_device);

	return 0;
}

console_initcall(edb93xx_console_init);

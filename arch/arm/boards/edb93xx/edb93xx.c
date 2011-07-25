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
static struct device_d eth_dev = {
	.id	  = -1,
	.name     = "ep93xx_eth",
};

static int ep93xx_devices_init(void)
{
	struct device_d *sdram_dev;

	add_cfi_flash_device(-1, 0x60000000, EDB93XX_CFI_FLASH_SIZE, 0);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 0x20000, PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	sdram_dev = add_mem_device("ram0", CONFIG_EP93XX_SDRAM_BANK0_BASE,
				   CONFIG_EP93XX_SDRAM_BANK0_SIZE,
				   IORESOURCE_MEM_WRITEABLE);
	armlinux_add_dram(sdram_dev);
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 2)
	sdram_dev = add_mem_device("ram1", CONFIG_EP93XX_SDRAM_BANK1_BASE,
				   CONFIG_EP93XX_SDRAM_BANK1_SIZE,
				   IORESOURCE_MEM_WRITEABLE);
	armlinux_add_dram(sdram_dev);
#endif
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS >= 3)
	sdram_dev = add_mem_device("ram2", CONFIG_EP93XX_SDRAM_BANK2_BASE,
				   CONFIG_EP93XX_SDRAM_BANK2_SIZE,
				   IORESOURCE_MEM_WRITEABLE);
	armlinux_add_dram(sdram_dev);
#endif
#if (CONFIG_EP93XX_SDRAM_NUM_BANKS == 4)
	sdram_dev = add_mem_device("ram3", CONFIG_EP93XX_SDRAM_BANK3_BASE,
				   CONFIG_EP93XX_SDRAM_BANK2_SIZE,
				   IORESOURCE_MEM_WRITEABLE);
	armlinux_add_dram(sdram_dev);
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

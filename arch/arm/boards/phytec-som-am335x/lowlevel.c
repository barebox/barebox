/*
 * Copyright (C) 2015 Wadim Egorov, PHYTEC Messtechnik GmbH
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
 */

#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <init.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/generic.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/am33xx-mux.h>
#include <mach/am33xx-generic.h>
#include <mach/wdt.h>
#include <debug_ll.h>

#include "ram-timings.h"

#define DDR_IOCTRL	0x18B

static const struct am33xx_cmd_control physom_cmd = {
	.slave_ratio0	= 0x80,
	.dll_lock_diff0	= 0x0,
	.invert_clkout0	= 0x0,
	.slave_ratio1	= 0x80,
	.dll_lock_diff1	= 0x0,
	.invert_clkout1	= 0x0,
	.slave_ratio2	= 0x80,
	.dll_lock_diff2	= 0x0,
	.invert_clkout2	= 0x0,
};

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static noinline void physom_board_init(int sdram, void *fdt)
{
	struct am335x_sdram_timings *timing = &physom_timings[sdram];

	/*
	 * WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	am33xx_pll_init(MPUPLL_M_600, DDRPLL_M_400);

	am335x_sdram_init(DDR_IOCTRL, &physom_cmd,
			&timing->regs,
			&timing->data);

	am33xx_uart_soft_reset((void *)AM33XX_UART0_BASE);
	am33xx_enable_uart0_pin_mux();
	omap_uart_lowlevel_init((void *)AM33XX_UART0_BASE);
	putc_ll('>');

	am335x_barebox_entry(fdt);
}

static noinline void physom_board_entry(unsigned long bootinfo, int sdram, void *fdt)
{
	am33xx_save_bootinfo((void *)bootinfo);

	arm_cpu_lowlevel_init();

	/*
	 * Setup C environment, the board init code uses global variables.
	 * Stackpointer has already been initialized by the ROM code.
	 */
	relocate_to_current_adr();
	setup_c();

	physom_board_init(sdram, fdt);
}

#define PHYTEC_ENTRY_MLO(name, fdt_name, sdram)			\
	ENTRY_FUNCTION(name, bootinfo, r1, r2)			\
	{							\
		extern char __dtb_z_##fdt_name##_start[];		\
		void *fdt = __dtb_z_##fdt_name##_start -		\
			get_runtime_offset();			\
		physom_board_entry(bootinfo, sdram, fdt);	\
	}

#define PHYTEC_ENTRY(name, fdt_name)				\
	ENTRY_FUNCTION(name, r0, r1, r2)			\
	{							\
		extern char __dtb_z_##fdt_name##_start[];		\
		void *fdt = __dtb_z_##fdt_name##_start -		\
			get_runtime_offset();			\
		am335x_barebox_entry(fdt);			\
	}

/* phycore-som */
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_sram_128mb, am335x_phytec_phycore_som_mlo, PHYCORE_MT41J64M1615IT_128MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_sram_256mb, am335x_phytec_phycore_som_mlo, PHYCORE_MT41J128M16125IT_256MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_sram_512mb, am335x_phytec_phycore_som_mlo, PHYCORE_MT41J256M16HA15EIT_512MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_sram_2x512mb, am335x_phytec_phycore_som_mlo, PHYCORE_MT41J512M8125IT_2x512MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_r2_sram_512mb, am335x_phytec_phycore_som_mlo, PHYCORE_R2_MT41K256M16TW107IT_512MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_r2_sram_256mb, am335x_phytec_phycore_som_mlo, PHYCORE_R2_MT41K128M16JT_256MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycore_r2_sram_1024mb,  am335x_phytec_phycore_som_mlo, PHYCORE_R2_MT41K512M16HA125IT_1024MB);
PHYTEC_ENTRY(start_am33xx_phytec_phycore_nand_sdram, am335x_phytec_phycore_som_nand);
PHYTEC_ENTRY(start_am33xx_phytec_phycore_emmc_sdram, am335x_phytec_phycore_som_emmc);
PHYTEC_ENTRY(start_am33xx_phytec_phycore_nand_no_spi_sdram, am335x_phytec_phycore_som_nand_no_spi);
PHYTEC_ENTRY(start_am33xx_phytec_phycore_nand_no_eeprom_sdram, am335x_phytec_phycore_som_nand_no_eeprom);
PHYTEC_ENTRY(start_am33xx_phytec_phycore_nand_no_spi_no_eeprom_sdram, am335x_phytec_phycore_som_nand_no_spi_no_eeprom);

/* phyflex-som */
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phyflex_sram_256mb, am335x_phytec_phyflex_som_mlo, PHYFLEX_MT41K128M16JT_256MB);
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phyflex_sram_512mb, am335x_phytec_phyflex_som_mlo, PHYFLEX_MT41K256M16HA_512MB);
PHYTEC_ENTRY(start_am33xx_phytec_phyflex_sdram, am335x_phytec_phyflex_som);
PHYTEC_ENTRY(start_am33xx_phytec_phyflex_no_spi_sdram, am335x_phytec_phyflex_som_no_spi);
PHYTEC_ENTRY(start_am33xx_phytec_phyflex_no_eeprom_sdram, am335x_phytec_phyflex_som_no_eeprom);
PHYTEC_ENTRY(start_am33xx_phytec_phyflex_no_spi_no_eeprom_sdram, am335x_phytec_phyflex_som_no_spi_no_eeprom);

/* phycard-som */
PHYTEC_ENTRY_MLO(start_am33xx_phytec_phycard_sram_256mb, am335x_phytec_phycard_som_mlo, PHYCARD_NT5CB128M16BP_256MB);
PHYTEC_ENTRY(start_am33xx_phytec_phycard_sdram, am335x_phytec_phycard_som);

/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Authors:
 *	Aneesh V <aneesh@ti.com>
 *
 * Derived from OMAP3 work by
 *	Richard Woodruff <r-woodruff2@ti.com>
 *	Syed Mohammed Khasim <x0khasim@ti.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _OMAP4_H_
#define _OMAP4_H_

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
#include <asm/types.h>
#endif /* !(__KERNEL_STRICT_NAMES || __ASSEMBLY__) */

/*
 * L4 Peripherals - L4 Wakeup and L4 Core now
 */
#define OMAP44XX_L4_CORE_BASE		0x4A000000
#define OMAP44XX_WAKEUP_L4_IO_BASE	0x4A300000
#define OMAP44XX_L4_WKUP_BASE		0x4A300000
#define OMAP44XX_L4_PER_BASE		0x48000000

#define OMAP44XX_SRAM_BASE		0x40300000

#define OMAP44XX_SRAM_BASE		0x40300000

/* EMIF and DMM registers */
#define OMAP44XX_EMIF1_BASE		0x4c000000
#define OMAP44XX_EMIF2_BASE		0x4d000000

#define OMAP44XX_DRAM_ADDR_SPACE_START	0x80000000
#define OMAP44XX_DRAM_ADDR_SPACE_END	0xD0000000


/* CONTROL */
#define OMAP44XX_CTRL_BASE		(OMAP44XX_L4_CORE_BASE + 0x2000)
#define OMAP44XX_CONTROL_PADCONF_CORE	(OMAP44XX_L4_CORE_BASE + 0x100000)
#define OMAP44XX_CONTROL_PADCONF_WKUP	(OMAP44XX_L4_CORE_BASE + 0x31E000)

/* PRM */
#define OMAP44XX_PRM_VC_VAL_BYPASS	(OMAP44XX_WAKEUP_L4_IO_BASE + 0x7ba0)
#define OMAP44XX_PRM_VC_CFG_I2C_MODE	(OMAP44XX_WAKEUP_L4_IO_BASE + 0x7ba8)
#define OMAP44XX_PRM_VC_CFG_I2C_CLK	(OMAP44XX_WAKEUP_L4_IO_BASE + 0x7bac)
#define OMAP44XX_PRM_VC_VAL_BYPASS_VALID_BIT		0x1000000
#define OMAP44XX_PRM_VC_VAL_BYPASS_SLAVEADDR_SHIFT	0
#define OMAP44XX_PRM_VC_VAL_BYPASS_SLAVEADDR_MASK	0x7F
#define OMAP44XX_PRM_VC_VAL_BYPASS_REGADDR_SHIFT	8
#define OMAP44XX_PRM_VC_VAL_BYPASS_REGADDR_MASK		0xFF
#define OMAP44XX_PRM_VC_VAL_BYPASS_DATA_SHIFT		16
#define OMAP44XX_PRM_VC_VAL_BYPASS_DATA_MASK		0xFF

/* IRQ */
#define OMAP44XX_PRM_IRQSTATUS_MPU_A9	(OMAP44XX_WAKEUP_L4_IO_BASE + 0x6010)

/* UART */
#define OMAP44XX_UART1_BASE		(OMAP44XX_L4_PER_BASE + 0x6a000)
#define OMAP44XX_UART2_BASE		(OMAP44XX_L4_PER_BASE + 0x6c000)
#define OMAP44XX_UART3_BASE		(OMAP44XX_L4_PER_BASE + 0x20000)

/* I2C */
#define OMAP44XX_I2C1_BASE		(OMAP44XX_L4_PER_BASE + 0x070000)
#define OMAP44XX_I2C2_BASE		(OMAP44XX_L4_PER_BASE + 0x072000)
#define OMAP44XX_I2C3_BASE		(OMAP44XX_L4_PER_BASE + 0x060000)
#define OMAP44XX_I2C4_BASE		(OMAP44XX_L4_PER_BASE + 0x350000)

/* General Purpose Timers */
#define OMAP44XX_GPT1_BASE		(OMAP44XX_L4_WKUP_BASE + 0x18000)
#define OMAP44XX_GPT2_BASE		(OMAP44XX_L4_PER_BASE  + 0x32000)
#define OMAP44XX_GPT3_BASE		(OMAP44XX_L4_PER_BASE  + 0x34000)

/* Watchdog Timer2 - MPU watchdog */
#define OMAP44XX_WDT2_BASE		(OMAP44XX_L4_WKUP_BASE + 0x14000)

#define OMAP44XX_SCRM_BASE              0x4a30a000
#define OMAP44XX_SCRM_ALTCLKSRC         (OMAP44XX_SCRM_BASE + 0x110)
#define OMAP44XX_SCRM_AUXCLK1           (OMAP44XX_SCRM_BASE + 0x314)
#define OMAP44XX_SCRM_AUXCLK3           (OMAP44XX_SCRM_BASE + 0x31c)

/* 32KTIMER */
#define OMAP44XX_32KTIMER_BASE		(OMAP44XX_L4_WKUP_BASE + 0x4000)

/* MMC */
#define OMAP44XX_MMC1_BASE		(OMAP44XX_L4_PER_BASE + 0x09C000)
#define OMAP44XX_MMC2_BASE		(OMAP44XX_L4_PER_BASE + 0x0B4000)
#define OMAP44XX_MMC3_BASE		(OMAP44XX_L4_PER_BASE + 0x0AD000)
#define OMAP44XX_MMC4_BASE		(OMAP44XX_L4_PER_BASE + 0x0D1000)
#define OMAP44XX_MMC5_BASE		(OMAP44XX_L4_PER_BASE + 0x0D5000)

/* GPIO
 *
 *  Note that, while the GPIO controller is the same as on an OMAP3,
 * the base address has an additional offset of 0x100, which you can
 * see being added here so that the OMAP_GPIO_* macros you see in
 * mach-omap/gpio.c don't need to be adjusted based on the platform.
 */

#define OMAP44XX_GPIO1_BASE		(OMAP44XX_L4_WKUP_BASE + 0x10100)
#define OMAP44XX_GPIO2_BASE		(OMAP44XX_L4_PER_BASE  + 0x55100)
#define OMAP44XX_GPIO3_BASE		(OMAP44XX_L4_PER_BASE  + 0x57100)
#define OMAP44XX_GPIO4_BASE		(OMAP44XX_L4_PER_BASE  + 0x59100)
#define OMAP44XX_GPIO5_BASE		(OMAP44XX_L4_PER_BASE  + 0x5B100)
#define OMAP44XX_GPIO6_BASE		(OMAP44XX_L4_PER_BASE  + 0x5D100)

/* GPMC */
#define OMAP44XX_GPMC_BASE		0x50000000

/* EHCI */
#define OMAP44XX_EHCI_BASE		(OMAP44XX_L4_CORE_BASE + 0x64C00)

/* DMM */
#define OMAP44XX_DMM_BASE		0x4E000000
#define DMM_LISA_MAP_BASE		(OMAP44XX_DMM_BASE + 0x40)
#define DMM_LISA_MAP_SYS_SIZE_MASK	(7 << 20)
#define DMM_LISA_MAP_SYS_SIZE_SHIFT	20
#define DMM_LISA_MAP_SYS_ADDR_MASK	(0xFF << 24)

/* Memory Adapter (4460 onwards) */
#define OMAP44XX_MA_BASE		0x482AF000

/*
 * Hardware Register Details
 */

/* Watchdog Timer */
#define WD_UNLOCK1		0xAAAA
#define WD_UNLOCK2		0x5555

/* GP Timer */
#define TCLR_ST			(0x1 << 0)
#define TCLR_AR			(0x1 << 1)
#define TCLR_PRE		(0x1 << 5)

/*
 * PRCM
 */

/* PRM */
#define OMAP44XX_PRM_BASE		0x4A306000
#define OMAP44XX_PRM_DEVICE_BASE	(OMAP44XX_PRM_BASE + 0x1B00)

#define OMAP44XX_PRM_RSTCTRL		OMAP44XX_PRM_DEVICE_BASE
#define OMAP44XX_PRM_RSTCTRL_RESET	0x01

/*
 * SAR (Save & Rescue) memory region
 */
#define OMAP44XX_SAR_RAM_BASE      0x4a326000
#define OMAP44XX_SAR_CH_ADDRESS    (OMAP44XX_SAR_RAM_BASE + 0xA00)
#define OMAP44XX_SAR_CH_START      (OMAP44XX_SAR_RAM_BASE + 0xA0C)
#define OMAP44XX_SAR_BOOT_VOID     0x00
#define OMAP44XX_SAR_BOOT_XIP      0x01
#define OMAP44XX_SAR_BOOT_XIPWAIT  0x02
#define OMAP44XX_SAR_BOOT_NAND     0x03
#define OMAP44XX_SAR_BOOT_ONENAND  0x04
#define OMAP44XX_SAR_BOOT_MMC1     0x05
#define OMAP44XX_SAR_BOOT_MMC2_1   0x06
#define OMAP44XX_SAR_BOOT_MMC2_2   0x07
#define OMAP44XX_SAR_BOOT_UART     0x43
#define OMAP44XX_SAR_BOOT_USB_1    0x45
#define OMAP44XX_SAR_BOOT_USB_ULPI 0x46
#define OMAP44XX_SAR_BOOT_USB_2    0x47

/*
 * Non-secure SRAM Addresses
 * Non-secure RAM starts at 0x40300000 for GP devices. But we keep SRAM_BASE
 * at 0x40304000(EMU base) so that our code works for both EMU and GP
 */
#define NON_SECURE_SRAM_START	0x40304000
#define NON_SECURE_SRAM_END	0x4030E000	/* Not inclusive */
/* base address for indirect vectors (internal boot mode) */
#define SRAM_ROM_VECT_BASE	0x4030D000

/*
 * OMAP4 real hardware:
 * TODO: Change this to the IDCODE in the hw regsiter
 */
#define CPU_OMAP4430_ES10	1
#define CPU_OMAP4430_ES20	2

#define CM_DLL_CTRL		0x4a004110
#define CM_MEMIF_EMIF_1_CLKCTRL	0x4a008b30
#define CM_MEMIF_EMIF_2_CLKCTRL 0x4a008b38

/* Silicon revisions */
#define OMAP4430_SILICON_ID_INVALID	0
#define OMAP4430_ES1_0	1
#define OMAP4430_ES2_0	2
#define OMAP4430_ES2_1	3
#define OMAP4430_ES2_2	4
#define OMAP4430_ES2_3	5
#define OMAP4460_ES1_0	6
#define OMAP4460_ES1_1	7

#ifndef __ASSEMBLY__

struct ddr_regs {
	u32 tim1;
	u32 tim2;
	u32 tim3;
	u32 phy_ctrl_1;
	u32 ref_ctrl;
	u32 config_init;
	u32 config_final;
	u32 zq_config;
	u8 mr1;
	u8 mr2;
};

struct dpll_param;

void omap4_ddr_init(const struct ddr_regs *, const struct dpll_param *);
void omap4_power_i2c_send(u32);
unsigned int omap4_revision(void);
int omap4430_scale_vcores(void);
int omap4460_scale_vcores(unsigned vsel0_pin, unsigned volt_mv);
void omap4_set_warmboot_order(u32 *device_list);

#endif

#endif

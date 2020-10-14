/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 */

#ifndef __MACH_STM32_BOOTSOURCE_H__
#define __MACH_STM32_BOOTSOURCE_H__

enum stm32mp_boot_device {
	STM32MP_BOOT_FLASH_SD		= 0x10, /* .. 0x13 */
	STM32MP_BOOT_FLASH_EMMC		= 0x20, /* .. 0x23 */
	STM32MP_BOOT_FLASH_NAND		= 0x30,
	STM32MP_BOOT_FLASH_NAND_FMC	= 0x31,
	STM32MP_BOOT_FLASH_NOR		= 0x40,
	STM32MP_BOOT_FLASH_NOR_QSPI	= 0x41,
	STM32MP_BOOT_SERIAL_UART	= 0x50, /* .. 0x58 */
	STM32MP_BOOT_SERIAL_USB		= 0x60,
	STM32MP_BOOT_SERIAL_USB_OTG	= 0x62,
};

#endif

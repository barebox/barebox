/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/omap4-silicon.h>
#include <sizes.h>
#include <i2c/i2c.h>
#include <gpio.h>

static struct NS16550_plat serial_plat = {
	.clock = 48000000, /* 48MHz (APLL96/2) */
	.shift = 2,
};
static int archosg9_console_init(void){
	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_OMAP4_USBBOOT))
		add_generic_device("serial_omap4_usbboot", DEVICE_ID_DYNAMIC
			, NULL, 0, 0, 0, NULL);
	if (IS_ENABLED(CONFIG_DRIVER_SERIAL_NS16550)) {
		gpio_direction_output(41, 0); /* gps_disable */
		gpio_direction_output(34, 1); /* 1v8_pwron */
		add_ns16550_device(DEVICE_ID_DYNAMIC, OMAP44XX_UART1_BASE, 1024,
			IORESOURCE_MEM_8BIT, &serial_plat);
	}
	return 0;
}
console_initcall(archosg9_console_init);

static int archosg9_mem_init(void){
	arm_add_mem_device("ram0", 0x80000000, SZ_1G);
	return 0;
}
mem_initcall(archosg9_mem_init);

static struct i2c_board_info i2c_devices[] = {
	{ I2C_BOARD_INFO("twl6030", 0x48), },
};

static int archosg9_devices_init(void){
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	add_generic_device("i2c-omap"  , DEVICE_ID_DYNAMIC, NULL,
		OMAP44XX_I2C1_BASE, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("omap-hsmmc", DEVICE_ID_DYNAMIC, NULL,
		OMAP44XX_MMC1_BASE, SZ_4K, IORESOURCE_MEM, NULL);
	add_generic_device("omap-hsmmc", DEVICE_ID_DYNAMIC, NULL,
		OMAP44XX_MMC2_BASE, SZ_4K, IORESOURCE_MEM, NULL);

	armlinux_set_bootparams((void *)0x80000100);
	/*
	 * This should be:
	 * armlinux_set_architecture(MACH_TYPE_OMAP4_ARCHOSG9);
	 * But Archos has not registered it's board to arch/arm/tools/mach-types
	 * So here there is the hardcoded value
	 */
	armlinux_set_architecture(5032);

	return 0;
}
device_initcall(archosg9_devices_init);

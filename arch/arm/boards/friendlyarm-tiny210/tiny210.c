// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) 2012 Alexey Galakhov
 * Based on Mini6410 code by Juergen Beisert
 *
 * Copyright (C) 2012 Juergen Beisert, Pengutronix
 *
 * In some ways inspired by code
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/sizes.h>
#include <generated/mach-types.h>
#include <gpio.h>
#include <led.h>
#include <io.h>
#include <nand.h>
#include <asm/armlinux.h>
#include <mach/iomux.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-clocks.h>
#include <mach/s3c-generic.h>


static const unsigned pin_usage[] = {
	/* TODO */
};

static struct gpio_led leds[] = {
	{
		.gpio = GPJ20,
		.led = {
			.name = "led1",
		}
	}, {
		.gpio = GPJ21,
		.led = {
			.name = "led2",
		}
	}, {
		.gpio = GPJ22,
		.led = {
			.name = "led3",
		}
	}, {
		.gpio = GPJ23,
		.led = {
			.name = "led4",
		}
	}
};

static int tiny210_mem_init(void)
{
	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s5p_get_memory_size());
	return 0;
}
mem_initcall(tiny210_mem_init);

static int tiny210_console_init(void)
{
	/*
	 * configure the UART1 right now, as barebox will
	 * start to send data immediately
	 */
	s3c_gpio_mode(GPA00_RXD0 | ENABLE_PU);
	s3c_gpio_mode(GPA01_TXD0);
	s3c_gpio_mode(GPA02_NCTS0 | ENABLE_PU);
	s3c_gpio_mode(GPA03_NRTS0);

	barebox_set_model("Friendlyarm tiny210");
	barebox_set_hostname("tiny210");

	add_generic_device("s3c_serial", DEVICE_ID_DYNAMIC, NULL,
			   S3C_UART1_BASE, S3C_UART1_SIZE,
			   IORESOURCE_MEM, NULL);
	return 0;
}
console_initcall(tiny210_console_init);

static int tiny210_devices_init(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pin_usage); i++)
		s3c_gpio_mode(pin_usage[i]);

	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		leds[i].active_low = 1;
		gpio_direction_output(leds[i].gpio, leds[i].active_low);
		led_gpio_register(&leds[i]);
	}

	led_set_trigger(LED_TRIGGER_HEARTBEAT, &leds[0].led);

	armlinux_set_architecture(MACH_TYPE_MINI210);

	return 0;
}
device_initcall(tiny210_devices_init);

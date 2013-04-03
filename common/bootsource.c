/*
 *
 * Copyright (C) 2011 Marc Reilly <marc@cpdesign.com.au>
 * Copyright (C) 2013 Marc Kleine-Budde <mkl@pengutronix.de>
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
#include <bootsource.h>
#include <environment.h>
#include <magicvar.h>
#include <init.h>

static const char *bootsource_str[] = {
	[BOOTSOURCE_UNKNOWN] = "unknown",
	[BOOTSOURCE_NAND] = "nand",
	[BOOTSOURCE_NOR] = "nor",
	[BOOTSOURCE_MMC] = "mmc",
	[BOOTSOURCE_I2C] = "i2c",
	[BOOTSOURCE_I2C_EEPROM] = "i2c-eeprom",
	[BOOTSOURCE_SPI] = "spi",
	[BOOTSOURCE_SPI_EEPROM] = "spi-eeprom",
	[BOOTSOURCE_SPI_NOR] = "spi-nor",
	[BOOTSOURCE_SERIAL] = "serial",
	[BOOTSOURCE_ONENAND] = "onenand",
	[BOOTSOURCE_HD] = "harddisk",
	[BOOTSOURCE_USB] = "usb",
};

static enum bootsource bootsource = BOOTSOURCE_UNKNOWN;
static int bootsource_instance = BOOTSOURCE_INSTANCE_UNKNOWN;

void bootsource_set(enum bootsource src)
{
	if (src >= ARRAY_SIZE(bootsource_str))
		src = BOOTSOURCE_UNKNOWN;

	bootsource = src;

	setenv("bootsource", bootsource_str[src]);
}

void bootsource_set_instance(int instance)
{
	char buf[32];

	bootsource_instance = instance;

	if (instance < 0)
		sprintf(buf, "unknown");
	else
		snprintf(buf, sizeof(buf), "%d", instance);

	setenv("bootsource_instance", buf);
}

enum bootsource bootsource_get(void)
{
	return bootsource;
}

BAREBOX_MAGICVAR(bootsource, "The source barebox has been booted from");

int bootsource_get_instance(void)
{
	return bootsource_instance;
}

BAREBOX_MAGICVAR(bootsource_instance, "The instance of the source barebox has been booted from");

static int bootsource_init(void)
{
	bootsource_set(bootsource);
	bootsource_set_instance(bootsource_instance);
	export("bootsource");
	export("bootsource_instance");

	return 0;
}
coredevice_initcall(bootsource_init);

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
	[BOOTSOURCE_NET] = "net",
	[BOOTSOURCE_CAN] = "can",
	[BOOTSOURCE_JTAG] = "jtag",
};

static enum bootsource bootsource = BOOTSOURCE_UNKNOWN;
static int bootsource_instance = BOOTSOURCE_INSTANCE_UNKNOWN;
const char *bootsource_alias_name = NULL;

/**
 * bootsource_get_alias_name() - Get the name of the bootsource alias
 *
 * This function will return newly allocated string containing name of
 * the alias that is expected to point to DTB node corresponding to
 * detected bootsource
 *
 * NOTE: Caller is expected to free() the string allocated by this
 * function
 */
char *bootsource_get_alias_name(void)
{
	const char *stem;

	/*
	 * If alias name was overridden via
	 * bootsource_set_alias_name() return that value without
	 * asking any questions.
	 *
	 * Note that we have to strdup() the result to make it
	 * free-able.
	 */
	if (bootsource_alias_name)
		return strdup(bootsource_alias_name);

	switch (bootsource) {
		/*
		 * For I2C and SPI EEPROMs we set the stem to be 'i2c'
		 * and 'spi' correspondingly. The resulting alias will
		 * be pointing at the controller said EEPROM is
		 * attached to.
		 *
		 * NOTE: This code assumes single bootable EEPROM per
		 * controller
		 */
	case BOOTSOURCE_I2C_EEPROM:
		stem = bootsource_str[BOOTSOURCE_I2C];
		break;
	case BOOTSOURCE_SPI_EEPROM:
	case BOOTSOURCE_SPI_NOR:
		stem = bootsource_str[BOOTSOURCE_SPI];
		break;
	case BOOTSOURCE_SERIAL:	/* FALLTHROUGH */
	case BOOTSOURCE_I2C:	/* FALLTHROUGH */
	case BOOTSOURCE_MMC:	/* FALLTHROUGH */
	case BOOTSOURCE_SPI:	/* FALLTHROUGH */
	case BOOTSOURCE_CAN:
		stem = bootsource_str[bootsource];
		break;
	default:
		return NULL;
	}

	/*
	 * We expect SoC specific bootsource detection code to properly
	 * initialize bootsource_instance, so we bail out if it didn't
	 */
	if (bootsource_instance == BOOTSOURCE_INSTANCE_UNKNOWN)
		return NULL;

	return basprintf("%s%d", stem, bootsource_instance);
}

void bootsource_set_alias_name(const char *name)
{
	bootsource_alias_name = name;
}

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

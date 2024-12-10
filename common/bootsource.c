// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011 Marc Reilly <marc@cpdesign.com.au>
 * Copyright (C) 2013 Marc Kleine-Budde <mkl@pengutronix.de>
 */

#include <common.h>
#include <bootsource.h>
#include <environment.h>
#include <magicvar.h>
#include <init.h>

static const char *bootsource_str[BOOTSOURCE_MAX] = {
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

const char *bootsource_to_string(enum bootsource src)
{
	if (src >= ARRAY_SIZE(bootsource_str))
		return NULL;

	return bootsource_str[src];
}

const char *bootsource_get_alias_stem(enum bootsource src)
{
	switch (src) {
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
		return bootsource_str[BOOTSOURCE_I2C];
	case BOOTSOURCE_SPI_EEPROM:
	case BOOTSOURCE_SPI_NOR:
		return bootsource_str[BOOTSOURCE_SPI];
	case BOOTSOURCE_SERIAL:	/* FALLTHROUGH */
	case BOOTSOURCE_I2C:	/* FALLTHROUGH */
	case BOOTSOURCE_MMC:	/* FALLTHROUGH */
	case BOOTSOURCE_SPI:	/* FALLTHROUGH */
	case BOOTSOURCE_CAN:
		return bootsource_str[src];
	default:
		return NULL;
	}
}

/**
 * bootsource_get_alias_name() - Get the name of the bootsource alias
 *
 * This function will return a pointer to a static string containing name of
 * the alias that is expected to point to DTB node corresponding to
 * detected bootsource
 *
 */
const char *bootsource_get_alias_name(void)
{
	static char buf[sizeof("i2c-eeprom-2147483647")];
	const char *stem;
	int ret;

	/*
	 * If alias name was overridden via
	 * bootsource_set_alias_name() return that value without
	 * asking any questions.
	 */
	if (bootsource_alias_name)
		return bootsource_alias_name;

	stem = bootsource_get_alias_stem(bootsource);
	if (!stem)
		return NULL;

	/*
	 * We expect SoC specific bootsource detection code to properly
	 * initialize bootsource_instance, so we bail out if it didn't
	 */
	if (bootsource_instance == BOOTSOURCE_INSTANCE_UNKNOWN)
		return NULL;

	ret = snprintf(buf, sizeof(buf), "%s%d", stem, bootsource_instance);
	if (ret < 0 || ret >= sizeof(buf))
		return NULL;

	return buf;
}

struct device_node *bootsource_of_node_get(struct device_node *root)
{
	struct device_node *np;
	const char *alias_name;

	alias_name = bootsource_get_alias_name();
	if (!alias_name)
		return NULL;

	np = of_find_node_by_alias(root, alias_name);

	return np;
}

void bootsource_set_alias_name(const char *name)
{
	bootsource_alias_name = name;
}

void bootsource_set_raw(enum bootsource src, int instance)
{
	if (src >= BOOTSOURCE_MAX)
		src = BOOTSOURCE_UNKNOWN;

	bootsource = src;

	setenv("bootsource", bootsource_to_string(src));

	bootsource_set_raw_instance(instance);
}

void bootsource_set_raw_instance(int instance)
{
	bootsource_instance = instance;

	if (instance < 0)
		setenv("bootsource_instance","unknown");
	else
		pr_setenv("bootsource_instance", "%d", instance);
}

int bootsource_of_alias_xlate(enum bootsource src, int instance)
{
	char chosen[sizeof("barebox,bootsource-harddisk4294967295")];
	const char *bootsource_stem;
	struct device_node *np;
	int alias_id;

	if (!IS_ENABLED(CONFIG_OFDEVICE) || IN_PBL)
		return BOOTSOURCE_INSTANCE_UNKNOWN;

	if (src == BOOTSOURCE_UNKNOWN ||
	    instance == BOOTSOURCE_INSTANCE_UNKNOWN)
		return BOOTSOURCE_INSTANCE_UNKNOWN;

	bootsource_stem = bootsource_get_alias_stem(src);
	if (!bootsource_stem)
		return BOOTSOURCE_INSTANCE_UNKNOWN;

	scnprintf(chosen, sizeof(chosen), "barebox,bootsource-%s%u",
		  bootsource_stem, instance);

	np = of_find_node_by_chosen(chosen, NULL);
	if (!np)
		return BOOTSOURCE_INSTANCE_UNKNOWN;

	alias_id = of_alias_get_id(np, bootsource_stem);
	if (alias_id < 0)
		return BOOTSOURCE_INSTANCE_UNKNOWN;

	return alias_id;
}

int bootsource_set(enum bootsource src, int instance)
{
	int alias_id;

	alias_id = bootsource_of_alias_xlate(src, instance);
	if (alias_id == BOOTSOURCE_INSTANCE_UNKNOWN)
		alias_id = instance;

	bootsource_set_raw(src, alias_id);

	return alias_id;
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
	bootsource_set_raw(bootsource, bootsource_instance);
	export("bootsource");
	export("bootsource_instance");

	return 0;
}
coredevice_initcall(bootsource_init);

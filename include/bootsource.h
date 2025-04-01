/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTSOURCE_H__
#define __BOOTSOURCE_H__

enum bootsource {
	BOOTSOURCE_UNKNOWN,
	BOOTSOURCE_NAND,
	BOOTSOURCE_NOR,
	BOOTSOURCE_MMC,
	BOOTSOURCE_I2C,
	BOOTSOURCE_I2C_EEPROM,
	BOOTSOURCE_SPI,
	BOOTSOURCE_SPI_EEPROM,
	BOOTSOURCE_SPI_NOR,
	BOOTSOURCE_SPI_NAND,
	BOOTSOURCE_SERIAL,
	BOOTSOURCE_ONENAND,
	BOOTSOURCE_HD,
	BOOTSOURCE_USB,
	BOOTSOURCE_NET,
	BOOTSOURCE_CAN,
	BOOTSOURCE_JTAG,
	BOOTSOURCE_MAX,
};

#define BOOTSOURCE_INSTANCE_UNKNOWN	-1

enum bootsource bootsource_get(void);
enum bootsource bootsource_get_device(void);
int bootsource_get_instance(void);
void bootsource_set_alias_name(const char *name);
const char *bootsource_get_alias_name(void);
const char *bootsource_to_string(enum bootsource src);
const char *bootsource_get_alias_stem(enum bootsource bs);
int bootsource_of_alias_xlate(enum bootsource bs, int instance);
struct device_node *bootsource_of_node_get(struct device_node *root);
struct cdev *bootsource_of_cdev_find(void);

/**
 * bootsource_set - set bootsource with optional DT mapping table
 * @bs:	bootrom reported bootsource
 * @instance: bootrom reported instance
 *
 * Returns computed bootsource instace
 *
 * Normal bootsource_set_raw_instance() expects numbering used by
 * bootrom for instance to align with DT aliases, e.g.
 * $bootsource = "mmc" && $bootsource_instance = 1 -> /aliases/mmc1
 * bootsource_set() will instead consult
 * /aliases/barebox,bootsource-mmc1 which may point at a different
 * device than mmc1. In absence of appropriate barebox,bootsource-*
 * chosen property, instance is set without translation.
 */
int bootsource_set(enum bootsource bs, int instance);

/**
 * bootsource_set_raw - set bootsource as-is
 * @bs:	bootsource to report
 * @instance: bootsource instance to report
 *
 * This sets bootsource and bootsource_instance directly.
 * Preferably, use bootsource_set in new code.
 */
void bootsource_set_raw(enum bootsource src, int instance);

/**
 * bootsource_set_raw_instance - set bootsource_instance as-is
 * @bs:	bootrom reported bootsource
 * @instance: bootrom reported instance
 *
 * This directly sets bootsource_instance without changing bootsource.
 */
void bootsource_set_raw_instance(int instance);

#endif	/* __BOOTSOURCE_H__ */

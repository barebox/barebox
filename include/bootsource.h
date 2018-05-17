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
	BOOTSOURCE_SERIAL,
	BOOTSOURCE_ONENAND,
	BOOTSOURCE_HD,
	BOOTSOURCE_USB,
	BOOTSOURCE_NET,
	BOOTSOURCE_CAN,
};

#define BOOTSOURCE_INSTANCE_UNKNOWN	-1

enum bootsource bootsource_get(void);
int bootsource_get_instance(void);
void bootsource_set(enum bootsource src);
void bootsource_set_instance(int instance);
void bootsource_set_alias_name(const char *name);
char *bootsource_get_alias_name(void);

#endif	/* __BOOTSOURCE_H__ */

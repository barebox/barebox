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
};

#define BOOTSOURCE_INSTANCE_UNKNOWN	-1

enum bootsource bootsource_get(void);
int bootsource_get_instance(void);
void bootsource_set(enum bootsource src);
void bootsource_set_instance(int instance);

#endif	/* __BOOTSOURCE_H__ */

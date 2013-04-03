#ifndef __BOOTSOURCE_H__
#define __BOOTSOURCE_H__

enum bootsource {
	BOOTSOURCE_UNKNOWN,
	BOOTSOURCE_NAND,
	BOOTSOURCE_NOR,
	BOOTSOURCE_MMC,
	BOOTSOURCE_I2C,
	BOOTSOURCE_SPI,
	BOOTSOURCE_SERIAL,
	BOOTSOURCE_ONENAND,
	BOOTSOURCE_HD,
};

enum bootsource bootsource_get(void);
void bootsource_set(enum bootsource src);

#endif	/* __BOOTSOURCE_H__ */

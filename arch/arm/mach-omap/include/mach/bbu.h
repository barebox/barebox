#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE_AM33XX_SPI_NOR_MLO
int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile);
int am33xx_bbu_spi_nor_register_handler(const char *name, char *devicefile);
#else
static inline int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile)
{
	return 0;
}

static inline int am33xx_bbu_spi_nor_register_handler(const char *name, char *devicefile)
{
	return 0;
}
#endif

#ifdef CONFIG_BAREBOX_UPDATE_AM33XX_NAND_XLOADSLOTS
int am33xx_bbu_nand_xloadslots_register_handler(const char *name,
						char **devicefile,
						int num_devicefiles);
#else
static inline int am33xx_bbu_nand_xloadslots_register_handler(const char *name,
							char **devicefile,
							int num_devicefiles)
{
	return 0;
}
#endif

#endif

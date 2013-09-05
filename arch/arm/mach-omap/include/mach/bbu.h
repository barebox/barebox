#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE_AM33XX_SPI_NOR_MLO
int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile);
#else
int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile)
{
	return 0;
}
#endif

#endif

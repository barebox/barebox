#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE_AM33XX_SPI_NOR_MLO
int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile);
#else
static inline int am33xx_bbu_spi_nor_mlo_register_handler(const char *name, char *devicefile)
{
	return 0;
}
#endif

static inline int am33xx_bbu_spi_nor_register_handler(const char *name, char *devicefile)
{
	return bbu_register_std_file_update(name, 0, devicefile, filetype_arm_barebox);
}

#ifdef CONFIG_BAREBOX_UPDATE_AM33XX_NAND
int am33xx_bbu_nand_xloadslots_register_handler(const char *name,
						char **devicefile,
						int num_devicefiles);
int am33xx_bbu_nand_register_handler(const char *name, char *devicefile);
#else
static inline int am33xx_bbu_nand_xloadslots_register_handler(const char *name,
							char **devicefile,
							int num_devicefiles)
{
	return 0;
}

static inline int am33xx_bbu_nand_register_handler(const char *name, char *devicefile)
{
	return 0;
}
#endif

#ifdef CONFIG_BAREBOX_UPDATE_AM33XX_EMMC
int am33xx_bbu_emmc_mlo_register_handler(const char *name, char *devicefile);
int am33xx_bbu_emmc_register_handler(const char *name, char *devicefile);
#else
static inline int am33xx_bbu_emmc_mlo_register_handler(const char *name,
							char *devicefile)
{
	return 0;
}

static inline int am33xx_bbu_emmc_register_handler(const char *name,
							char *devicefile)
{
	return 0;
}
#endif


#endif

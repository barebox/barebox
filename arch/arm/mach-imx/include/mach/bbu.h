#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>
#include <errno.h>

struct imx_dcd_entry;
struct imx_dcd_v2_entry;

#ifdef CONFIG_BAREBOX_UPDATE

int imx51_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags);

int imx53_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags);

int imx53_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags);

int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, int partition_size);

int imx6_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags);

int imx6_bbu_internal_mmcboot_register_handler(const char *name, char *devicefile,
		unsigned long flags);

int imx6_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags);

int imx_bbu_external_nor_register_handler(const char *name, char *devicefile,
		unsigned long flags);

#else

static inline int imx51_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, int partition_size)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_mmcboot_register_handler(const char *name,
							     char *devicefile,
							     unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx_bbu_external_nor_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}
#endif

#if defined(CONFIG_BAREBOX_UPDATE_IMX_EXTERNAL_NAND)
int imx_bbu_external_nand_register_handler(const char *name, char *devicefile,
		unsigned long flags);
#else
static inline int imx_bbu_external_nand_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}
#endif

#endif

#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>

struct imx_dcd_entry;
struct imx_dcd_v2_entry;

#ifdef CONFIG_BAREBOX_UPDATE

int imx51_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_entry *, int dcdsize,
		unsigned long app_dest);

int imx53_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *, int dcdsize,
		unsigned long app_dest);

int imx53_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest);

int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, struct imx_dcd_v2_entry *, int dcdsize,
		int partition_size, unsigned long app_dest);

int imx6_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *, int dcdsize,
		unsigned long app_dest);

int imx6_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest);

#else

static inline int imx51_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		int partition_size, unsigned long app_dest)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_mmc_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
		unsigned long flags, struct imx_dcd_v2_entry *dcd, int dcdsize,
		unsigned long app_dest)
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

struct dcd_table {
	void *data;
	unsigned int size;
};

void *imx53_bbu_internal_concat_dcd_table(struct dcd_table *table, int num_entries);

#endif

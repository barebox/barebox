#ifndef __MACH_BBU_H
#define __MACH_BBU_H

#include <bbu.h>
#include <errno.h>

struct imx_dcd_entry;
struct imx_dcd_v2_entry;

/*
 * The ROM code reads images from a certain offset of the boot device
 * (usually 0x400), whereas the update images start from offset 0x0.
 * Set this flag to skip the offset on both the update image and the
 * device so that the initial boot device portion is preserved. This
 * is useful if a partition table or other data is in this area.
 */
#define IMX_BBU_FLAG_KEEP_HEAD	BIT(16)

/*
 * Set this flag when the partition the update image is written to
 * actually starts at the offset where the i.MX flash header is expected
 * (usually 0x400). This means for the update code that it has to skip
 * the first 0x400 bytes of the image.
 */
#define IMX_BBU_FLAG_PARTITION_STARTS_AT_HEADER	(1 << 17)

/*
 * The upper 16 bit of the flags passes to the below functions are reserved
 * for i.MX specific flags
 */
#define IMX_BBU_FLAG_MASK	0xffff0000

#ifdef CONFIG_BAREBOX_UPDATE

int imx51_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int imx51_bbu_internal_spi_i2c_register_handler(const char *name,
		const char *devicefile, unsigned long flags);

int imx53_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int imx53_bbu_internal_spi_i2c_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, int partition_size);

int imx6_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int imx6_bbu_internal_mmcboot_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int imx6_bbu_internal_spi_i2c_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int vf610_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

int vf610_bbu_internal_spi_i2c_register_handler(const char *name, const char *devicefile,
						unsigned long flags);

int imx8mq_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
					     unsigned long flags);

int imx_bbu_external_nor_register_handler(const char *name, const char *devicefile,
		unsigned long flags);

#else

static inline int imx51_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx51_bbu_internal_spi_i2c_register_handler(const char *name,
		const char *devicefile, unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_spi_i2c_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx53_bbu_internal_nand_register_handler(const char *name,
		unsigned long flags, int partition_size)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_mmcboot_register_handler(const char *name,
							     const char *devicefile,
							     unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx6_bbu_internal_spi_i2c_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int vf610_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
					    unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx8mq_bbu_internal_mmc_register_handler(const char *name, const char *devicefile,
							   unsigned long flags)
{
	return -ENOSYS;
}

static inline int imx_bbu_external_nor_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}

static inline int
vf610_bbu_internal_spi_i2c_register_handler(const char *name, char *devicefile,
					    unsigned long flags)
{
	return -ENOSYS;
}

#endif

#if defined(CONFIG_BAREBOX_UPDATE_IMX_EXTERNAL_NAND)
int imx_bbu_external_nand_register_handler(const char *name, const char *devicefile,
		unsigned long flags);
#else
static inline int imx_bbu_external_nand_register_handler(const char *name, const char *devicefile,
		unsigned long flags)
{
	return -ENOSYS;
}
#endif

#endif

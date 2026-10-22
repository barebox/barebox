// SPDX-License-Identifier: GPL-2.0-only

#include <io.h>
#include <linux/array_size.h>
#include <linux/mtd/spi-nor.h>
#include <linux/printk.h>
#include <linux/time.h>
#include <pbl/bio.h>
#include <pbl/mtd.h>

#include "cadence-quadspi.h"

static void cqspi_write(struct spi_nor *nor, loff_t to,
			size_t len, size_t *retlen, const u_char *buf)
{
}

static int cqspi_get_flash_pdata(struct cqspi_st *cqspi,
				 struct cqspi_flash_pdata *f_pdata)
{
	void __iomem *reg_base = cqspi->iobase;

	f_pdata->block_size = (readl(reg_base + CQSPI_REG_SIZE) >>
		CQSPI_REG_SIZE_BLOCK_LSB) &
		CQSPI_REG_SIZE_BLOCK_MASK;
	cqspi->no_reconfig = true;

	return 0;
}

static int cqspi_setup_flash(struct cqspi_st *cqspi,
			     struct cqspi_flash_pdata *f_pdata)
{
	const struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_READ_1_1_2 |
			SNOR_HWCAPS_READ_1_1_4 |
			SNOR_HWCAPS_PP,
	};
	struct spi_nor *nor;
	struct mtd_info *mtd;
	int ret;

	ret = cqspi_get_flash_pdata(cqspi, f_pdata);
	if (ret)
		return ret;

	nor = &f_pdata->nor;
	mtd = &f_pdata->mtd;

	nor->mtd = mtd;

	nor->priv = cqspi;
	nor->read_reg = cqspi_read_reg;
	nor->write_reg = cqspi_write_reg;
	nor->read = cqspi_read;
	nor->write = cqspi_write;

	return spi_nor_scan(nor, NULL, &hwcaps, false);
}

static int cqspi_bio_read(struct pbl_bio *bio, off_t start,
			  void *buf, unsigned int nblocks)
{
	struct cqspi_st *cqspi = bio->priv;
	struct spi_nor *nor = &cqspi->f_pdata[0].nor;
	int ret;
	size_t count = 0;

	ret = nor->read(nor, start, nblocks, &count, buf);
	if (ret)
		return ret;

	return count;
}

static struct cqspi_st cqspi;

int cqspi_bio_init(struct pbl_bio *bio,
		   void __iomem *iobase, void __iomem *ahb_base,
		   unsigned int master_ref_clk,
		   struct cadence_qspi_platform_data *qspi_pdata)
{
	cqspi.master_ref_clk_hz = master_ref_clk;
	cqspi.iobase = iobase;
	cqspi.ahb_base = ahb_base;

	cqspi_wait_idle(&cqspi);
	cqspi_controller_init(&cqspi);

	cqspi.current_cs = -1;
	cqspi.sclk = 0;

	cqspi.is_decoded_cs = qspi_pdata->is_decoded_cs;
	cqspi.fifo_depth = qspi_pdata->fifo_depth;

	cqspi_setup_flash(&cqspi, &cqspi.f_pdata[0]);

	bio->priv = &cqspi;
	bio->read = cqspi_bio_read;

	return 0;
}

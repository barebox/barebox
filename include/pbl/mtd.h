/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PBL_MTD_H__
#define __PBL_MTD_H__

#include <linux/types.h>

struct pbl_bio;

#include <platform_data/cadence_qspi.h>

#ifdef CONFIG_SPI_CADENCE_QUADSPI_PBL
int cqspi_bio_init(struct pbl_bio *bio,
		   void __iomem *iobase, void __iomem *ahb_base,
		   unsigned int master_ref_clk,
		   struct cadence_qspi_platform_data *qspi_pdata);
#else
static inline int cqspi_bio_init(struct pbl_bio *bio,
				 void __iomem *iobase, void __iomem *ahb_base,
				 unsigned int master_ref_clk,
				 struct cadence_qspi_platform_data *qspi_pdata)
{
	return -ENOSYS;
}
#endif

#endif /* __PBL_MTD_H__ */

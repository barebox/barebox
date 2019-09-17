/*
 * Copyright (C) 2012 Juergen Beisert
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>

#define S3C_DRAMC_CHIP_0_CFG (S3C_DRAMC + 0x200)

/* note: this routine honors the first memory bank only */
unsigned s3c6410_get_memory_size(void)
{
	unsigned reg = readl(S3C_DRAMC_CHIP_0_CFG) & 0xff;

	return ~(reg << 24) + 1;
}

/* configure the timing of one of the available external chip select lines */
int s3c6410_setup_chipselect(int no, const struct s3c6410_chipselect *c)
{
	unsigned per_t = 1000000000 / s3c_get_hclk();
	unsigned tacs, tcos, tacc, tcoh, tcah, shift;
	uint32_t reg;

	/* start of cycle to chip select assertion (= address/data setup) */
	tacs = DIV_ROUND_UP(c->adr_setup_t, per_t);
	/* start of CS to read/write assertion (= access setup) */
	tcos = DIV_ROUND_UP(c->access_setup_t, per_t);
	/* length of read/write assertion (= access length) */
	tacc = DIV_ROUND_UP(c->access_t, per_t) - 1;
	/* CS hold after access is finished */
	tcoh = DIV_ROUND_UP(c->cs_hold_t, per_t);
	/* adress/data hold after CS is deasserted */
	tcah = DIV_ROUND_UP(c->adr_hold_t, per_t);

	shift = no * 4;
	reg = readl(S3C_SROM_BW) & ~(0xf << shift);
	if (c->width == 16)
		reg |= 0x1 << shift;
	writel(reg, S3C_SROM_BW);
#ifdef DEBUG
	if (tacs > 15 || tcos > 15 || tacc > 31 || tcoh > 15 || tcah > 15) {
		pr_err("At least one of the timings are invalid\n");
		return -EINVAL;
	}
	pr_info("Will write 0x%08X\n", tacs << 28 | tcos << 24 | tacc << 16 |
					tcoh << 12 | tcah << 8);
#endif
	writel(tacs << 28 | tcos << 24 | tacc << 16 | tcoh << 12 | tcah << 8,
						S3C_SROM_BC0 + shift);

	return 0;
}

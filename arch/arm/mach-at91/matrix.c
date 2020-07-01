/* SPDX-License-Identifier: BSD-1-Clause */
/*
 * Copyright (c) 2013, Atmel Corporation
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 */

#include <io.h>
#include <mach/tz_matrix.h>
#include <mach/matrix.h>

static inline void matrix_write(void __iomem *base,
				unsigned int offset,
				const unsigned int value)
{
	writel(value, base + offset);
}

static inline unsigned int matrix_read(void __iomem *base, unsigned int offset)
{
	return readl(base + offset);
}

void at91_matrix_write_protect_enable(void __iomem *matrix_base)
{
	matrix_write(matrix_base, MATRIX_WPMR,
		     MATRIX_WPMR_WPKEY_PASSWD | MATRIX_WPMR_WPEN_ENABLE);
}

void at91_matrix_write_protect_disable(void __iomem *matrix_base)
{
	matrix_write(matrix_base, MATRIX_WPMR, MATRIX_WPMR_WPKEY_PASSWD);
}

void at91_matrix_configure_slave_security(void __iomem *matrix_base,
					  unsigned int slave,
					  unsigned int srtop_setting,
					  unsigned int srsplit_setting,
					  unsigned int ssr_setting)
{
	matrix_write(matrix_base, MATRIX_SSR(slave), ssr_setting);
	matrix_write(matrix_base, MATRIX_SRTSR(slave), srtop_setting);
	matrix_write(matrix_base, MATRIX_SASSR(slave), srsplit_setting);
}

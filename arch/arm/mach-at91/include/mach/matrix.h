/* SPDX-License-Identifier: BSD-1-Clause */
/*
 * Copyright (c) 2013, Atmel Corporation
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 */
#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <linux/compiler.h>

void at91_matrix_write_protect_enable(void __iomem *matrix_base);
void at91_matrix_write_protect_disable(void __iomem *matrix_base);
void at91_matrix_configure_slave_security(void __iomem *matrix_base,
					  unsigned int slave,
					  unsigned int srtop_setting,
					  unsigned int srsplit_setting,
					  unsigned int ssr_setting);

#endif /* #ifndef __MATRIX_H__ */

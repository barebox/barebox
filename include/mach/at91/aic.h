/* SPDX-License-Identifier: BSD-1-Clause */
#ifndef __AT91_AIC_H_
#define __AT91_AIC_H_

#include <linux/compiler.h>

void at91_aic_redir(void __iomem *sfr, u32 key);

#endif

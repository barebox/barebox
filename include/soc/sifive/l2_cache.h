/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SOC_SIFIVE_L2_CACHE_H_
#define __SOC_SIFIVE_L2_CACHE_H_

#include <linux/types.h>

void sifive_l2_flush64_range(dma_addr_t start, dma_addr_t end);

#endif

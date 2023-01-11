// SPDX-License-Identifier: BSD-3-Clause
#ifndef __SOC_FSL_CAAM_H_
#define __SOC_FSL_CAAM_H_

#include <linux/compiler.h>
#include <linux/types.h>

struct caam_ctrl;

int early_caam_init(struct caam_ctrl __iomem *caam, bool is_imx);

static inline int imx_early_caam_init(struct caam_ctrl __iomem *caam)
{
	return early_caam_init(caam, true);
}

#endif

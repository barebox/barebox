// SPDX-License-Identifier: GPL-2.0-or-later
//
#ifndef __CAAM_DETECT_H__
#define __CAAM_DETECT_H__

#include "regs.h"

static inline int caam_is_64bit(struct caam_ctrl __iomem *ctrl)
{
	return	(rd_reg32(&ctrl->perfmon.comp_parms_ms) & CTPR_MS_PS) &&
		(rd_reg32(&ctrl->mcr) & MCFGR_LONG_PTR);
}

static inline bool caam_is_big_endian(struct caam_ctrl *ctrl)
{
	return rd_reg32(&ctrl->perfmon.status) & (CSTA_PLEND | CSTA_ALT_PLEND);
}

#endif

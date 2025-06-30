/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IMX_TZASC_H__
#define __IMX_TZASC_H__

#include <linux/types.h>
#include <asm/system.h>

void imx6q_tzc380_early_ns_region1(void);
void imx6ul_tzc380_early_ns_region1(void);
void imx8m_tzc380_init(void);
bool imx8m_tzc380_is_enabled(void);

#endif

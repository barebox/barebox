/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IMX_TZASC_H__
#define __IMX_TZASC_H__

#include <linux/types.h>
#include <asm/system.h>

void imx8m_tzc380_init(void);
bool imx8m_tzc380_is_enabled(void);

#endif

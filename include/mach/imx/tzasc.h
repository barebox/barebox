/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IMX_TZASC_H__
#define __IMX_TZASC_H__

#include <linux/types.h>
#include <asm/system.h>

void imx8mq_tzc380_init(void);
void imx8mm_tzc380_init(void);
void imx8mn_tzc380_init(void);
void imx8mp_tzc380_init(void);

bool tzc380_is_enabled(void);

#endif

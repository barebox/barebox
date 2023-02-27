/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Michael Tretter <m.tretter@pengutronix.de>
 */
#ifndef __MACH_ZYNQMP_BBU_H
#define __MACH_ZYNQMP_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE
int zynqmp_bbu_register_handler(const char *name, char *devicefile,
				unsigned long flags);
#else
static int zynqmp_bbu_register_handler(const char *name, char *devicefile,
				       unsigned long flags)
{
	return 0;
};
#endif

#endif /* __MACH_ZYNQMP_BBU_H */

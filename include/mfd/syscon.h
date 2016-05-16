/* System Control Driver
 *
 * Based on linux driver by:
 *  Copyright (C) 2012 Freescale Semiconductor, Inc.
 *  Copyright (C) 2012 Linaro Ltd.
 *  Author: Dong Aisheng <dong.aisheng@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFD_SYSCON_H__
#define __MFD_SYSCON_H__

#ifdef CONFIG_MFD_SYSCON
void __iomem *syscon_base_lookup_by_pdevname(const char *s);
void __iomem *syscon_base_lookup_by_phandle
	(struct device_node *np, const char *property);
#else
static inline void __iomem *syscon_base_lookup_by_pdevname(const char *s)
{
	return ERR_PTR(-ENOSYS);
}

static inline void __iomem *syscon_base_lookup_by_phandle
	(struct device_node *np, const char *property)
{
	return ERR_PTR(-ENOSYS);
}
#endif

#endif

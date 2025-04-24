/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: (C) 2012 Freescale Semiconductor, Inc. */
/* SPDX-FileCopyrightText: (C) (C) 2012 Linaro Ltd. */
/* System Control Driver
 *
 * Based on linux driver by:
 *  Author: Dong Aisheng <dong.aisheng@linaro.org>
 */

#ifndef __MFD_SYSCON_H__
#define __MFD_SYSCON_H__

#include <linux/compiler.h>
#include <linux/err.h>

struct regmap;
struct device_node;

#ifdef CONFIG_MFD_SYSCON
void __iomem *syscon_base_lookup_by_phandle
	(struct device_node *np, const char *property);
struct regmap *syscon_node_to_regmap(struct device_node *np);
struct regmap *device_node_to_regmap(struct device_node *np);
struct regmap *syscon_regmap_lookup_by_compatible(const char *s);
extern struct regmap *syscon_regmap_lookup_by_phandle(
					struct device_node *np,
					const char *property);
#else
static inline void __iomem *syscon_base_lookup_by_phandle
	(struct device_node *np, const char *property)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct regmap *syscon_node_to_regmap(struct device_node *np)
{
	return ERR_PTR(-ENOSYS);
}
static inline struct regmap *device_node_to_regmap(struct device_node *np)
{
	return ERR_PTR(-ENOSYS);
}
static inline struct regmap *syscon_regmap_lookup_by_compatible(const char *s)
{
	return ERR_PTR(-ENOSYS);
}
static inline struct regmap *syscon_regmap_lookup_by_phandle(
					struct device_node *np,
					const char *property)
{
	return ERR_PTR(-ENOSYS);
}
#endif

#endif

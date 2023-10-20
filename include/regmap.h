/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __REGMAP_H
#define __REGMAP_H

#include <linux/compiler.h>
#include <linux/types.h>

extern void __compiletime_error("Last argument is now number of registers, not bytes. Fix "
				"it and include <linux/regmap.h> instead")
__regmap_bulk_api_changed(void);

struct regmap;

#ifndef regmap_bulk_read
#define regmap_bulk_read regmap_bulk_read
static inline int regmap_bulk_read(struct regmap *map, unsigned int reg, void *val,
				   size_t val_bytes)
{
	__regmap_bulk_api_changed();
	return -1;
}
#endif

#ifndef regmap_bulk_write
#define regmap_bulk_write regmap_bulk_write
static inline int regmap_bulk_write(struct regmap *map, unsigned int reg,
				    const void *val, size_t val_bytes)
{
	__regmap_bulk_api_changed();
	return -1;
}
#endif

#include <linux/regmap.h>

#endif /* __REGMAP_H */

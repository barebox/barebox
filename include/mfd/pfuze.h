/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __INCLUDE_PFUZE_H
#define __INCLUDE_PFUZE_H

struct regmap;

#ifdef CONFIG_REGULATOR_PFUZE
/*
 * For proper poweroff sequencing, users on imx6 needs to call
 * poweroff_handler_register_fn(imx6_pm_stby_poweroff);
 * inside of the callback, to ensure a proper poweroff sequence
 */
int pfuze_register_init_callback(void(*callback)(struct regmap *map));

#else

static inline int pfuze_register_init_callback(void(*callback)(struct regmap *map))
{
	return -ENODEV;
}
#endif

#endif /* __INCLUDE_PFUZE_H */

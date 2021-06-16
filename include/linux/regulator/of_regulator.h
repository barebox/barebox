/* SPDX-License-Identifier: GPL-2.0 */
/*
 * OpenFirmware regulator support routines
 */

#ifndef __LINUX_OF_REG_H
#define __LINUX_OF_REG_H

#include <linux/types.h>

struct device_d;
struct regulator_desc;

struct of_regulator_match {
	const char *name;
	void *driver_data;
	struct device_node *of_node;
	const struct regulator_desc *desc;
};

#if defined(CONFIG_OFDEVICE)
extern int of_regulator_match(struct device_d *dev, struct device_node *node,
			      struct of_regulator_match *matches,
			      unsigned int num_matches);
#else
static inline int of_regulator_match(struct device_d *dev,
				     struct device_node *node,
				     struct of_regulator_match *matches,
				     unsigned int num_matches)
{
	return 0;
}
#endif /* CONFIG_OF */

#endif /* __LINUX_OF_REG_H */

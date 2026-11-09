/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/pinctrl/machine.h?id=2e9ba1d9a31f68b4deb5f9e62a9201a51b00abf7 */
/*
 * Machine interface for the pinctrl subsystem.
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * Based on bits of regulator core, gpio core and clk core
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */
#ifndef __LINUX_PINCTRL_MACHINE_H
#define __LINUX_PINCTRL_MACHINE_H

#include <linux/array_size.h>

#include <linux/pinctrl/pinctrl-state.h>

enum pinctrl_map_type {
	PIN_MAP_TYPE_INVALID,
	PIN_MAP_TYPE_DUMMY_STATE,
	PIN_MAP_TYPE_MUX_GROUP,
	PIN_MAP_TYPE_CONFIGS_PIN,
	PIN_MAP_TYPE_CONFIGS_GROUP,
};

/**
 * struct pinctrl_map_mux - mapping table content for MAP_TYPE_MUX_GROUP
 * @group: the name of the group whose mux function is to be configured. This
 *	field may be left NULL, and the first applicable group for the function
 *	will be used.
 * @function: the mux function to select for the group
 */
struct pinctrl_map_mux {
	const char *group;
	const char *function;
};

/**
 * struct pinctrl_map_configs - mapping table content for MAP_TYPE_CONFIGS_*
 * @group_or_pin: the name of the pin or group whose configuration parameters
 *	are to be configured.
 * @configs: a pointer to an array of config parameters/values to program into
 *	hardware. Each individual pin controller defines the format and meaning
 *	of config parameters.
 * @num_configs: the number of entries in array @configs
 */
struct pinctrl_map_configs {
	const char *group_or_pin;
	unsigned long *configs;
	unsigned int num_configs;
};

/**
 * struct pinctrl_map - boards/machines shall provide this map for devices
 * @dev_name: the name of the device using this specific mapping, the name
 *	must be the same as in your struct device*. If this name is set to the
 *	same name as the pin controllers own dev_name(), the map entry will be
 *	hogged by the driver itself upon registration
 * @name: the name of this specific map entry for the particular machine.
 *	This is the parameter passed to pinmux_lookup_state()
 * @type: the type of mapping table entry
 * @ctrl_dev_name: the name of the device controlling this specific mapping,
 *	the name must be the same as in your struct device*. This field is not
 *	used for PIN_MAP_TYPE_DUMMY_STATE
 * @data: Data specific to the mapping type
 */
struct pinctrl_map {
	const char *dev_name;
	const char *name;
	enum pinctrl_map_type type;
	const char *ctrl_dev_name;
	union {
		struct pinctrl_map_mux mux;
		struct pinctrl_map_configs configs;
	} data;
};

#endif

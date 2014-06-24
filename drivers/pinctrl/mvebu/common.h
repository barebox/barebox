/*
 * Marvell MVEBU pinctrl driver
 *
 * Authors: Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *          Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PINCTRL_MVEBU_H__
#define __PINCTRL_MVEBU_H__

#include <io.h>

/**
 * struct mvebu_mpp_ctrl_setting - describe a mpp ctrl setting
 * @val: ctrl setting value
 * @name: ctrl setting name, e.g. uart2, spi0 - unique per mpp_mode
 * @subname: (optional) additional ctrl setting name, e.g. rts, cts
 * @variant: (optional) variant identifier mask
 * @flags: (private) flags to store gpi/gpo/gpio capabilities
 *
 * A ctrl_setting describes a specific internal mux function that a mpp pin
 * can be switched to. The value (val) will be written in the corresponding
 * register for common mpp pin configuration registers on MVEBU. SoC specific
 * mpp_get/_set function may use val to distinguish between different settings.
 *
 * The name will be used to switch to this setting in DT description, e.g.
 * marvell,function = "uart2". subname is only for debugging purposes.
 *
 * If name is one of "gpi", "gpo", "gpio" gpio capabilities are
 * parsed during initialization and stored in flags.
 *
 * The variant can be used to combine different revisions of one SoC to a
 * common pinctrl driver. It is matched (AND) with variant of soc_info to
 * determine if a setting is available on the current SoC revision.
 */
struct mvebu_mpp_ctrl_setting {
	u8 val;
	const char *name;
	const char *subname;
	u8 variant;
	u8 flags;
#define  MVEBU_SETTING_GPO	(1 << 0)
#define  MVEBU_SETTING_GPI	(1 << 1)
};

/**
 * struct mvebu_mpp_mode - link ctrl and settings
 * @pid: first pin id handled by this mode
 * @name: name of the mpp group
 * @mpp_get: function to get mpp setting
 * @mpp_set: function to set mpp setting
 * @settings: list of settings available for this mode
 *
 * A mode connects all available settings with the corresponding mpp_ctrl
 * given by pid.
 */
struct mvebu_mpp_mode {
	u8 pid;
	const char *name;
	int (*mpp_get)(unsigned pid, unsigned long *config);
	int (*mpp_set)(unsigned pid, unsigned long config);
	struct mvebu_mpp_ctrl_setting *settings;
};

/**
 * struct mvebu_pinctrl_soc_info - SoC specific info passed to pinctrl-mvebu
 * @variant: variant mask of soc_info
 * @controls: list of available mvebu_mpp_ctrls
 * @ncontrols: number of available mvebu_mpp_ctrls
 * @modes: list of available mvebu_mpp_modes
 * @nmodes: number of available mvebu_mpp_modes
 * @gpioranges: list of pinctrl_gpio_ranges
 * @ngpioranges: number of available pinctrl_gpio_ranges
 *
 * This struct describes all pinctrl related information for a specific SoC.
 * If variant is unequal 0 it will be matched (AND) with variant of each
 * setting and allows to distinguish between different revisions of one SoC.
 */
struct mvebu_pinctrl_soc_info {
	u8 variant;
	struct mvebu_mpp_mode *modes;
	int nmodes;
};

#define _MPP_VAR_FUNCTION(_val, _name, _subname, _mask)		\
	{							\
		.val = _val,					\
		.name = _name,					\
		.subname = _subname,				\
		.variant = _mask,				\
		.flags = 0,					\
	}

#define MPP_VAR_FUNCTION(_val, _name, _subname, _mask)		\
	_MPP_VAR_FUNCTION(_val, _name, _subname, _mask)

#define MPP_FUNCTION(_val, _name, _subname)			\
	MPP_VAR_FUNCTION(_val, _name, _subname, (u8)-1)

#define MPP_MODE(_id, _name, _func, ...)			\
	{							\
		.pid = _id,					\
		.name = _name,					\
		.mpp_get = _func ## _get,			\
		.mpp_set = _func ## _set,			\
		.settings = (struct mvebu_mpp_ctrl_setting[]){	\
			__VA_ARGS__, { } },			\
	}

#define MVEBU_MPPS_PER_REG	8
#define MVEBU_MPP_BITS		4
#define MVEBU_MPP_MASK		0xf

static inline int default_mpp_ctrl_get(void __iomem *base, unsigned int pid,
				       unsigned long *config)
{
	unsigned off = (pid / MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned shift = (pid % MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;

	*config = (readl(base + off) >> shift) & MVEBU_MPP_MASK;

	return 0;
}

static inline int default_mpp_ctrl_set(void __iomem *base, unsigned int pid,
				       unsigned long config)
{
	unsigned off = (pid / MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned shift = (pid % MVEBU_MPPS_PER_REG) * MVEBU_MPP_BITS;
	unsigned long reg;

	reg = readl(base + off) & ~(MVEBU_MPP_MASK << shift);
	writel(reg | (config << shift), base + off);

	return 0;
}

int mvebu_pinctrl_probe(struct device_d *dev,
			struct mvebu_pinctrl_soc_info *soc);

#endif

/*
 * Marvell MVEBU pinctrl core driver
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <malloc.h>
#include <of.h>
#include <pinctrl.h>

#include "common.h"

struct mvebu_pinctrl {
	struct mvebu_pinctrl_soc_info *soc;
	struct pinctrl_device pinctrl;
};

static struct mvebu_mpp_mode *mvebu_pinctrl_find_mode_by_name(
	struct mvebu_pinctrl *pctl, const char *name)
{
	unsigned n;
	for (n = 0; n < pctl->soc->nmodes; n++) {
		if (strcmp(name, pctl->soc->modes[n].name) == 0)
			return &pctl->soc->modes[n];
	}
	return NULL;
}

static struct mvebu_mpp_ctrl_setting *mvebu_pinctrl_find_setting_by_name(
	struct mvebu_pinctrl *pctl, struct mvebu_mpp_mode *mode,
	const char *name)
{
	struct mvebu_mpp_ctrl_setting *setting = mode->settings;

	while (setting->name) {
		if (strcmp(name, setting->name) == 0) {
			if (!pctl->soc->variant ||
			    (pctl->soc->variant & setting->variant))
				return setting;
		}
		setting++;
	}
	return NULL;
}

static int mvebu_pinctrl_set_state(struct pinctrl_device *pdev,
				   struct device_node *np)
{
	struct mvebu_pinctrl *pctl =
		container_of(pdev, struct mvebu_pinctrl, pinctrl);
	struct property *prop;
	const char *function;
	const char *group;
	int ret;

	ret = of_property_read_string(np, "marvell,function", &function);
	if (ret) {
		dev_err(pdev->dev, "missing marvell,function in node %s\n",
			np->full_name);
		return -EINVAL;
	}

	of_property_for_each_string(np, "marvell,pins", prop, group) {
		struct mvebu_mpp_mode *mode =
			mvebu_pinctrl_find_mode_by_name(pctl, group);
		struct mvebu_mpp_ctrl_setting *set;

		if (!mode) {
			dev_err(pdev->dev, "unknown pin group %s", group);
			continue;
		}

		set = mvebu_pinctrl_find_setting_by_name(pctl, mode, function);
		if (!set) {
			dev_err(pdev->dev,
				"unsupported function %s on pin %s\n",
				function, group);
			continue;
		}

		dev_dbg(pdev->dev, "np = %s, mode = %s, setting = %s (%s)\n",
			np->name, mode->name, set->name, set->subname);

		mode->mpp_set(mode->pid, set->val);
	}

	return 0;
}

static struct pinctrl_ops mvebu_pinctrl_ops = {
	.set_state = mvebu_pinctrl_set_state,
};

int mvebu_pinctrl_probe(struct device_d *dev,
			struct mvebu_pinctrl_soc_info *soc)
{
	struct mvebu_pinctrl *pctl;
	int ret;

	pctl = xzalloc(sizeof(*pctl));
	pctl->soc = soc;
	pctl->pinctrl.dev = dev;
	pctl->pinctrl.ops = &mvebu_pinctrl_ops;

	ret = pinctrl_register(&pctl->pinctrl);
	if (ret)
		free(pctl);

	return ret;
}

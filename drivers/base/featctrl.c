// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum, Pengutronix

#define pr_fmt(fmt) "featctrl: " fmt

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <of.h>

#include <featctrl.h>

/* List of registered feature controllers */
static LIST_HEAD(of_feature_controllers);

/**
 * feature_controller_register() - Register a feature controller
 * @feat: Pointer to feature controller
 */
int feature_controller_register(struct feature_controller *feat)
{
	struct device_node *np = dev_of_node(feat->dev);

	if (!np)
		return -EINVAL;

	list_add(&feat->list, &of_feature_controllers);
	dev_dbg(feat->dev, "Registering feature controller\n");
	return 0;
}
EXPORT_SYMBOL_GPL(feature_controller_register);

/**
 * featctrl_get_from_provider() - Look-up feature gate
 * @spec: OF phandle args to use for look-up
 * @gateid: ID of feature controller gate populated on successful lookup
 *
 * Looks for a feature controller under the node specified by @spec.
 *
 * Returns a valid pointer to struct feature_controller on success or ERR_PTR()
 * on failure.
 */
static struct feature_controller *featctrl_get_from_provider(struct of_phandle_args *spec,
							     unsigned *gateid)
{
	struct feature_controller *featctrl;
	int ret;

	ret = of_device_ensure_probed(spec->np);
	if (ret < 0)
		return ERR_PTR(ret);

	if (spec->args_count > 1)
		return ERR_PTR(-EINVAL);

	/* Check if we have such a controller in our array */
	list_for_each_entry(featctrl, &of_feature_controllers, list) {
		if (dev_of_node(featctrl->dev) == spec->np) {
			*gateid = spec->args_count ? spec->args[0] : 0;
			return featctrl;
		}
	}

	return ERR_PTR(-ENOENT);
}

/**
 * of_feature_controller_check - Check whether a feature controller gates the device
 * @np: Device node to check
 *
 * Parse device's OF node to find a feature controller specifier. If such is
 * found, checks it to determine whether device is gated.
 *
 * Returns FEATCTRL_GATED if a specified feature controller gates the device
 * and FEATCTRL_OKAY if none do. On error a negative error code is returned.
 */
int of_feature_controller_check(struct device_node *np)
{
	struct of_phandle_args featctrl_args;
	struct feature_controller *featctrl;
	int ret, err = 0, i, ngates;

	ngates = of_count_phandle_with_args(np, "barebox,feature-gates",
					    "#feature-cells");
	if (ngates <= 0)
		return FEATCTRL_OKAY;

	for (i = 0; i < ngates; i++) {
		unsigned gateid = 0;

		ret = of_parse_phandle_with_args(np, "barebox,feature-gates",
						 "#feature-cells", i, &featctrl_args);
		if (ret < 0)
			return ret;

		featctrl = featctrl_get_from_provider(&featctrl_args, &gateid);
		if (IS_ERR(featctrl)) {
			ret = PTR_ERR(featctrl);
			pr_debug("%s() failed to find feature controller: %pe\n",
				 __func__, ERR_PTR(ret));
			/*
			 * Assume that missing featctrls are unresolved
			 * dependency are report them as deferred
			 */
			return (ret == -ENOENT) ? -EPROBE_DEFER : ret;
		}

		ret = featctrl->check(featctrl, gateid);

		dev_dbg(featctrl->dev, "checking %s: %d\n", np->full_name, ret);

		if (ret == FEATCTRL_OKAY)
			return FEATCTRL_OKAY;
		if (ret != FEATCTRL_GATED)
			err = ret;
	}

	return err ?: FEATCTRL_GATED;
}
EXPORT_SYMBOL_GPL(of_feature_controller_check);

static int of_featctrl_fixup(struct device_node *root, void *context)
{
	struct device_node *srcnp, *dstnp;
	int err = 0;

	for_each_node_with_property(srcnp, "barebox,feature-gates") {
		int ret;

		ret = of_feature_controller_check(srcnp);
		if (ret < 0)
			err = ret;
		if (ret != FEATCTRL_GATED)
			continue;

		dstnp = of_get_node_by_reproducible_name(root, srcnp);
		if (!dstnp) {
			pr_debug("gated node %s not in fixup DT\n",
				 srcnp->full_name);
			continue;
		}

		pr_debug("fixing up gating of node %s\n", dstnp->full_name);
		/* Convention is deleting non-existing CPUs, not disable them. */
		if (of_property_match_string(srcnp, "device_type", "cpu") >= 0)
			of_delete_node(dstnp);
		else
			of_device_disable(dstnp);
	}

	return err;
}

static __maybe_unused int of_featctrl_fixup_register(void)
{
	return of_register_fixup(of_featctrl_fixup, NULL);
}
#ifdef CONFIG_FEATURE_CONTROLLER_FIXUP
device_initcall(of_featctrl_fixup_register);
#endif

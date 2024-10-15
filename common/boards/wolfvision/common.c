// SPDX-License-Identifier: GPL-2.0-only
/*
 * Common board code functions WolfVision boards.
 *
 * Copyright (C) 2024 WolfVision GmbH.
 */

#define pr_fmt(fmt) "boards: wolfvision: " fmt

#include <common.h>
#include <aiodev.h>
#include <net.h>
#include <state.h>

#include <boards/wolfvision/common.h>

#define WV_RK3568_HWID_TOLERANCE 50

int wolfvision_apply_overlay(const struct wv_overlay *overlay, char **files)
{
	int ret;

	if (overlay->filename && files) {
		if (*files) {
			char *old = *files;
			*files = basprintf("%s %s", old, overlay->filename);
			free(old);
		} else {
			*files = basprintf("%s", overlay->filename);
		}
	}

	if (overlay->data) {
		struct device_node *node =
			of_unflatten_dtb(overlay->data, INT_MAX);

		if (IS_ERR(node)) {
			pr_err("Cannot unflatten dtbo\n");
			return PTR_ERR(node);
		}

		ret = of_overlay_apply_tree(of_get_root_node(), node);

		of_delete_node(node);

		if (ret) {
			pr_err("Cannot apply overlay: %pe\n", ERR_PTR(ret));
			return ret;
		}

		return 1;
	}

	return 0;
}

int wolfvision_register_ethaddr(void)
{
	struct device_node *eth0;
	struct state *state;
	char mac[ETH_ALEN];
	int ret;

	state = state_by_alias("state");
	if (!state)
		return -ENOENT;

	ret = state_read_mac(state, "mac-address", mac);
	if (ret)
		return ret;

	if (!is_valid_ether_addr(mac))
		return -EINVAL;

	eth0 = of_find_node_by_alias(of_get_root_node(), "ethernet0");
	if (eth0)
		of_eth_register_ethaddr(eth0, mac);

	return 0;
}

static int wolfvision_rk3568_get_hwid(int chan_idx)
{
	const int values[WV_RK3568_HWID_MAX] = {
		0,    112,  225,  337,	450,  562,  675,  787,	900,
		1012, 1125, 1237, 1350, 1462, 1575, 1687, 1800,
	};
	int ret, hwid, voltage;
	char *chan_name;

	chan_name = basprintf("saradc.in_value%d_mV", chan_idx);
	ret = aiochannel_name_get_value(chan_name, &voltage);
	free(chan_name);
	if (ret)
		return ret;

	for (hwid = 0; hwid < ARRAY_SIZE(values); hwid++)
		if (abs(voltage - values[hwid]) < WV_RK3568_HWID_TOLERANCE)
			return hwid;

	return -EINVAL;
};

int wolfvision_rk3568_detect_hw(const struct wv_rk3568_extension *extensions,
				int num_extensions, char **overlays)
{
	int i, hwid, ret;
	bool do_of_probe = false;

	ret = of_device_ensure_probed_by_alias("saradc");
	if (ret)
		return ret;

	if (overlays && !*overlays)
		*overlays = xstrdup("");

	for (i = 0; i < num_extensions; i++) {
		const struct wv_rk3568_extension *extension = &extensions[i];
		const struct wv_overlay *overlay;

		ret = wolfvision_rk3568_get_hwid(extension->adc_chan);
		if (ret < 0) {
			pr_warning("Could not retrieve %s HWID (%d)\n",
				   extension->name, ret);
			continue;
		}

		hwid = ret;
		overlay = &extension->overlays[hwid];
		if (overlay->name) {
			pr_info("Detected %s %s\n", overlay->name,
				extension->name);
			ret = wolfvision_apply_overlay(overlay, overlays);
			if (ret > 0)
				do_of_probe = true;
		} else {
			pr_warning("Detected unknown %s HWID %d\n",
				   extension->name, hwid);
		}
	}

	if (do_of_probe) {
		of_clk_init();
		of_probe();
	}

	return 0;
}

// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <errno.h>
#include <driver.h>
#include <mach/linux.h>
#include <of.h>
#include <watchdog.h>
#include <mfd/syscon.h>
#include <reset_source.h>

struct sandbox_watchdog {
	struct watchdog wdd;
	bool cant_disable :1;
};

static inline struct sandbox_watchdog *to_sandbox_watchdog(struct watchdog *wdd)
{
	return container_of(wdd, struct sandbox_watchdog, wdd);
}

static int sandbox_watchdog_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct sandbox_watchdog *wd = to_sandbox_watchdog(wdd);

	if (!timeout && wd->cant_disable)
		return -ENOSYS;

	if (timeout > wdd->timeout_max)
		return -EINVAL;

	return linux_watchdog_set_timeout(timeout);
}

static int sandbox_watchdog_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct sandbox_watchdog *wd;
	struct watchdog *wdd;
	struct regmap *src;
	u32 src_offset;
	int ret;

	wd = xzalloc(sizeof(*wd));

	wdd = &wd->wdd;
	wdd->hwdev = dev;
	wdd->set_timeout = sandbox_watchdog_set_timeout;
	wdd->timeout_max = 1000;

	wd->cant_disable = of_property_read_bool(np, "barebox,cant-disable");

	ret = watchdog_register(wdd);
	if (ret) {
		dev_err(dev, "Failed to register watchdog device\n");
		return ret;
	}

	src = syscon_regmap_lookup_by_phandle(np, "barebox,reset-source");
	if (IS_ERR(src))
		return 0;

	ret = of_property_read_u32_index(np, "barebox,reset-source", 1, &src_offset);
	if (ret)
		return 0;

	regmap_update_bits(src, src_offset, 0xff, RESET_WDG);

	dev_info(dev, "probed\n");
	return 0;
}


static __maybe_unused struct of_device_id sandbox_watchdog_dt_ids[] = {
	{ .compatible = "barebox,sandbox-watchdog" },
	{ /* sentinel */ }
};

static struct driver_d sandbox_watchdog_drv = {
	.name  = "sandbox-watchdog",
	.of_compatible = sandbox_watchdog_dt_ids,
	.probe = sandbox_watchdog_probe,
};
device_platform_driver(sandbox_watchdog_drv);

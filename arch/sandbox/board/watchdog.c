// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <errno.h>
#include <driver.h>
#include <mach/linux.h>
#include <of.h>
#include <watchdog.h>
#include <linux/nvmem-consumer.h>
#include <reset_source.h>

struct sandbox_watchdog {
	struct watchdog wdd;
	bool cant_disable :1;
	struct nvmem_cell *reset_source_cell;
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

	nvmem_cell_write(wd->reset_source_cell, &(u8) { RESET_WDG }, 1);

	linux_watchdog_set_timeout(timeout);
	return 0;
}

static int sandbox_watchdog_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct sandbox_watchdog *wd;
	struct watchdog *wdd;
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

	wd->reset_source_cell = of_nvmem_cell_get(dev->device_node, "reset-source");
	if (IS_ERR(wd->reset_source_cell)) {
		dev_warn(dev, "No reset source info available: %pe\n", wd->reset_source_cell);
		goto out;
	}

out:
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

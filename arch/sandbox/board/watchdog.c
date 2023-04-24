// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <errno.h>
#include <driver.h>
#include <mach/linux.h>
#include <of.h>
#include <watchdog.h>
#include <linux/nvmem-consumer.h>
#include <asm/reset_source.h>

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

	sandbox_save_reset_source(wd->reset_source_cell, RESET_WDG);

	linux_watchdog_set_timeout(timeout);
	return 0;
}

static int sandbox_watchdog_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct sandbox_watchdog *wd;
	struct watchdog *wdd;

	wd = xzalloc(sizeof(*wd));

	wdd = &wd->wdd;
	wdd->hwdev = dev;
	wdd->set_timeout = sandbox_watchdog_set_timeout;
	wdd->timeout_max = 1000;

	wd->reset_source_cell = of_nvmem_cell_get(np, "reset-source");
	if (IS_ERR(wd->reset_source_cell)) {
		if (PTR_ERR(wd->reset_source_cell) != -EPROBE_DEFER)
			dev_warn(dev, "No reset source info available: %pe\n",
				 wd->reset_source_cell);
		wd->reset_source_cell = NULL;
	}

	wd->cant_disable = of_property_read_bool(np, "barebox,cant-disable");

	return watchdog_register(wdd);
}


static __maybe_unused struct of_device_id sandbox_watchdog_dt_ids[] = {
	{ .compatible = "barebox,sandbox-watchdog" },
	{ /* sentinel */ }
};

static struct driver sandbox_watchdog_drv = {
	.name  = "sandbox-watchdog",
	.of_compatible = sandbox_watchdog_dt_ids,
	.probe = sandbox_watchdog_probe,
};
device_platform_driver(sandbox_watchdog_drv);

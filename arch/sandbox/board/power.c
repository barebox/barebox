// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <driver.h>
#include <poweroff.h>
#include <restart.h>
#include <mach/linux.h>
#include <reset_source.h>
#include <linux/nvmem-consumer.h>

struct sandbox_power {
	struct restart_handler rst_hang, rst_reexec;
	struct poweroff_handler poweroff;
	struct nvmem_cell *reset_source_cell;
};

static void sandbox_poweroff(struct poweroff_handler *poweroff)
{
	struct sandbox_power *power = container_of(poweroff, struct sandbox_power, poweroff);

	nvmem_cell_write(power->reset_source_cell, &(u8) { RESET_POR }, 1);
	linux_exit();
}

static void sandbox_rst_hang(struct restart_handler *rst)
{
	linux_hang();
}

static void sandbox_rst_reexec(struct restart_handler *rst)
{
	u8 reason = RESET_RST;
	struct sandbox_power *power = container_of(rst, struct sandbox_power, rst_reexec);

	if (!IS_ERR(power->reset_source_cell))
		WARN_ON(nvmem_cell_write(power->reset_source_cell, &reason, 1) <= 0);

	linux_reexec();
}

static int sandbox_power_probe(struct device_d *dev)
{
	struct sandbox_power *power = xzalloc(sizeof(*power));
	size_t len;
	u8 *rst;

	power->poweroff = (struct poweroff_handler) {
		.name = "exit",
		.poweroff = sandbox_poweroff
	};

	poweroff_handler_register(&power->poweroff);

	power->rst_hang = (struct restart_handler) {
		.name = "hang",
		.restart = sandbox_rst_hang
	};

	power->rst_reexec = (struct restart_handler) {
		.name = "reexec", .priority = 200,
		.restart = sandbox_rst_reexec,
	};

	restart_handler_register(&power->rst_hang);

	if (IS_ENABLED(CONFIG_SANDBOX_REEXEC))
		restart_handler_register(&power->rst_reexec);

	power->reset_source_cell = of_nvmem_cell_get(dev->device_node, "reset-source");
	if (IS_ERR(power->reset_source_cell)) {
		dev_warn(dev, "No reset source info available: %pe\n", power->reset_source_cell);
		return 0;
	}

	rst = nvmem_cell_read(power->reset_source_cell, &len);
	if (!IS_ERR(rst)) {
		reset_source_set_prinst(*rst, RESET_SOURCE_DEFAULT_PRIORITY, 0);

		free(rst);
	}

	return 0;
}

static __maybe_unused struct of_device_id sandbox_power_dt_ids[] = {
	{ .compatible = "barebox,sandbox-power" },
	{ /* sentinel */ }
};

static struct driver_d sandbox_power_drv = {
	.name  = "sandbox-power",
	.of_compatible = sandbox_power_dt_ids,
	.probe = sandbox_power_probe,
};
coredevice_platform_driver(sandbox_power_drv);

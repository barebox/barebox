#include <common.h>
#include <driver.h>
#include <poweroff.h>
#include <restart.h>
#include <mach/linux.h>
#include <reset_source.h>
#include <mfd/syscon.h>

struct sandbox_power {
	struct restart_handler rst_hang, rst_reexec;
	struct regmap *src;
	u32 src_offset;
};

static void sandbox_poweroff(struct poweroff_handler *poweroff)
{
	linux_exit();
}

static void sandbox_rst_hang(struct restart_handler *rst)
{
	linux_hang();
}

static void sandbox_rst_reexec(struct restart_handler *rst)
{
	struct sandbox_power *power = container_of(rst, struct sandbox_power, rst_reexec);
	regmap_update_bits(power->src, power->src_offset, 0xff, RESET_RST);
	linux_reexec();
}

static int sandbox_power_probe(struct device_d *dev)
{
	struct sandbox_power *power = xzalloc(sizeof(*power));
	unsigned int rst;
	int ret;

	poweroff_handler_register_fn(sandbox_poweroff);

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

	power->src = syscon_regmap_lookup_by_phandle(dev->device_node, "barebox,reset-source");
	if (IS_ERR(power->src))
		return 0;

	ret = of_property_read_u32_index(dev->device_node, "barebox,reset-source", 1,
					 &power->src_offset);
	if (ret)
		return 0;

	ret = regmap_read(power->src, power->src_offset, &rst);
	if (ret == 0 && rst == 0)
		rst = RESET_POR;

	reset_source_set_prinst(rst, RESET_SOURCE_DEFAULT_PRIORITY, 0);
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

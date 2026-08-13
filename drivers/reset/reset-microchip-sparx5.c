// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/reset/reset-microchip-sparx5.c?id=b7e5ac83cb16f7ffd11dc23736f84276602100ed
/* Microchip Sparx5 / LAN969X / LAN966X Switch Reset driver
 *
 * Copyright (c) 2020 Microchip Technology Inc. and its subsidiaries.
 *
 * Ported from Linux drivers/reset/reset-microchip-sparx5.c. Identifiers,
 * struct layouts and per-SoC props are kept aligned with Linux so future
 * fixes can be backported with minimal context churn.
 */
#include <common.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <mfd/syscon.h>

struct reset_props {
	u32 protect_reg;
	u32 protect_bit;
	u32 reset_reg;
	u32 reset_bit;
};

struct mchp_reset_context {
	struct regmap *cpu_ctrl;
	struct regmap *gcb_ctrl;
	struct reset_controller_dev rcdev;
	const struct reset_props *props;
};

static const struct regmap_config sparx5_reset_regmap_config = {
	.reg_bits	= 32,
	.val_bits	= 32,
	.reg_stride	= 4,
};

static int sparx5_switch_reset(struct mchp_reset_context *ctx)
{
	u32 val;

	/* Make sure the core is PROTECTED from reset */
	regmap_update_bits(ctx->cpu_ctrl, ctx->props->protect_reg,
			   ctx->props->protect_bit, ctx->props->protect_bit);

	/* Start soft reset */
	regmap_write(ctx->gcb_ctrl, ctx->props->reset_reg,
		     ctx->props->reset_bit);

	/* Wait for soft reset done */
	return regmap_read_poll_timeout(ctx->gcb_ctrl, ctx->props->reset_reg,
					val,
					(val & ctx->props->reset_bit) == 0,
					100);
}

static int sparx5_reset_noop(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	return 0;
}

static const struct reset_control_ops sparx5_reset_ops = {
	.reset = sparx5_reset_noop,
};

static int mchp_sparx5_reset_probe(struct device *dev)
{
	struct device_node *dn = dev->of_node;
	struct mchp_reset_context *ctx;
	struct device_node *syscon_np;
	struct resource *iores;
	void __iomem *base;
	int ret;

	ctx = xzalloc(sizeof(*ctx));

	syscon_np = of_parse_phandle(dn, "cpu-syscon", 0);
	if (!syscon_np) {
		dev_err(dev, "missing 'cpu-syscon' phandle\n");
		ret = -ENODEV;
		goto err;
	}
	ctx->cpu_ctrl = syscon_node_to_regmap(syscon_np);
	if (IS_ERR(ctx->cpu_ctrl)) {
		dev_err(dev, "failed to map cpu-syscon: %pe\n", ctx->cpu_ctrl);
		ret = PTR_ERR(ctx->cpu_ctrl);
		goto err;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err;
	}
	base = IOMEM(iores->start);
	ctx->gcb_ctrl = regmap_init_mmio(dev, base, &sparx5_reset_regmap_config);
	if (IS_ERR(ctx->gcb_ctrl)) {
		ret = PTR_ERR(ctx->gcb_ctrl);
		goto err;
	}

	ctx->props = device_get_match_data(dev);
	if (!ctx->props) {
		ret = -EINVAL;
		goto err;
	}

	ctx->rcdev.nr_resets = 1;
	ctx->rcdev.ops = &sparx5_reset_ops;
	ctx->rcdev.of_node = dn;

	/*
	 * Issue the reset very early; reset_control_reset() callers see a
	 * no-op afterwards.
	 */
	ret = sparx5_switch_reset(ctx);
	if (ret) {
		dev_err(dev, "switch reset timed out: %d\n", ret);
		goto err;
	}

	ret = reset_controller_register(&ctx->rcdev);
	if (ret)
		goto err;

	dev_info(dev, "switch reset complete\n");
	return 0;

err:
	free(ctx);
	return ret;
}

static const struct reset_props reset_props_sparx5 = {
	.protect_reg    = 0x84,
	.protect_bit    = BIT(10),
	.reset_reg      = 0x0,
	.reset_bit      = BIT(1),
};

static const struct reset_props reset_props_lan966x = {
	.protect_reg    = 0x88,
	.protect_bit    = BIT(5),
	.reset_reg      = 0x0,
	.reset_bit      = BIT(1),
};

static const struct of_device_id mchp_sparx5_reset_of_match[] = {
	{ .compatible = "microchip,sparx5-switch-reset",
	  .data = &reset_props_sparx5 },
	{ .compatible = "microchip,lan966x-switch-reset",
	  .data = &reset_props_lan966x },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mchp_sparx5_reset_of_match);

static struct driver mchp_sparx5_reset_driver = {
	.name = "sparx5-switch-reset",
	.probe = mchp_sparx5_reset_probe,
	.of_compatible = DRV_OF_COMPAT(mchp_sparx5_reset_of_match),
};
/*
 * Same rationale as Linux's postcore_initcall(): the switch IP must be
 * reset before downstream consumers (SGPIO, MDIO, switch fabric) probe.
 */
coredevice_platform_driver(mchp_sparx5_reset_driver);

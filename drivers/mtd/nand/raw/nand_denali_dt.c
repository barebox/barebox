// SPDX-License-Identifier: GPL-2.0-only
/*
 * NAND Flash Controller Device Driver for DT
 *
 * Copyright © 2011, Picochip.
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <io.h>
#include <of_mtd.h>
#include <errno.h>
#include <globalvar.h>

#include <linux/clk.h>
#include <linux/spinlock.h>


#include "denali.h"

struct denali_dt {
	struct denali_controller	denali;
	struct clk *clk;	/* core clock */
	struct clk *clk_x;	/* bus interface clock */
	struct clk *clk_ecc;	/* ECC circuit clock */
};

struct denali_dt_data {
	unsigned int revision;
	unsigned int caps;
	unsigned int oob_skip_bytes;
	const  struct nand_ecc_caps *ecc_caps;
};

NAND_ECC_CAPS_SINGLE(denali_socfpga_ecc_caps, denali_calc_ecc_bytes,
		     512, 8, 15);
static const struct denali_dt_data denali_socfpga_data = {
	.caps = DENALI_CAP_HW_ECC_FIXUP,
	.oob_skip_bytes = 2,
	.ecc_caps = &denali_socfpga_ecc_caps,
};

enum of_binding_name {
	DENALI_OF_BINDING_CHIP,
	DENALI_OF_BINDING_CONTROLLER,
	DENALI_OF_BINDING_AUTO,
};

static const char *denali_of_binding_names[] = {
	"chip", "controller", "auto"
};

static int denali_of_binding;

/*
 * Older versions of the kernel driver require the partition nodes
 * to be direct subnodes of the controller node. Starting with Kernel
 * v5.2 (d8e8fd0ebf8b ("mtd: rawnand: denali: decouple controller and
 * NAND chips")) the device node for the Denali controller is seen as a
 * NAND controller node which has subnodes for each chip attached to that
 * controller. The chip subnodes then hold the partitions. The barebox
 * Denali driver also supports chip subnodes like the newer Kernel
 * driver. To find the container node for the partitions we first try
 * to find the chip subnodes in the Kernel device tree. Only if we
 * can't find these we try the controller device node and put the
 * partitions there.
 * Note that we take the existence of the chip subnodes in the kernel
 * device tree as a sign that we put the partitions there. When they
 * don't exist we use the controller node. This means you have to make
 * sure the chip subnodes exist when you start a Kernel that requires
 * these. Beginning with Kernel v5.5 (f34a5072c465 ("mtd: rawnand: denali:
 * remove the old unified controller/chip DT support")) the chip subnodes
 * are mandatory for the Kernel.
 */
static int denali_partition_fixup(struct mtd_info *mtd, struct device_node *root)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct denali_controller *denali = container_of(chip->controller,
							struct denali_controller,
							controller);
	struct device_node *np, *mtdnp = mtd_get_of_node(mtd);
	struct device_node *chip_np, *controller_np;
	char *name;

	name = of_get_reproducible_name(mtdnp);
	chip_np = of_find_node_by_reproducible_name(root, name);
	free(name);

	name = of_get_reproducible_name(mtdnp->parent);
	controller_np = of_find_node_by_reproducible_name(root, name);
	free(name);

	if (!controller_np)
		return -EINVAL;

	switch (denali_of_binding) {
	case DENALI_OF_BINDING_CHIP:
		if (chip_np) {
			np = chip_np;
		} else {
			np = of_new_node(controller_np, mtdnp->name);
			of_property_write_u32(np, "reg", 0);
			chip_np = np;
		}
		break;
	case DENALI_OF_BINDING_CONTROLLER:
		np = controller_np;
		break;
	case DENALI_OF_BINDING_AUTO:
	default:
		np = chip_np ? chip_np : controller_np;
		break;
	};

	if (!np)
		return -EINVAL;

	dev_info(denali->dev, "Fixing up %s node %pOF\n",
		 chip_np ? "chip" : "controller", np);

	if (!chip_np) {
		of_property_write_bool(np, "#size-cells", false);
		of_property_write_bool(np, "#address-cells", false);
	}

	return of_fixup_partitions(np, &mtd->cdev);
}

static int denali_dt_chip_init(struct denali_controller *denali,
			       struct device_node *chip_np)
{
	struct denali_chip *dchip;
	u32 bank;
	int nsels, i, ret;
	struct mtd_info *mtd;

	nsels = of_property_count_elems_of_size(chip_np, "reg", sizeof(u32));
	if (nsels < 0)
		return nsels;

	dchip = xzalloc(sizeof(*dchip) + sizeof(struct denali_chip_sel) *nsels);

	dchip->nsels = nsels;

	mtd = nand_to_mtd(&dchip->chip);

	mtd->of_fixup = denali_partition_fixup;

	for (i = 0; i < nsels; i++) {
		ret = of_property_read_u32_index(chip_np, "reg", i, &bank);
		if (ret)
			return ret;

		dchip->sels[i].bank = bank;

		nand_set_flash_node(&dchip->chip, chip_np);
	}

	ret = denali_chip_init(denali, dchip);
	if (ret)
		return ret;

	dev_add_param_enum(&dchip->chip.base.mtd.dev, "denali_partition_binding",
			   NULL, NULL,  &denali_of_binding, denali_of_binding_names,
			   ARRAY_SIZE(denali_of_binding_names), NULL);

	return 0;
}

static int denali_dt_probe(struct device *ofdev)
{
	struct resource *iores;
	struct denali_dt *dt;
	struct denali_controller *denali;
	struct denali_dt_data *data;
	struct device_node *np;
	int ret;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return 1;

	ret = dev_get_drvdata(ofdev, (const void **)&data);
	if (ret)
		return ret;

	dt = kzalloc(sizeof(*dt), GFP_KERNEL);
	if (!dt)
		return -ENOMEM;
	denali = &dt->denali;

	denali->dev = ofdev;

	iores = dev_request_mem_resource(ofdev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	denali->host = IOMEM(iores->start);

	iores = dev_request_mem_resource(ofdev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	denali->reg = IOMEM(iores->start);

	dt->clk = clk_get(ofdev, "nand");
	if (IS_ERR(dt->clk))
		return PTR_ERR(dt->clk);

	dt->clk_x = clk_get(ofdev, "nand_x");
	if (IS_ERR(dt->clk_x))
		return PTR_ERR(dt->clk_x);

	dt->clk_ecc = clk_get(ofdev, "ecc");
	if (IS_ERR(dt->clk_ecc))
		return PTR_ERR(dt->clk_ecc);

	clk_enable(dt->clk);
	clk_enable(dt->clk_x);
	clk_enable(dt->clk_ecc);

	denali->clk_rate = clk_get_rate(dt->clk);
	denali->clk_x_rate = clk_get_rate(dt->clk_x);

	denali->revision = data->revision;
	denali->caps = data->caps;
	denali->oob_skip_bytes = data->oob_skip_bytes;
	denali->ecc_caps = data->ecc_caps;

	ret = denali_init(denali);
	if (ret)
		goto out_disable_clk;

	for_each_child_of_node(ofdev->of_node, np) {
		ret = denali_dt_chip_init(denali, np);
		if (ret)
			goto out_disable_clk;
	}

	return 0;

out_disable_clk:
	clk_disable(dt->clk);

	return ret;
}

static __maybe_unused struct of_device_id denali_nand_compatible[] = {
	{
		.compatible = "altr,socfpga-denali-nand",
		.data = &denali_socfpga_data,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, denali_nand_compatible);

static struct driver denali_dt_driver = {
	.name	= "denali-nand-dt",
	.probe		= denali_dt_probe,
	.of_compatible = DRV_OF_COMPAT(denali_nand_compatible)
};
device_platform_driver(denali_dt_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jamie Iles");
MODULE_DESCRIPTION("DT driver for Denali NAND controller");

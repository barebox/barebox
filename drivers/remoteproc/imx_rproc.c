// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017 Pengutronix, Oleksij Rempel <kernel@pengutronix.de>
 */

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/remoteproc.h>
#include <mfd/syscon.h>
#include <module.h>
#include <memory.h>
#include <of_address.h>
#include <of_device.h>
#include <regmap.h>

#define IMX7D_SRC_SCR			0x0C
#define IMX7D_ENABLE_M4			BIT(3)
#define IMX7D_SW_M4P_RST		BIT(2)
#define IMX7D_SW_M4C_RST		BIT(1)
#define IMX7D_SW_M4C_NON_SCLR_RST	BIT(0)

#define IMX7D_M4_RST_MASK		(IMX7D_ENABLE_M4 | IMX7D_SW_M4P_RST \
					 | IMX7D_SW_M4C_RST \
					 | IMX7D_SW_M4C_NON_SCLR_RST)

#define IMX7D_M4_START			(IMX7D_ENABLE_M4 | IMX7D_SW_M4P_RST \
					 | IMX7D_SW_M4C_RST)
#define IMX7D_M4_STOP			IMX7D_SW_M4C_NON_SCLR_RST

/* Address: 0x020D8000 */
#define IMX6SX_SRC_SCR			0x00
#define IMX6SX_ENABLE_M4		BIT(22)
#define IMX6SX_SW_M4P_RST		BIT(12)
#define IMX6SX_SW_M4C_NON_SCLR_RST	BIT(4)
#define IMX6SX_SW_M4C_RST		BIT(3)

#define IMX6SX_M4_RST_MASK		(IMX6SX_ENABLE_M4 | IMX6SX_SW_M4P_RST \
					 | IMX6SX_SW_M4C_NON_SCLR_RST \
					 | IMX6SX_SW_M4C_RST)

#define IMX7D_RPROC_MEM_MAX		8

/**
 * struct imx_rproc_mem - slim internal memory structure
 * @cpu_addr: MPU virtual address of the memory region
 * @sys_addr: Bus address used to access the memory region
 * @size: Size of the memory region
 */
struct imx_rproc_mem {
	void __iomem *cpu_addr;
	phys_addr_t sys_addr;
	size_t size;
};

/* att flags */
/* M4 own area. Can be mapped at probe */
#define ATT_OWN		BIT(1)

/* address translation table */
struct imx_rproc_att {
	u32 da;	/* device address (From Cortex M4 view)*/
	u32 sa;	/* system bus address */
	u32 size; /* size of reg range */
	int flags;
};

struct imx_rproc_dcfg {
	const struct imx_rproc_att	*att;
	size_t				att_size;
	int (*rproc_init)(struct rproc *rproc);
	int (*rproc_start)(struct rproc *rproc);
	int (*rproc_stop)(struct rproc *rproc);
};

struct imx_rproc {
	struct device			*dev;
	struct regmap			*regmap;
	struct regmap			*gpr;
	struct rproc			*rproc;
	const struct imx_rproc_dcfg	*dcfg;
	struct imx_rproc_mem		mem[IMX7D_RPROC_MEM_MAX];
	struct clk			*clk;
};

static const struct imx_rproc_att imx_rproc_att_imx6sx[] = {
	/* dev addr , sys addr  , size	    , flags */
	/* TCML (M4 Boot Code) - alias */
	{ 0x00000000, 0x007F8000, 0x00008000, 0 },
	/* OCRAM_S (Code) */
	{ 0x00180000, 0x008F8000, 0x00004000, 0 },
	/* OCRAM_S (Code) - alias */
	{ 0x00180000, 0x008FC000, 0x00004000, 0 },
	/* TCML (Code) */
	{ 0x1FFF8000, 0x007F8000, 0x00008000, ATT_OWN },
	/* DDR (Code) - alias, first part of DDR (Data) */
	{ 0x10000000, 0x80000000, 0x0FFF8000, 0 },

	/* TCMU (Data) */
	{ 0x20000000, 0x00800000, 0x00008000, ATT_OWN },
	/* OCRAM_S (Data) - alias? */
	{ 0x208F8000, 0x008F8000, 0x00004000, 0 },
	/* DDR (Data) */
	{ 0x80000000, 0x80000000, 0x60000000, 0 },
};

static const struct imx_rproc_att imx_rproc_att_imx7d[] = {
	/* dev addr , sys addr  , size	    , flags */
	/* OCRAM_S (M4 Boot code) - alias */
	{ 0x00000000, 0x00180000, 0x00008000, 0 },
	/* OCRAM_S (Code) */
	{ 0x00180000, 0x00180000, 0x00008000, ATT_OWN },
	/* OCRAM (Code) - alias */
	{ 0x00900000, 0x00900000, 0x00020000, 0 },
	/* OCRAM_EPDC (Code) - alias */
	{ 0x00920000, 0x00920000, 0x00020000, 0 },
	/* OCRAM_PXP (Code) - alias */
	{ 0x00940000, 0x00940000, 0x00008000, 0 },
	/* TCML (Code) */
	{ 0x1FFF8000, 0x007F8000, 0x00008000, ATT_OWN },
	/* DDR (Code) - alias, first part of DDR (Data) */
	{ 0x10000000, 0x80000000, 0x0FFF0000, 0 },

	/* TCMU (Data) */
	{ 0x20000000, 0x00800000, 0x00008000, ATT_OWN },
	/* OCRAM (Data) */
	{ 0x20200000, 0x00900000, 0x00020000, 0 },
	/* OCRAM_EPDC (Data) */
	{ 0x20220000, 0x00920000, 0x00020000, 0 },
	/* OCRAM_PXP (Data) */
	{ 0x20240000, 0x00940000, 0x00008000, 0 },
	/* DDR (Data) */
	{ 0x80000000, 0x80000000, 0x60000000, 0 },
};

static const struct imx_rproc_att imx_rproc_att_imx8mn[] = {
	/* dev addr , sys addr  , size      , flags */
	{ 0x00000000, 0x007e0000, 0x00020000, ATT_OWN }, /* ITCM   */
	{ 0x00180000, 0x00180000, 0x00009000, 0 },	 /* OCRAM_S */
	{ 0x00900000, 0x00900000, 0x00020000, 0 },	 /* OCRAM */
	{ 0x00920000, 0x00920000, 0x00020000, 0 },	 /* OCRAM */
	{ 0x00940000, 0x00940000, 0x00050000, 0 },	 /* OCRAM */
	{ 0x08000000, 0x08000000, 0x08000000, 0 },	 /* QSPI Code - alias */
	{ 0x10000000, 0x80000000, 0x0ffe0000, 0 },	 /* DDR (Code) - alias */
	{ 0x20000000, 0x00800000, 0x00020000, ATT_OWN }, /* DTCM */
	{ 0x20180000, 0x00180000, 0x00008000, ATT_OWN }, /* OCRAM_S - alias */
	{ 0x20200000, 0x00900000, 0x00020000, ATT_OWN }, /* OCRAM */
	{ 0x20220000, 0x00920000, 0x00020000, ATT_OWN }, /* OCRAM */
	{ 0x20240000, 0x00940000, 0x00040000, ATT_OWN }, /* OCRAM */
	{ 0x40000000, 0x40000000, 0x80000000, 0 },	 /* DDR (Data) */
};

static const struct imx_rproc_att imx_rproc_att_imx8mq[] = {
	/* dev addr , sys addr  , size      , flags */
	{ 0x00000000, 0x007e0000, 0x00020000, 0 },	 /* TCML - alias */
	{ 0x00180000, 0x00180000, 0x00008000, 0 },	 /* OCRAM_S */
	{ 0x00900000, 0x00900000, 0x00020000, 0 },	 /* OCRAM */
	{ 0x00920000, 0x00920000, 0x00020000, 0 },	 /* OCRAM */
	{ 0x08000000, 0x08000000, 0x08000000, 0 },	 /* QSPI Code - alias */
	{ 0x10000000, 0x80000000, 0x0ffe0000, 0 },	 /* DDR (Code) - alias */
	{ 0x1ffe0000, 0x007e0000, 0x00020000, ATT_OWN }, /* TCML */
	{ 0x20000000, 0x00800000, 0x00020000, ATT_OWN }, /* TCMU */
	{ 0x20180000, 0x00180000, 0x00008000, ATT_OWN }, /* OCRAM_S */
	{ 0x20200000, 0x00900000, 0x00020000, ATT_OWN }, /* OCRAM */
	{ 0x20220000, 0x00920000, 0x00020000, ATT_OWN }, /* OCRAM */
	{ 0x40000000, 0x40000000, 0x80000000, 0 },	 /* DDR (Data) */
};

static int imx6sx_rproc_start(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;

	return regmap_update_bits(priv->regmap,
			IMX6SX_SRC_SCR,
			IMX6SX_M4_RST_MASK,
			IMX6SX_ENABLE_M4 | IMX6SX_SW_M4P_RST | IMX6SX_SW_M4C_RST);
}

static int imx6sx_rproc_stop(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;

	return regmap_update_bits(priv->regmap,
			IMX6SX_SRC_SCR,
			IMX6SX_M4_RST_MASK,
			IMX6SX_SW_M4C_NON_SCLR_RST);
}

static int imx7d_rproc_start(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;

	return regmap_update_bits(priv->regmap,
			IMX7D_SRC_SCR,
			IMX7D_M4_RST_MASK,
			IMX7D_M4_START);
}

static int imx7d_rproc_stop(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;

	return regmap_update_bits(priv->regmap,
			IMX7D_SRC_SCR,
			IMX7D_M4_RST_MASK,
			IMX7D_M4_STOP);
}

#define IOMUXC_GPR22		0x58
#define IOMUXC_GPR22_CM7_CPUWAIT	BIT(0)

static int imx8mn_cm7_wait(struct rproc *rproc, bool wait)
{
	struct imx_rproc *priv = rproc->priv;

	return regmap_update_bits(priv->gpr,
			IOMUXC_GPR22,
			IOMUXC_GPR22_CM7_CPUWAIT,
			wait ? IOMUXC_GPR22_CM7_CPUWAIT : 0);
}

static int imx8mn_rproc_start(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;
	int ret;

	ret = regmap_update_bits(priv->regmap,
			IMX7D_SRC_SCR,
			IMX7D_M4_RST_MASK,
			IMX7D_ENABLE_M4 | IMX7D_SW_M4C_RST);
	if (ret)
		return ret;

	ret = imx8mn_cm7_wait(rproc, false);
	if (ret)
		return ret;

	return 0;
}

static int imx8mn_rproc_stop(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;
	int ret;

	ret = imx8mn_cm7_wait(rproc, true);
	if (ret)
		return ret;

	return regmap_update_bits(priv->regmap,
			IMX7D_SRC_SCR,
			IMX7D_M4_RST_MASK,
			IMX7D_ENABLE_M4 | IMX7D_SW_M4C_RST | IMX7D_SW_M4C_NON_SCLR_RST);
}

static int imx8mn_rproc_init(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;
	int ret;

	priv->gpr = syscon_regmap_lookup_by_compatible("fsl,imx8mp-iomuxc-gpr");
	if (IS_ERR(priv->gpr))
		return PTR_ERR(priv->gpr);

	ret = imx8mn_cm7_wait(rproc, true);
	if (ret)
		return ret;

	return regmap_update_bits(priv->regmap,
			IMX7D_SRC_SCR,
			IMX7D_M4_RST_MASK,
			IMX7D_ENABLE_M4 | IMX7D_SW_M4C_RST);
}

static const struct imx_rproc_dcfg imx_rproc_cfg_imx6sx = {
	.att		= imx_rproc_att_imx6sx,
	.att_size	= ARRAY_SIZE(imx_rproc_att_imx6sx),
	.rproc_start	= imx6sx_rproc_start,
	.rproc_stop	= imx6sx_rproc_stop,
};

static const struct imx_rproc_dcfg imx_rproc_cfg_imx7d = {
	.att		= imx_rproc_att_imx7d,
	.att_size	= ARRAY_SIZE(imx_rproc_att_imx7d),
	.rproc_start	= imx7d_rproc_start,
	.rproc_stop	= imx7d_rproc_stop,
};

static const struct imx_rproc_dcfg imx_rproc_cfg_imx8mn = {
	.att		= imx_rproc_att_imx8mn,
	.att_size	= ARRAY_SIZE(imx_rproc_att_imx8mn),
	.rproc_init	= imx8mn_rproc_init,
	.rproc_start	= imx8mn_rproc_start,
	.rproc_stop	= imx8mn_rproc_stop,
};

static const struct imx_rproc_dcfg imx_rproc_cfg_imx8mq = {
	.att		= imx_rproc_att_imx8mq,
	.att_size	= ARRAY_SIZE(imx_rproc_att_imx8mq),
	.rproc_start	= imx7d_rproc_start,
	.rproc_stop	= imx7d_rproc_stop,
};

static int imx_rproc_start(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;
	const struct imx_rproc_dcfg *dcfg = priv->dcfg;

	return dcfg->rproc_start(rproc);
}

static int imx_rproc_stop(struct rproc *rproc)
{
	struct imx_rproc *priv = rproc->priv;
	const struct imx_rproc_dcfg *dcfg = priv->dcfg;

	return dcfg->rproc_stop(rproc);
}

static int imx_rproc_da_to_sys(struct imx_rproc *priv, u64 da,
			       int len, u64 *sys)
{
	const struct imx_rproc_dcfg *dcfg = priv->dcfg;
	int i;

	/* parse address translation table */
	for (i = 0; i < dcfg->att_size; i++) {
		const struct imx_rproc_att *att = &dcfg->att[i];

		if (da >= att->da && da + len < att->da + att->size) {
			unsigned int offset = da - att->da;

			*sys = att->sa + offset;
			return 0;
		}
	}

	dev_warn(priv->dev, "Translation failed: da = 0x%llx len = 0x%x\n",
		 da, len);
	return -ENOENT;
}

static void *imx_rproc_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct imx_rproc *priv = rproc->priv;
	void *va = NULL;
	u64 sys;
	int i;

	if (len <= 0)
		return NULL;

	/*
	 * On device side we have many aliases, so we need to convert device
	 * address (M4) to system bus address first.
	 */
	if (imx_rproc_da_to_sys(priv, da, len, &sys))
		return NULL;

	for (i = 0; i < IMX7D_RPROC_MEM_MAX; i++) {
		if (sys >= priv->mem[i].sys_addr && sys + len <
		    priv->mem[i].sys_addr +  priv->mem[i].size) {
			unsigned int offset = sys - priv->mem[i].sys_addr;
			/* __force to make sparse happy with type conversion */
			va = (__force void *)(priv->mem[i].cpu_addr + offset);
			break;
		}
	}

	dev_dbg(&rproc->dev, "da = 0x%llx len = 0x%x va = 0x%p\n", da, len, va);

	return va;
}

static const struct rproc_ops imx_rproc_ops = {
	.start		= imx_rproc_start,
	.stop		= imx_rproc_stop,
	.da_to_va       = imx_rproc_da_to_va,
};

static int imx_rproc_addr_init(struct imx_rproc *priv,
			       struct device *dev)
{
	const struct imx_rproc_dcfg *dcfg = priv->dcfg;
	struct device_node *np = dev->of_node;
	int a, b = 0, err, nph;

	/* remap required addresses */
	for (a = 0; a < dcfg->att_size; a++) {
		const struct imx_rproc_att *att = &dcfg->att[a];
		struct resource *res_cpu;

		if (!(att->flags & ATT_OWN))
			continue;

		if (b >= IMX7D_RPROC_MEM_MAX)
			break;

		res_cpu = request_iomem_region(dev_name(dev),
							     att->sa,
							     att->sa + att->size - 1);
		if (!res_cpu) {
			dev_err(dev, "remap required addresses failed\n");
			return PTR_ERR(priv->mem[b].cpu_addr);
		}
		priv->mem[b].cpu_addr = (void *)res_cpu->start;
		priv->mem[b].sys_addr = att->sa;
		priv->mem[b].size = att->size;
		b++;
	}

	/* memory-region is optional property */
	nph = of_count_phandle_with_args(np, "memory-region", NULL);
	if (nph <= 0)
		return 0;

	/* remap optional addresses */
	for (a = 0; a < nph; a++) {
		struct device_node *node;
		struct resource res, *res_cpu;

		node = of_parse_phandle(np, "memory-region", a);
		err = of_address_to_resource(node, 0, &res);
		if (err) {
			dev_err(dev, "unable to resolve memory region\n");
			return err;
		}

		if (b >= IMX7D_RPROC_MEM_MAX)
			break;

		res_cpu = request_sdram_region(dev_name(dev), res.start,
					       resource_size(&res));
		if (!res_cpu) {
			dev_err(dev, "remap optional addresses failed\n");
			return -ENOMEM;
		}
		priv->mem[b].cpu_addr = (void *)res_cpu->start;
		priv->mem[b].sys_addr = res.start;
		priv->mem[b].size = resource_size(&res);
		b++;
	}

	return 0;
}

static int imx_rproc_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct imx_rproc *priv;
	struct rproc *rproc;
	const struct imx_rproc_dcfg *dcfg;
	struct regmap *regmap;
	int ret;

	regmap = syscon_regmap_lookup_by_phandle(np, "syscon");
	if (IS_ERR(regmap)) {
		dev_err(dev, "failed to find syscon\n");
		return PTR_ERR(regmap);
	}

	/* set some other name then imx */
	rproc = rproc_alloc(dev, dev_name(dev), &imx_rproc_ops, sizeof(*priv));
	if (!rproc)
		return -ENOMEM;

	dcfg = of_device_get_match_data(dev);
	if (!dcfg) {
		ret = -EINVAL;
		goto err_put_rproc;
	}

	priv = rproc->priv;
	priv->rproc = rproc;
	priv->regmap = regmap;
	priv->dcfg = dcfg;
	priv->dev = dev;

	ret = imx_rproc_addr_init(priv, dev);
	if (ret) {
		dev_err(dev, "failed on imx_rproc_addr_init\n");
		goto err_put_rproc;
	}

	priv->clk = clk_get(dev, 0);
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "Failed to get clock\n");
		ret = PTR_ERR(priv->clk);
		goto err_put_rproc;
	}

	/*
	 * clk for M4 block including memory. Should be
	 * enabled before .start for FW transfer.
	 */
	ret = clk_enable(priv->clk);
	if (ret) {
		dev_err(&rproc->dev, "Failed to enable clock\n");
		goto err_put_rproc;
	}

	if (dcfg->rproc_init) {
		ret = dcfg->rproc_init(rproc);
		if (ret)
			goto err_put_clk;
	}

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "rproc_add failed\n");
		goto err_put_clk;
	}

	return 0;

err_put_clk:
	clk_disable(priv->clk);
err_put_rproc:
	return ret;
}

static const struct of_device_id imx_rproc_of_match[] = {
	{ .compatible = "fsl,imx6sx-cm4", .data = &imx_rproc_cfg_imx6sx },
	{ .compatible = "fsl,imx7d-cm4", .data = &imx_rproc_cfg_imx7d },
	{ .compatible = "fsl,imx8mm-cm4", .data = &imx_rproc_cfg_imx8mq },
	{ .compatible = "fsl,imx8mn-cm7", .data = &imx_rproc_cfg_imx8mn },
	{ .compatible = "fsl,imx8mp-cm7", .data = &imx_rproc_cfg_imx8mn },
	{ .compatible = "fsl,imx8mq-cm4", .data = &imx_rproc_cfg_imx8mq },
	{},
};
MODULE_DEVICE_TABLE(of, imx_rproc_of_match);

static struct driver imx_rproc_driver = {
	.name = "imx-rproc",
	.probe = imx_rproc_probe,
	.of_compatible = DRV_OF_COMPAT(imx_rproc_of_match),
};
device_platform_driver(imx_rproc_driver);

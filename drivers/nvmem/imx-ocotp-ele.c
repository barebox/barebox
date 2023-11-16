// SPDX-License-Identifier: GPL-2.0-only
/*
 * i.MX9 OCOTP fusebox driver
 *
 * Copyright 2023 NXP
 */
#include <common.h>
#include <io.h>
#include <linux/nvmem-provider.h>
#include <linux/regmap.h>
#include <mach/imx/ele.h>

enum fuse_type {
	FUSE_FSB = 1,
	FUSE_ELE = 2,
	FUSE_INVALID = -1
};

struct ocotp_map_entry {
	u32 start; /* start word */
	u32 num; /* num words */
	enum fuse_type type;
};

struct ocotp_devtype_data {
	u32 reg_off;
	char *name;
	u32 size;
	u32 num_entry;
	u32 flag;
	struct ocotp_map_entry *entry;
};

struct imx_ocotp_priv {
	struct device *dev;
	struct regmap *map;
	void __iomem *base;
	const struct ocotp_devtype_data *data;
	struct regmap_config map_config;
};

static enum fuse_type imx_ocotp_fuse_type(struct imx_ocotp_priv *priv, u32 index)
{
	const struct ocotp_devtype_data *data = priv->data;
	u32 start, end;
	int i;

	for (i = 0; i < data->num_entry; i++) {
		start = data->entry[i].start;
		end = data->entry[i].start + data->entry[i].num;

		if (index >= start && index < end)
			return data->entry[i].type;
	}

	return FUSE_INVALID;
}

static int imx_ocotp_reg_read(void *context, unsigned int offset, unsigned int *val)
{
	struct imx_ocotp_priv *priv = context;
	void __iomem *reg = priv->base + priv->data->reg_off;
	u32 index;
	enum fuse_type type;
	int ret;
	u32 fuse_word;

	index = offset >> 2;

	type = imx_ocotp_fuse_type(priv, index);

	switch (type) {
	case FUSE_ELE:
		ret = ele_read_common_fuse(index, &fuse_word, NULL);
		if (ret)
			*val = 0xbeefdead;
		else
			*val = fuse_word;
		break;
	case FUSE_FSB:
		*val = readl_relaxed(reg + (index << 2));
		break;
	default:
		*val = 0xdeadbeef;
		break;
	}

	return 0;
};

static int imx_ocotp_cell_pp(void *context, const char *id, unsigned int offset,
			     void *data, size_t bytes)
{
	/* Deal with some post processing of nvmem cell data */
	if (id && !strcmp(id, "mac-address")) {
		u8 *buf = data;

		if (offset == 0x4ec) {
			swap(buf[0], buf[5]);
			swap(buf[1], buf[4]);
			swap(buf[2], buf[3]);
		} else if (offset == 0x4f2) {
			swap(buf[0], buf[1]);
			swap(buf[2], buf[5]);
			swap(buf[3], buf[4]);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

static struct regmap_bus imx_ocotp_regmap_bus = {
	.reg_read = imx_ocotp_reg_read,
};      

static int imx_ele_ocotp_probe(struct device *dev)
{
	struct imx_ocotp_priv *priv;
	struct nvmem_device *nvmem;
	struct resource *iores;
	struct ocotp_devtype_data *data;
	int ret;

	priv = xzalloc(sizeof(*priv));

	ret = dev_get_drvdata(dev, (const void **)&data);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
        if (IS_ERR(iores))
                return PTR_ERR(iores);

	priv->base = IOMEM(iores->start);
	priv->data = data;

	priv->map_config.reg_bits = 32;
	priv->map_config.val_bits = 32;
	priv->map_config.reg_stride = 4;
	priv->map_config.max_register = priv->data->size - 1;

	priv->map = regmap_init(dev, &imx_ocotp_regmap_bus, priv, &priv->map_config);
	if (IS_ERR(priv->map))
		return PTR_ERR(priv->map);

	nvmem = nvmem_regmap_register_with_pp(priv->map, "imx-ocotp",
					      imx_ocotp_cell_pp);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	return 0;
}

static struct ocotp_map_entry imx93_entries[] = {
	{ 12, 40, FUSE_FSB },
	{ 24, 11, FUSE_ELE },
	{ 63, 1, FUSE_ELE},
	{ 128, 16, FUSE_ELE },
	{ 182, 1, FUSE_ELE },
	{ 188, 1, FUSE_ELE },
	{ 312, 200, FUSE_FSB }
};

static const struct ocotp_devtype_data imx93_ocotp_data = {
	.reg_off = 0x8000,
	.size = 2048,
	.num_entry = ARRAY_SIZE(imx93_entries),
	.entry = imx93_entries,
};

static const struct of_device_id imx_ele_ocotp_dt_ids[] = {
	{ .compatible = "fsl,imx93-ocotp", .data = &imx93_ocotp_data, },
	{},
};
MODULE_DEVICE_TABLE(of, imx_ele_ocotp_dt_ids);

static struct driver imx_ele_ocotp_driver = {
	.name = "imx_ele_ocotp",
	.of_match_table = imx_ele_ocotp_dt_ids,
	.probe = imx_ele_ocotp_probe,
};
core_platform_driver(imx_ele_ocotp_driver);

MODULE_DESCRIPTION("i.MX OCOTP/ELE driver");
MODULE_AUTHOR("Peng Fan <peng.fan@nxp.com>");
MODULE_LICENSE("GPL");

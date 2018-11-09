/*
 * iim.c - i.MX IIM fusebox driver
 *
 * Provide an interface for programming and sensing the information that are
 * stored in on-chip fuse elements. This functionality is part of the IC
 * Identification Module (IIM), which is present on some i.MX CPUs.
 *
 * Copyright (c) 2010 Baruch Siach <baruch@tkos.co.il>,
 * 	Orex Computed Radiography
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <param.h>
#include <fcntl.h>
#include <malloc.h>
#include <of.h>
#include <io.h>
#include <regmap.h>
#include <regulator.h>
#include <linux/err.h>

#include <mach/iim.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>
#include <mach/clock-imx51_53.h>
#include <mach/imx25-fusemap.h>

#define DRIVERNAME	"imx_iim"
#define IIM_NUM_BANKS	8

struct iim_priv;

struct iim_bank {
	struct regmap *map;
	void __iomem *bankbase;
	int bank;
	struct iim_priv *iim;
	struct regmap_config map_config;
};

struct iim_priv {
	struct device_d dev;
	void __iomem *base;
	void __iomem *bankbase;
	struct iim_bank *bank[IIM_NUM_BANKS];
	int write_enable;
	int sense_enable;
	void (*supply)(int enable);
	struct regulator *fuse_supply;
};

struct imx_iim_drvdata {
	void (*supply)(int enable);
	char nregs[8];
};

static struct iim_priv *imx_iim;

static void imx_iim_field_decode(uint32_t field, unsigned *bank, unsigned *byte,
				 unsigned *bit, unsigned *mask)
{
	unsigned width;

	*bank = (field >> IIM_BANK_MASK_SHIFT) & ((1 << IIM_BANK_MASK_WIDTH) - 1);
	*byte = (field >> IIM_BYTE_MASK_SHIFT) & ((1 << IIM_BYTE_MASK_WIDTH) - 1);
	*bit = (field >> IIM_BIT_MASK_SHIFT) & ((1 << IIM_BIT_MASK_WIDTH) - 1);
	width = ((field >> IIM_WIDTH_MASK_SHIFT) & ((1 << IIM_WIDTH_MASK_WIDTH) - 1)) + 1;
	*mask = (1 << width) - 1;
}

int imx_iim_read_field(uint32_t field, unsigned *value)
{
	unsigned bank, byte, bit, mask;
	int ret;
	unsigned v;

	if (!imx_iim)
		return -ENODEV;

	imx_iim_field_decode(field, &bank, &byte, &bit, &mask);

	dev_dbg(&imx_iim->dev, "read_field: 0x%08x bank %d byte %d bit %d width 0x%x\n",
		field, bank, byte, bit, mask);

	ret = regmap_read(imx_iim->bank[bank]->map, byte, &v);
	if (ret < 0)
		return ret;

	v >>= bit;
	v &= mask;

	*value = v;

	return 0;
}

int imx_iim_write_field(uint32_t field, unsigned value)
{
	unsigned bank, byte, bit, mask;

	if (!imx_iim)
		return -ENODEV;

	imx_iim_field_decode(field, &bank, &byte, &bit, &mask);

	value &= mask;
	value <<= bit;

	dev_dbg(&imx_iim->dev, "write_field: 0x%08x bank %d byte %d bit %d width 0x%x\n",
	       field, bank, byte, bit, mask);

	return regmap_write(imx_iim->bank[bank]->map, byte, value);
}

int imx_iim_permanent_write(int enable)
{
	if (!imx_iim)
		return -ENODEV;

	imx_iim->write_enable = enable;

	return 0;
}

static int imx_iim_fuse_sense(struct iim_bank *bank, unsigned int row)
{
	struct iim_priv *iim = bank->iim;
	void __iomem *reg_base = iim->base;
	u8 err, stat;

	if (row > 255) {
		dev_err(&iim->dev, "%s: invalid row index\n", __func__);
		return -EINVAL;
	}

	/* clear status and error registers */
	writeb(3, reg_base + IIM_STATM);
	writeb(0xfe, reg_base + IIM_ERR);

	/* upper and lower address halves */
	writeb((bank->bank << 3) | (row >> 5), reg_base + IIM_UA);
	writeb((row << 3) & 0xf8, reg_base + IIM_LA);

	/* start fuse sensing */
	writeb(0x08, reg_base + IIM_FCTL);

	/* wait for sense done */
	while ((readb(reg_base + IIM_STAT) & 0x80) != 0)
		;

	stat = readb(reg_base + IIM_STAT);
	writeb(stat, reg_base + IIM_STAT);

	err = readb(reg_base + IIM_ERR);
	if (err) {
		dev_err(&iim->dev, "sense error (0x%02x)\n", err);
		return -EIO;
	}

	return readb(reg_base + IIM_SDAT);
}

static int imx_iim_reg_read(void *ctx, unsigned int reg, unsigned int *val)
{
	struct iim_bank *bank = ctx;

	if (bank->iim->sense_enable) {
		int row_val;
		row_val = imx_iim_fuse_sense(bank, reg);
		if (row_val < 0)
			return row_val;
		*val = (u8)row_val;
	} else {
		*val = readl(bank->bankbase + reg * 4);
	}

	return 0;
}

int imx_iim_read(unsigned int banknum, int offset, void *buf, int count)
{
	struct iim_priv *iim = imx_iim;
	struct iim_bank *bank;
	int ret;

	if (!imx_iim)
		return -ENODEV;

	if (banknum >= IIM_NUM_BANKS)
		return -EINVAL;

	bank = iim->bank[banknum];

	ret = regmap_bulk_read(bank->map, offset, buf, count);
	if (ret)
		return ret;

	return count;
}

static int imx_iim_fuse_blow_one(struct iim_bank *bank, unsigned int row, u8 value)
{
	struct iim_priv *iim = bank->iim;
	void __iomem *reg_base = iim->base;
	int bit, ret = 0;
	u8 err, stat;

	if (row > 255) {
		dev_err(&iim->dev, "%s: invalid row index\n", __func__);
		return -EINVAL;
	}

	/* clear status and error registers */
	writeb(3, reg_base + IIM_STATM);
	writeb(0xfe, reg_base + IIM_ERR);

	/* unprotect fuse programing */
	writeb(0xaa, reg_base + IIM_PREG_P);

	/* upper half address register */
	writeb((bank->bank << 3) | (row >> 5), reg_base + IIM_UA);

	for (bit = 0; bit < 8; bit++) {
		if (((value >> bit) & 1) == 0)
			continue;

		/* lower half address register */
		writeb(((row << 3) | bit), reg_base + IIM_LA);

		/* start fuse programing */
		writeb(0x71, reg_base + IIM_FCTL);

		/* wait for program done */
		while ((readb(reg_base + IIM_STAT) & 0x80) != 0)
			;

		/* clear program done status */
		stat = readb(reg_base + IIM_STAT);
		writeb(stat, reg_base + IIM_STAT);

		err = readb(reg_base + IIM_ERR);
		if (err) {
			dev_err(&iim->dev, "bank %u, row %u, bit %d program error "
					"(0x%02x)\n", bank->bank, row, bit,
					err);
			ret = -EIO;
			goto out;
		}
	}

out:
	/* protect fuse programing */
	writeb(0, reg_base + IIM_PREG_P);
	return ret;
}

static int imx_iim_fuse_blow(struct iim_bank *bank, unsigned offset, unsigned val)
{
	struct iim_priv *iim = bank->iim;
	int ret;

	if (IS_ERR(iim->fuse_supply)) {
		iim->fuse_supply = regulator_get(iim->dev.parent, "vdd-fuse");
		if (IS_ERR(iim->fuse_supply))
			return PTR_ERR(iim->fuse_supply);
	}

	ret = regulator_enable(iim->fuse_supply);
	if (ret < 0)
		return ret;

	if (iim->supply)
		iim->supply(1);

	ret = imx_iim_fuse_blow_one(bank, offset, val);
	if (ret < 0)
		goto err_out;

	if (iim->supply)
		iim->supply(0);

	ret = 0;

err_out:
	regulator_disable(iim->fuse_supply);

	return ret;
}

static ssize_t imx_iim_reg_write(void *ctx, unsigned int reg, unsigned int val)
{
	struct iim_bank *bank = ctx;

	if (IS_ENABLED(CONFIG_IMX_IIM_FUSE_BLOW) && bank->iim->write_enable)
		return imx_iim_fuse_blow(bank, reg, val);
	else
		writel(val, bank->bankbase + reg * 4);

	return 0;
}

static struct regmap_bus imx_iim_regmap_bus = {
	.reg_write = imx_iim_reg_write,
	.reg_read = imx_iim_reg_read,
};

static int imx_iim_add_bank(struct iim_priv *iim, int num, int nregs)
{
	struct iim_bank *bank;
	char *name;
	int ret;

	bank = xzalloc(sizeof (*bank));

	bank->bankbase	= iim->base + 0x800 + 0x400 * num;
	bank->bank	= num;
	bank->iim	= iim;

	iim->bank[num] = bank;

	bank->map_config.reg_bits = 8,
	bank->map_config.val_bits = 8,
	bank->map_config.reg_stride = 1,
	bank->map_config.max_register = (nregs - 1),
	bank->map_config.name = xasprintf("bank%d", num);

	bank->map = regmap_init(&iim->dev, &imx_iim_regmap_bus, bank, &bank->map_config);
	if (IS_ERR(bank->map))
		return PTR_ERR(bank->map);

	name = xasprintf(DRIVERNAME "_bank%d", num);

	ret = regmap_register_cdev(bank->map, name);

	free(name);

	if (ret)
		return ret;

	return 0;
}


#if IS_ENABLED(CONFIG_OFDEVICE)

#define MAC_BYTES	6

struct imx_iim_mac {
	struct iim_bank *bank;
	int offset;
	u8 ethaddr[MAC_BYTES];
};

static int imx_iim_get_mac(struct param_d *param, void *priv)
{
	struct imx_iim_mac *iimmac = priv;
	struct iim_bank *bank = iimmac->bank;
	int ret;

	ret = regmap_bulk_read(bank->map, iimmac->offset, iimmac->ethaddr, MAC_BYTES);
	if (ret < 0)
		return ret;

	return 0;
}

static int imx_iim_set_mac(struct param_d *param, void *priv)
{
	struct imx_iim_mac *iimmac = priv;
	struct iim_bank *bank = iimmac->bank;
	int ret;

	ret = regmap_bulk_write(bank->map, iimmac->offset, iimmac->ethaddr, MAC_BYTES);
	if (ret < 0)
		return ret;

	return 0;
}

static void imx_iim_add_mac_param(struct iim_priv *iim, int macnum, int bank, int offset)
{
	struct imx_iim_mac *iimmac;
	char *name;

	iimmac = xzalloc(sizeof(*iimmac));
	iimmac->offset = offset;
	iimmac->bank = iim->bank[bank];

	name = basprintf("ethaddr%d", macnum);

	dev_add_param_mac(&iim->dev, name, imx_iim_set_mac,
			imx_iim_get_mac, iimmac->ethaddr, iimmac);

	free(name);
}

/*
 * a single MAC address reference has the form
 * <&phandle iim-bank-no offset>, so three cells
 */
#define MAC_ADDRESS_PROPLEN	(3 * sizeof(__be32))

static void imx_iim_init_dt(struct device_d *dev, struct iim_priv *iim)
{
	char mac[6];
	const __be32 *prop;
	struct device_node *node = dev->device_node;
	int len, ret, macnum = 0;

	if (!node)
		return;

	prop = of_get_property(node, "barebox,provide-mac-address", &len);
	if (!prop)
		return;

	while (len >= MAC_ADDRESS_PROPLEN) {
		struct device_node *rnode;
		uint32_t phandle, bank, offset;

		phandle = be32_to_cpup(prop++);

		rnode = of_find_node_by_phandle(phandle);
		bank = be32_to_cpup(prop++);
		offset = be32_to_cpup(prop++);

		ret = regmap_bulk_read(iim->bank[bank]->map, offset, mac, MAC_BYTES);
		if (ret)
			dev_err(dev, "cannot read: %s\n", strerror(-ret));
		else
			of_eth_register_ethaddr(rnode, mac);

		if (IS_ENABLED(CONFIG_NET))
			imx_iim_add_mac_param(iim, macnum, bank, offset);
		macnum++;

		len -= MAC_ADDRESS_PROPLEN;
	}
}
#else
static inline void imx_iim_init_dt(struct device_d *dev, struct iim_priv *iim)
{
}
#endif

static int imx_iim_probe(struct device_d *dev)
{
	struct resource *iores;
	struct iim_priv *iim;
	int i, ret;
	struct imx_iim_drvdata *drvdata = NULL;
	char *nregs = NULL;

	if (imx_iim)
		return -EBUSY;

	iim = xzalloc(sizeof(*iim));

	dev_get_drvdata(dev, (const void **)&drvdata);

	if (drvdata && drvdata->supply)
		iim->supply = drvdata->supply;

	imx_iim = iim;

	dev_set_name(&iim->dev, "iim");
	iim->dev.parent = dev;
	iim->dev.id = DEVICE_ID_SINGLE;
	ret = register_device(&iim->dev);
	if (ret)
		return ret;

	iim->fuse_supply = ERR_PTR(-ENODEV);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	iim->base = IOMEM(iores->start);

	if (drvdata && drvdata->nregs[0])
		nregs = drvdata->nregs;

	for (i = 0; i < IIM_NUM_BANKS; i++) {
		int n = nregs ? nregs[i] : 32;

		if (!n)
			continue;

		ret = imx_iim_add_bank(iim, i, n);
		if (ret)
			return ret;
	}

	imx_iim_init_dt(dev, iim);

	if (IS_ENABLED(CONFIG_IMX_IIM_FUSE_BLOW))
		dev_add_param_bool(&iim->dev, "permanent_write_enable",
			NULL, NULL, &iim->write_enable, NULL);

	dev_add_param_bool(&iim->dev, "explicit_sense_enable",
			NULL, NULL, &iim->sense_enable, NULL);

	return 0;
}

static void imx5_iim_supply(void __iomem *ccm_base, int enable)
{
	uint32_t val;

	val = readl(ccm_base + MX5_CCM_CGPR);

	if (enable)
		val |= 1 << 4;
	else
		val &= ~(1 << 4);

	writel(val, ccm_base + MX5_CCM_CGPR);
}

static void imx51_iim_supply(int enable)
{
	imx5_iim_supply((void __iomem *)MX51_CCM_BASE_ADDR, enable);
}

static void imx53_iim_supply(int enable)
{
	imx5_iim_supply((void __iomem *)MX53_CCM_BASE_ADDR, enable);
}

static struct imx_iim_drvdata imx27_drvdata = {
	.nregs = { 32, 32 },
};

static struct imx_iim_drvdata imx25_imx31_imx35_drvdata = {
	.nregs = { 32, 32, 32 },
};

static struct imx_iim_drvdata imx51_drvdata = {
	.supply = imx51_iim_supply,
	.nregs = { 32, 32, 32, 32 },
};

static struct imx_iim_drvdata imx53_drvdata = {
	.supply = imx53_iim_supply,
	.nregs = { 32, 32, 32, 32, 16 },
};

static __maybe_unused struct of_device_id imx_iim_dt_ids[] = {
	{
		.compatible = "fsl,imx53-iim",
		.data = &imx53_drvdata,
	}, {
		.compatible = "fsl,imx51-iim",
		.data = &imx51_drvdata,
	}, {
		.compatible = "fsl,imx35-iim",
		.data = &imx25_imx31_imx35_drvdata,
	}, {
		.compatible = "fsl,imx31-iim",
		.data = &imx25_imx31_imx35_drvdata,
	}, {
		.compatible = "fsl,imx27-iim",
		.data = &imx27_drvdata,
	}, {
		.compatible = "fsl,imx25-iim",
		.data = &imx25_imx31_imx35_drvdata,
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_iim_driver = {
	.name	= DRIVERNAME,
	.probe	= imx_iim_probe,
	.of_compatible = DRV_OF_COMPAT(imx_iim_dt_ids),
};

static int imx_iim_init(void)
{
	platform_driver_register(&imx_iim_driver);

	return 0;
}
coredevice_initcall(imx_iim_init);

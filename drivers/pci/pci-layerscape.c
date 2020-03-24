// SPDX-License-Identifier: GPL-2.0
/*
 * PCIe host controller driver for Freescale Layerscape SoCs
 *
 * Copyright (C) 2014 Freescale Semiconductor.
 *
 * Author: Minghuan Lian <Minghuan.Lian@freescale.com>
 */

#include <common.h>
#include <clock.h>
#include <abort.h>
#include <malloc.h>
#include <io.h>
#include <init.h>
#include <gpio.h>
#include <asm/mmu.h>
#include <of_gpio.h>
#include <of_device.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <of_address.h>
#include <of_pci.h>
#include <regmap.h>
#include <magicvar.h>
#include <globalvar.h>
#include <mfd/syscon.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/sizes.h>

#include "pcie-designware.h"

/* PEX1/2 Misc Ports Status Register */
#define SCFG_PEXMSCPORTSR(pex_idx)	(0x94 + (pex_idx) * 4)
#define LTSSM_STATE_SHIFT	20
#define LTSSM_STATE_MASK	0x3f
#define LTSSM_PCIE_L0		0x11 /* L0 state */

/* PEX Internal Configuration Registers */
#define PCIE_STRFMR1		0x71c /* Symbol Timer & Filter Mask Register1 */
#define PCIE_ABSERR		0x8d0 /* Bridge Slave Error Response Register */
#define PCIE_ABSERR_SETTING	0x9401 /* Forward error of non-posted request */

#define PCIE_IATU_NUM		6

#define PCIE_LUT_UDR(n)                (0x800 + (n) * 8)
#define PCIE_LUT_ENABLE                (1 << 31)
#define PCIE_LUT_LDR(n)                (0x804 + (n) * 8)

struct ls_pcie_drvdata {
	u32 lut_offset;
	u32 ltssm_shift;
	u32 lut_dbg;
	int pex_stream_id_start;
	int pex_stream_id_end;
	const struct dw_pcie_host_ops *ops;
	const struct dw_pcie_ops *dw_pcie_ops;
};

#define PCIE_LUT_ENTRY_COUNT    32

struct ls_pcie_lut_entry {
	int stream_id;
	uint16_t devid;
};

struct ls_pcie {
	struct dw_pcie pci;
	void __iomem *lut;
	struct regmap *scfg;
	const char *compatible;
	const struct ls_pcie_drvdata *drvdata;
	int index;
	int next_lut_index;
	struct ls_pcie_lut_entry luts[PCIE_LUT_ENTRY_COUNT];
};

static struct ls_pcie *to_ls_pcie(struct dw_pcie *pci)
{
	return container_of(pci, struct ls_pcie, pci);
}

static void lut_writel(struct ls_pcie *pcie, unsigned int value,
		       unsigned int offset)
{
	out_be32(pcie->lut + offset, value);
}

static int ls_pcie_next_streamid(struct ls_pcie *pcie)
{
	static int next_stream_id;
	int first = pcie->drvdata->pex_stream_id_start;
	int last = pcie->drvdata->pex_stream_id_end;
	int current = next_stream_id + first;

	if (current  > last)
		return -EINVAL;

	next_stream_id++;

	return current;
}

static int ls_pcie_next_lut_index(struct ls_pcie *pcie)
{
	if (pcie->next_lut_index < PCIE_LUT_ENTRY_COUNT)
		return pcie->next_lut_index++;
	else
		return -ENOSPC;  /* LUT is full */
}

static bool ls_pcie_is_bridge(struct ls_pcie *pcie)
{
	struct dw_pcie *pci = &pcie->pci;
	u32 header_type;

	header_type = ioread8(pci->dbi_base + PCI_HEADER_TYPE);
	header_type &= 0x7f;

	return header_type == PCI_HEADER_TYPE_BRIDGE;
}

/* Clear multi-function bit */
static void ls_pcie_clear_multifunction(struct ls_pcie *pcie)
{
	struct dw_pcie *pci = &pcie->pci;

	iowrite8(PCI_HEADER_TYPE_BRIDGE, pci->dbi_base + PCI_HEADER_TYPE);
}

/* Drop MSG TLP except for Vendor MSG */
static void ls_pcie_drop_msg_tlp(struct ls_pcie *pcie)
{
	u32 val;
	struct dw_pcie *pci = &pcie->pci;

	val = ioread32(pci->dbi_base + PCIE_STRFMR1);
	val &= 0xDFFFFFFF;
	iowrite32(val, pci->dbi_base + PCIE_STRFMR1);
}

static void ls_pcie_disable_outbound_atus(struct ls_pcie *pcie)
{
	int i;

	for (i = 0; i < PCIE_IATU_NUM; i++)
		dw_pcie_disable_atu(&pcie->pci, i, DW_PCIE_REGION_OUTBOUND);
}

static int ls1021_pcie_link_up(struct dw_pcie *pci)
{
	u32 state;
	struct ls_pcie *pcie = to_ls_pcie(pci);

	if (!pcie->scfg)
		return 0;

	regmap_read(pcie->scfg, SCFG_PEXMSCPORTSR(pcie->index), &state);
	state = (state >> LTSSM_STATE_SHIFT) & LTSSM_STATE_MASK;

	if (state < LTSSM_PCIE_L0)
		return 0;

	return 1;
}

static int ls_pcie_link_up(struct dw_pcie *pci)
{
	struct ls_pcie *pcie = to_ls_pcie(pci);
	u32 state;

	state = (ioread32(pcie->lut + pcie->drvdata->lut_dbg) >>
		 pcie->drvdata->ltssm_shift) &
		 LTSSM_STATE_MASK;

	if (state < LTSSM_PCIE_L0)
		return 0;

	return 1;
}

/* Forward error response of outbound non-posted requests */
static void ls_pcie_fix_error_response(struct ls_pcie *pcie)
{
	struct dw_pcie *pci = &pcie->pci;

	iowrite32(PCIE_ABSERR_SETTING, pci->dbi_base + PCIE_ABSERR);
}

static int ls_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ls_pcie *pcie = to_ls_pcie(pci);

	/*
	 * Disable outbound windows configured by the bootloader to avoid
	 * one transaction hitting multiple outbound windows.
	 * dw_pcie_setup_rc() will reconfigure the outbound windows.
	 */
	ls_pcie_disable_outbound_atus(pcie);
	ls_pcie_fix_error_response(pcie);

	dw_pcie_dbi_ro_wr_en(pci);
	ls_pcie_clear_multifunction(pcie);
	dw_pcie_dbi_ro_wr_dis(pci);

	ls_pcie_drop_msg_tlp(pcie);

	dw_pcie_setup_rc(pp);

	return 0;
}

static int ls1021_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ls_pcie *pcie = to_ls_pcie(pci);
	struct device_d *dev = pci->dev;
	u32 index[2];
	int ret;

	pcie->scfg = syscon_regmap_lookup_by_phandle(dev->device_node,
						     "fsl,pcie-scfg");
	if (IS_ERR(pcie->scfg)) {
		ret = PTR_ERR(pcie->scfg);
		dev_err(dev, "No syscfg phandle specified\n");
		pcie->scfg = NULL;
		return ret;
	}

	if (of_property_read_u32_array(dev->device_node,
				       "fsl,pcie-scfg", index, 2)) {
		pcie->scfg = NULL;
		return -EINVAL;
	}
	pcie->index = index[1];

	return ls_pcie_host_init(pp);
}

static const struct dw_pcie_host_ops ls1021_pcie_host_ops = {
	.host_init = ls1021_pcie_host_init,
};

static const struct dw_pcie_host_ops ls_pcie_host_ops = {
	.host_init = ls_pcie_host_init,
};

static const struct dw_pcie_ops dw_ls1021_pcie_ops = {
	.link_up = ls1021_pcie_link_up,
};

static const struct dw_pcie_ops dw_ls_pcie_ops = {
	.link_up = ls_pcie_link_up,
};

static const struct ls_pcie_drvdata ls1021_drvdata = {
	.ops = &ls1021_pcie_host_ops,
	.dw_pcie_ops = &dw_ls1021_pcie_ops,
};

static const struct ls_pcie_drvdata ls1043_drvdata = {
	.lut_offset = 0x10000,
	.ltssm_shift = 24,
	.lut_dbg = 0x7fc,
	.pex_stream_id_start = 11,
	.pex_stream_id_end = 26,
	.ops = &ls_pcie_host_ops,
	.dw_pcie_ops = &dw_ls_pcie_ops,
};

static const struct ls_pcie_drvdata ls1046_drvdata = {
	.lut_offset = 0x80000,
	.ltssm_shift = 24,
	.lut_dbg = 0x407fc,
	.pex_stream_id_start = 11,
	.pex_stream_id_end = 26,
	.ops = &ls_pcie_host_ops,
	.dw_pcie_ops = &dw_ls_pcie_ops,
};

static const struct ls_pcie_drvdata ls2080_drvdata = {
	.lut_offset = 0x80000,
	.ltssm_shift = 0,
	.lut_dbg = 0x7fc,
	.pex_stream_id_start = 7,
	.pex_stream_id_end = 22,
	.ops = &ls_pcie_host_ops,
	.dw_pcie_ops = &dw_ls_pcie_ops,
};

static const struct ls_pcie_drvdata ls2088_drvdata = {
	.lut_offset = 0x80000,
	.ltssm_shift = 0,
	.lut_dbg = 0x407fc,
	.pex_stream_id_start = 7,
	.pex_stream_id_end = 18,
	.ops = &ls_pcie_host_ops,
	.dw_pcie_ops = &dw_ls_pcie_ops,
};

static const struct of_device_id ls_pcie_of_match[] = {
	{ .compatible = "fsl,ls1012a-pcie", .data = &ls1046_drvdata },
	{ .compatible = "fsl,ls1021a-pcie", .data = &ls1021_drvdata },
	{ .compatible = "fsl,ls1043a-pcie", .data = &ls1043_drvdata },
	{ .compatible = "fsl,ls1046a-pcie", .data = &ls1046_drvdata },
	{ .compatible = "fsl,ls2080a-pcie", .data = &ls2080_drvdata },
	{ .compatible = "fsl,ls2085a-pcie", .data = &ls2080_drvdata },
	{ .compatible = "fsl,ls2088a-pcie", .data = &ls2088_drvdata },
	{ .compatible = "fsl,ls1088a-pcie", .data = &ls2088_drvdata },
	{ },
};

static int __init ls_add_pcie_port(struct ls_pcie *pcie)
{
	struct dw_pcie *pci = &pcie->pci;
	struct pcie_port *pp = &pci->pp;
	struct device_d *dev = pci->dev;
	int ret;

	pp->ops = pcie->drvdata->ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static phandle ls_pcie_get_iommu_handle(struct device_node *np, phandle *handle)
{
	u32 arr[4];
	int ret;

	/*
	 * We expect an existing "iommu-map" property with bogus values. All we
	 * use from it is the phandle to the iommu.
	 */
	ret = of_property_read_u32_array(np, "iommu-map", arr, 4);
	if (ret)
		return -ENOENT;

	*handle = arr[1];

	return 0;
}

/*
 * Normally every device gets its own stream_id. The stream_ids are communicated
 * to the kernel in the device tree and are also configured in the controllers
 * LUT table.
 * This only works when all PCI devices are known in the bootloader which may not
 * always be the case. For example, when a PCI device is a FPGA and its firmware
 * is only loaded under Linux, then the device is not known to barebox and thus
 * not assigned a stream_id.
 *
 * With ls_pcie_share_stream_id = true all devices on a host controller get the
 * same stream_id assigned.
 */
static int ls_pcie_share_stream_id;

BAREBOX_MAGICVAR_NAMED(global_ls_pcie_share_stream_id,
		       global.layerscape_pcie.share_stream_ids,
		       "If true, use a stream_id per host controller and not per device");

static int ls_pcie_of_fixup(struct device_node *root, void *ctx)
{
	struct ls_pcie *pcie = ctx;
	struct device_d *dev = pcie->pci.dev;
	struct device_node *np;
	phandle iommu_handle = 0;
	char *name;
	u32 *msi_map, *iommu_map, phandle;
	int nluts;
	int ret, i;
	u32 devid, stream_id;

	name = of_get_reproducible_name(dev->device_node);

	np = root;
	do {
		np = of_find_node_by_reproducible_name(np, name);
		if (!np)
			return -ENODEV;
	} while (!of_device_is_compatible(np, pcie->compatible));

	ret = of_property_read_u32(np, "msi-parent", &phandle);
	if (ret) {
		dev_err(pcie->pci.dev, "Unable to get \"msi-parent\" phandle\n");
		return ret;
	}

	ret = ls_pcie_get_iommu_handle(np, &iommu_handle);
	if (ret) {
		dev_info(pcie->pci.dev, "No iommu phandle, won't fixup\n");
		return 0;
	}

	if (ls_pcie_share_stream_id) {
		nluts = 1;
		iommu_map = xmalloc(nluts * sizeof(u32) * 4);
		msi_map = xmalloc(nluts * sizeof(u32) * 4);
		stream_id = pcie->luts[0].stream_id;

		dev_dbg(pcie->pci.dev, "Using stream_id %d\n", stream_id);

		msi_map[0] = 0;
		msi_map[1] = phandle;
		msi_map[2] = stream_id;
		msi_map[3] = 0x10000;

		iommu_map[0] = 0;
		iommu_map[1] = iommu_handle;
		iommu_map[2] = stream_id;
		iommu_map[3] = 0x10000;

		lut_writel(pcie, 0xffff, PCIE_LUT_UDR(0));
		lut_writel(pcie, pcie->luts[0].stream_id | PCIE_LUT_ENABLE, PCIE_LUT_LDR(0));
	} else {
		nluts = pcie->next_lut_index;
		iommu_map = xmalloc(nluts * sizeof(u32) * 4);
		msi_map = xmalloc(nluts * sizeof(u32) * 4);

		for (i = 0; i < nluts; i++) {
			devid = pcie->luts[i].devid;
			stream_id = pcie->luts[i].stream_id;

			dev_dbg(pcie->pci.dev, "Using stream_id %d for device 0x%04x\n",
				stream_id, devid);

			msi_map[i * 4] = devid;
			msi_map[i * 4 + 1] = phandle;
			msi_map[i * 4 + 2] = stream_id;
			msi_map[i * 4 + 3] = 1;

			iommu_map[i * 4] = devid;
			iommu_map[i * 4 + 1] = iommu_handle;
			iommu_map[i * 4 + 2] = stream_id;
			iommu_map[i * 4 + 3] = 1;

			lut_writel(pcie, pcie->luts[i].devid << 16, PCIE_LUT_UDR(i));
			lut_writel(pcie, pcie->luts[i].stream_id | PCIE_LUT_ENABLE, PCIE_LUT_LDR(i));
		}
	}

	for (i = nluts; i < PCIE_LUT_ENTRY_COUNT; i++) {
		lut_writel(pcie, 0, PCIE_LUT_UDR(i));
		lut_writel(pcie, 0, PCIE_LUT_LDR(i));
	}

	/*
	 * An msi-map is a property to be added to the pci controller
	 * node.  It is a table, where each entry consists of 4 fields
	 * e.g.:
	 *
	 *      msi-map = <[devid] [phandle-to-msi-ctrl] [stream-id] [count]
	 *                 [devid] [phandle-to-msi-ctrl] [stream-id] [count]>;
	 */

	ret = of_property_write_u32_array(np, "msi-map", msi_map, nluts * 4);
	if (ret)
		goto out;

	ret = of_property_write_u32_array(np, "iommu-map", iommu_map, nluts * 4);
	if (ret)
		goto out;

	of_device_enable(np);

	ret = 0;

out:
	free(msi_map);
	free(iommu_map);

	return ret;
}

static int __init ls_pcie_probe(struct device_d *dev)
{
	struct dw_pcie *pci;
	struct ls_pcie *pcie;
	struct resource *dbi_base;
	const struct of_device_id *match;
	int ret;
	static int once;

	if (!once) {
		globalvar_add_simple_bool("layerscape_pcie.share_stream_ids",
					  &ls_pcie_share_stream_id);
		once = 1;
	}

	pcie = xzalloc(sizeof(*pcie));
	if (!pcie)
		return -ENOMEM;

	pci = &pcie->pci;

	match = of_match_device(dev->driver->of_compatible, dev);
	if (!match)
		return -ENODEV;

	pcie->drvdata = match->data;
	pcie->compatible = match->compatible;

	pci->dev = dev;
	pci->ops = pcie->drvdata->dw_pcie_ops;

	dbi_base = dev_request_mem_resource(dev, 0);
	if (IS_ERR(dbi_base))
		return PTR_ERR(dbi_base);

	pci->dbi_base = IOMEM(dbi_base->start);

	pcie->lut = pci->dbi_base + pcie->drvdata->lut_offset;

	if (!ls_pcie_is_bridge(pcie))
		return -ENODEV;

	dev->priv = pcie;

	ret = ls_add_pcie_port(pcie);
	if (ret < 0)
		return ret;

	of_register_fixup(ls_pcie_of_fixup, pcie);

	return 0;
}

static struct driver_d ls_pcie_driver = {
	.name = "layerscape-pcie",
	.of_compatible = DRV_OF_COMPAT(ls_pcie_of_match),
	.probe = ls_pcie_probe,
};
device_platform_driver(ls_pcie_driver);

static void ls_pcie_fixup(struct pci_dev *pcidev)
{
	struct pci_bus *bus = pcidev->bus, *p;
	struct pci_controller *host = bus->host;
	struct pcie_port *pp = container_of(host, struct pcie_port, pci);
	struct dw_pcie *dwpcie = container_of(pp, struct dw_pcie, pp);
	struct ls_pcie *lspcie = container_of(dwpcie, struct ls_pcie, pci);
	int stream_id, index;
	uint32_t devid;
	int base_bus_num = 0;

	stream_id = ls_pcie_next_streamid(lspcie);
	index = ls_pcie_next_lut_index(lspcie);

	p = pcidev->bus;
	while (p) {
		base_bus_num = p->number;
		p = p->parent_bus;
	}

	devid = (pcidev->bus->number - base_bus_num) << 8 | pcidev->devfn;

	lspcie->luts[index].devid = devid;
	lspcie->luts[index].stream_id = stream_id;
}

DECLARE_PCI_FIXUP_EARLY(PCI_ANY_ID, PCI_ANY_ID, ls_pcie_fixup);

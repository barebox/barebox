// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2025 Texas Instruments Incorporated - https://www.ti.com
 */

#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <linux/bitops.h>
#include <linux/sizes.h>
#include <linux/printk.h>
#include <dma.h>
#include <soc/ti/ti-udma.h>
#include <soc/ti/ti_sci_protocol.h>
#include <dma-devices.h>
#include <soc/ti/cppi5.h>
#include <soc/ti/k3-navss-ringacc.h>
#include <clock.h>
#include <linux/bitmap.h>
#include <driver.h>
#include <linux/device.h>

#include "k3-udma-hwdef.h"
#include "k3-psil-priv.h"
#include "k3-udma.h"

enum am62l_udma_mmr {
	AM62L_MMR_GCFG = 0,
	AM62L_MMR_BCHANRT,
	AM62L_MMR_CHANRT,
	AM62L_MMR_RFLOWRT,
	AM62L_MMR_LAST,
};

static const char * const am62l_mmr_names[] = {
	[AM62L_MMR_GCFG] = "gcfg",
	[AM62L_MMR_BCHANRT] = "bchanrt",
	[AM62L_MMR_CHANRT] = "chanrt",
	[AM62L_MMR_RFLOWRT] = "ringrt",
};

static int pktdma_v2_rx_channel_config(struct udma_chan *uc)
{
	u32 val = 0;

	if (uc->config.needs_epib)
		val |= UDMA_RFLOW_RFA_EINFO;
	if (uc->config.psd_size)
		val |= UDMA_RFLOW_RFA_PSINFO;

	udma_rflowrt_write(uc->rflow, UDMA_RX_FLOWRT_RFA, val);

	return 0;
}

static int bcdma_v2_alloc_chan_resources(struct udma_chan *uc)
{
	int ret;

	uc->config.pkt_mode = false;

	switch (uc->config.dir) {
	case DMA_MEM_TO_MEM:
		/* Non synchronized - mem to mem type of transfer */
		dev_dbg(uc->ud->dev, "%s: chan%d as MEM-to-MEM\n", __func__,
			uc->id);

		ret = bcdma_alloc_bchan_resources(uc);
		if (ret)
			return ret;

		break;
	default:
		/* Can not happen */
		dev_err(uc->ud->dev, "%s: chan%d invalid direction (%u)\n",
			__func__, uc->id, uc->config.dir);
		return -EINVAL;
	}

	/* check if the channel configuration was successful */
	if (ret)
		goto err_res_free;

	if (udma_is_chan_running(uc)) {
		dev_warn(uc->ud->dev, "chan%d: is running!\n", uc->id);
		udma_stop(uc);
		if (udma_is_chan_running(uc)) {
			dev_err(uc->ud->dev, "chan%d: won't stop!\n", uc->id);
			goto err_res_free;
		}
	}

	udma_reset_rings(uc);

	return 0;

err_res_free:
	bcdma_free_bchan_resources(uc);
	udma_free_tx_resources(uc);
	udma_free_rx_resources(uc);

	udma_reset_uchan(uc);

	return ret;
}

static int pktdma_v2_alloc_chan_resources(struct udma_chan *uc)
{
	struct udma_dev *ud = uc->ud;
	int ret;

	switch (uc->config.dir) {
	case DMA_MEM_TO_DEV:
		/* Slave transfer synchronized - mem to dev (TX) trasnfer */
		dev_dbg(uc->ud->dev, "%s: chan%d as MEM-to-DEV\n", __func__,
			uc->id);

		ret = udma_alloc_tx_resources(uc);
		if (ret) {
			uc->config.remote_thread_id = -1;
			return ret;
		}
		break;
	case DMA_DEV_TO_MEM:
		/* Slave transfer synchronized - dev to mem (RX) trasnfer */
		dev_dbg(uc->ud->dev, "%s: chan%d as DEV-to-MEM\n", __func__,
			uc->id);

		ret = udma_alloc_rx_resources(uc);
		if (ret) {
			uc->config.remote_thread_id = -1;
			return ret;
		}

		ret = pktdma_v2_rx_channel_config(uc);
		break;
	default:
		/* Can not happen */
		dev_err(uc->ud->dev, "%s: chan%d invalid direction (%u)\n",
			__func__, uc->id, uc->config.dir);
		return -EINVAL;
	}

	/* check if the channel configuration was successful */
	if (ret)
		goto err_res_free;

	if (uc->config.dir == DMA_DEV_TO_MEM)
		udma_rchanrt_update_bits(uc->rchan, UDMA_RCHAN_RT_CTL_REG,
					 UDMA_CHAN_RT_CTL_TDOWN | UDMA_CHAN_RT_CTL_AUTOPAIR,
					 UDMA_CHAN_RT_CTL_AUTOPAIR);
	else if (uc->config.dir == DMA_MEM_TO_DEV)
		udma_tchanrt_update_bits(uc->tchan, UDMA_TCHAN_RT_CTL_REG,
					 UDMA_CHAN_RT_CTL_TDOWN | UDMA_CHAN_RT_CTL_AUTOPAIR,
					 UDMA_CHAN_RT_CTL_AUTOPAIR);

	if (udma_is_chan_running(uc)) {
		dev_warn(ud->dev, "chan%d: is running!\n", uc->id);
		udma_stop(uc);
		if (udma_is_chan_running(uc)) {
			dev_err(ud->dev, "chan%d: won't stop!\n", uc->id);
			goto err_res_free;
		}
	}

	if (uc->tchan)
		dev_dbg(ud->dev,
			"chan%d: tchan%d, tflow%d, Remote thread: 0x%04x\n",
			uc->id, uc->tchan->id, uc->tchan->tflow_id,
			uc->config.remote_thread_id);
	else if (uc->rchan)
		dev_dbg(ud->dev,
			"chan%d: rchan%d, rflow%d, Remote thread: 0x%04x\n",
			uc->id, uc->rchan->id, uc->rflow->id,
			uc->config.remote_thread_id);
	return 0;

err_res_free:
	udma_free_tx_resources(uc);
	udma_free_rx_resources(uc);

	udma_reset_uchan(uc);

	return ret;
}

static int am62l_udma_request(struct dma *dma)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan_config *ucc;
	struct udma_chan *uc;
	int ret;

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}

	uc = &ud->channels[dma->id];
	ucc = &uc->config;
	switch (ud->match_data->type) {
	case DMA_TYPE_BCDMA_V2:
		ret = bcdma_v2_alloc_chan_resources(uc);
		break;
	case DMA_TYPE_PKTDMA_V2:
		ret = pktdma_v2_alloc_chan_resources(uc);
		break;
	default:
		return -EINVAL;
	}
	if (ret) {
		dev_err(dma->dev, "alloc dma res failed %d\n", ret);
		return -EINVAL;
	}

	if (uc->config.dir == DMA_MEM_TO_DEV) {
		uc->desc_tx = dma_alloc_coherent(DMA_DEVICE_BROKEN, ucc->hdesc_size, DMA_ADDRESS_BROKEN);
	} else {
		uc->desc_rx = dma_alloc_coherent(DMA_DEVICE_BROKEN,
						 ucc->hdesc_size * UDMA_RX_DESC_NUM, DMA_ADDRESS_BROKEN);
	}

	uc->in_use = true;
	uc->desc_rx_cur = 0;
	uc->num_rx_bufs = 0;

	if (uc->config.dir == DMA_DEV_TO_MEM) {
		uc->cfg_data.flow_id_base = uc->rflow->id;
		uc->cfg_data.flow_id_cnt = 1;
	}

	return 0;
}

static int bcdma_v2_setup_resources(struct udma_dev *ud)
{
	struct device *dev = ud->dev;
	size_t desc_size;

	ud->bchan_map = devm_kmalloc_array(dev, BITS_TO_LONGS(ud->bchan_cnt),
					   sizeof(unsigned long), GFP_KERNEL);
	ud->bchans = devm_kcalloc(dev, ud->bchan_cnt, sizeof(*ud->bchans),
				  GFP_KERNEL);
	ud->tchan_map = devm_kmalloc_array(dev, BITS_TO_LONGS(ud->tchan_cnt),
					   sizeof(unsigned long), GFP_KERNEL);
	ud->tchans = devm_kcalloc(dev, ud->tchan_cnt, sizeof(*ud->tchans),
				  GFP_KERNEL);
	ud->rchan_map = devm_kmalloc_array(dev, BITS_TO_LONGS(ud->rchan_cnt),
					   sizeof(unsigned long), GFP_KERNEL);
	ud->rchans = devm_kcalloc(dev, ud->rchan_cnt, sizeof(*ud->rchans),
				  GFP_KERNEL);
	ud->rflows = devm_kcalloc(dev, ud->rchan_cnt, sizeof(*ud->rflows),
				  GFP_KERNEL);

	desc_size = cppi5_trdesc_calc_size(K3_UDMA_MAX_TR,
					   sizeof(struct cppi5_tr_type15_t));
	ud->bc_desc = devm_kzalloc(dev, ALIGN(desc_size, ARCH_DMA_MINALIGN), GFP_KERNEL);

	if (!ud->bchan_map || !ud->tchan_map || !ud->rchan_map ||
	    !ud->bchans || !ud->tchans || !ud->rchans ||
	    !ud->rflows || !ud->bc_desc)
		return -ENOMEM;

	bitmap_zero(ud->bchan_map, ud->bchan_cnt);
	bitmap_zero(ud->tchan_map, ud->tchan_cnt);
	bitmap_zero(ud->rchan_map, ud->rchan_cnt);

	return 0;
}

static int pktdma_v2_setup_resources(struct udma_dev *ud)
{
	struct device *dev = ud->dev;

	ud->tchan_map = devm_kmalloc_array(dev, BITS_TO_LONGS(ud->tchan_cnt),
					   sizeof(unsigned long), GFP_KERNEL);
	ud->tchans = devm_kcalloc(dev, ud->tchan_cnt, sizeof(*ud->tchans),
				  GFP_KERNEL);
	ud->rchan_map = devm_kmalloc_array(dev, BITS_TO_LONGS(ud->rchan_cnt),
					   sizeof(unsigned long), GFP_KERNEL);
	ud->rchans = devm_kcalloc(dev, ud->rchan_cnt, sizeof(*ud->rchans),
				  GFP_KERNEL);
	ud->rflow_map = devm_kcalloc(dev, BITS_TO_LONGS(ud->rflow_cnt),
				     sizeof(unsigned long),
				     GFP_KERNEL);
	ud->rflows = devm_kcalloc(dev, ud->rflow_cnt, sizeof(*ud->rflows),
				  GFP_KERNEL);
	ud->tflow_map = devm_kmalloc_array(dev, BITS_TO_LONGS(ud->tflow_cnt),
					   sizeof(unsigned long), GFP_KERNEL);

	if (!ud->tchan_map || !ud->rchan_map || !ud->tflow_map || !ud->tchans ||
	    !ud->rchans || !ud->rflows || !ud->rflow_map)
		return -ENOMEM;

	bitmap_zero(ud->bchan_map, ud->bchan_cnt);
	bitmap_zero(ud->tchan_map, ud->tchan_cnt);
	bitmap_zero(ud->rchan_map, ud->rchan_cnt);

	return 0;
}

static int am62l_udma_setup_resources(struct udma_dev *ud)
{
	struct device *dev = ud->dev;
	int ch_count, ret;

	switch (ud->match_data->type) {
	case DMA_TYPE_BCDMA_V2:
		ret = bcdma_v2_setup_resources(ud);
		break;
	case DMA_TYPE_PKTDMA_V2:
		ret = pktdma_v2_setup_resources(ud);
		break;
	default:
		return -EINVAL;
	}

	if (ret)
		return ret;

	ch_count = ud->bchan_cnt + ud->tchan_cnt + ud->rchan_cnt;

	ud->channels = devm_kcalloc(dev, ch_count, sizeof(*ud->channels),
				    GFP_KERNEL);
	if (!ud->channels)
		return -ENOMEM;

	switch (ud->match_data->type) {
	case DMA_TYPE_UDMA:
		dev_dbg(dev,
			"Channels: %d (tchan: %u, rchan: %u, gp-rflow: %u)\n",
			ch_count, ud->tchan_cnt, ud->rchan_cnt, ud->rflow_cnt);
		break;
	case DMA_TYPE_BCDMA:
	case DMA_TYPE_BCDMA_V2:
		dev_dbg(dev,
			"Channels: %d (bchan: %u, tchan: %u, rchan: %u)\n",
			ch_count, ud->bchan_cnt, ud->tchan_cnt, ud->rchan_cnt);
		break;
	case DMA_TYPE_PKTDMA_V2:
		dev_dbg(dev,
			"Channels: %d (tchan: %u, rchan: %u)\n",
			ch_count, ud->tchan_cnt, ud->rchan_cnt);
		break;
	default:
		break;
	}

	return ch_count;
}

static int am62l_udma_get_mmrs(struct device *dev, struct udma_dev *ud)
{
	int i;

	/* There are no tchan and rchan in BCDMA_V2 and PKTDMA_V2.
	 * Duplicate chan as tchan and rchan to keep the common code
	 * in k3-udma-common.c functional.
	 */
	if (ud->match_data->type == DMA_TYPE_BCDMA_V2) {
		ud->bchan_cnt = ud->match_data->bchan_cnt;
		ud->chan_cnt = ud->match_data->chan_cnt;
		ud->tchan_cnt = ud->match_data->chan_cnt;
		ud->rchan_cnt = ud->match_data->chan_cnt;
		ud->rflow_cnt = ud->chan_cnt;
	} else if (ud->match_data->type == DMA_TYPE_PKTDMA_V2) {
		ud->chan_cnt = ud->match_data->chan_cnt;
		ud->tchan_cnt = ud->match_data->tchan_cnt;
		ud->rchan_cnt = ud->match_data->rchan_cnt;
		ud->rflow_cnt = ud->match_data->rflow_cnt;
	}

	for (i = 1; i < AM62L_MMR_LAST; i++) {
		if (i == AM62L_MMR_BCHANRT && ud->bchan_cnt == 0)
			continue;
		if (i == AM62L_MMR_CHANRT && ud->chan_cnt == 0)
			continue;

		ud->mmrs[i] = dev_request_mem_region_by_name(ud->dev, am62l_mmr_names[i]);
		if (IS_ERR(ud->mmrs[i]))
			return PTR_ERR(ud->mmrs[i]);
	}

	return 0;
}

static int am62l_udma_rfree(struct dma *dma)
{
	struct udma_dev *ud = dev_get_priv(dma->dev);
	struct udma_chan *uc;

	if (dma->id >= (ud->rchan_cnt + ud->tchan_cnt)) {
		dev_err(dma->dev, "invalid dma ch_id %lu\n", dma->id);
		return -EINVAL;
	}
	uc = &ud->channels[dma->id];

	if (udma_is_chan_running(uc))
		udma_stop(uc);

	bcdma_free_bchan_resources(uc);
	udma_free_tx_resources(uc);
	udma_free_rx_resources(uc);
	udma_reset_uchan(uc);

	uc->in_use = false;

	return 0;
}

static const struct dma_device_ops am62l_udma_ops = {
	.transfer	= udma_transfer,
	.of_xlate	= udma_of_xlate,
	.request	= am62l_udma_request,
	.rfree		= am62l_udma_rfree,
	.enable		= udma_enable,
	.disable	= udma_disable,
	.send		= udma_send,
	.receive	= udma_receive,
	.prepare_rcv_buf = udma_prepare_rcv_buf,
	.get_cfg	= udma_get_cfg,
};

static int k3_udma_am62l_probe(struct device *dev)
{
	struct udma_dev *ud;
	struct udma_chan *uc;
	int i, ret;
	struct k3_ringacc_init_data ring_init_data = { 0 };
	const struct udma_match_data *match_data;
	struct dma_device *dmad;

	match_data = device_get_match_data(dev);

	ud = xzalloc(sizeof(*ud));
	ud->match_data = match_data;
	ud->dev = dev;

	dev->priv = ud;

	ret = am62l_udma_get_mmrs(dev, ud);
	if (ret)
		return ret;

	if (ud->match_data->type == DMA_TYPE_BCDMA_V2)
		ring_init_data.num_rings = ud->bchan_cnt + ud->chan_cnt;
	else if (ud->match_data->type == DMA_TYPE_PKTDMA_V2)
		ring_init_data.num_rings = ud->rflow_cnt;

	ring_init_data.base_rt = ud->mmrs[AM62L_MMR_RFLOWRT];

	ud->ringacc = k3_ringacc_dmarings_init(dev, &ring_init_data);
	if (IS_ERR(ud->ringacc))
		return PTR_ERR(ud->ringacc);

	ret = am62l_udma_setup_resources(ud);
	if (ret < 0)
		return ret;

	ud->ch_count = ret;

	for (i = 0; i < ud->bchan_cnt; i++) {
		struct udma_bchan *bchan = &ud->bchans[i];

		bchan->id = i;
		bchan->reg_rt = ud->mmrs[AM62L_MMR_BCHANRT] + i * 0x1000;
	}

	for (i = 0; i < ud->tchan_cnt; i++) {
		struct udma_tchan *tchan = &ud->tchans[i];

		tchan->id = i;
		tchan->reg_rt = ud->mmrs[AM62L_MMR_CHANRT] + UDMA_CH_1000(i);
		tchan->reg_chan = tchan->reg_rt + 0x4;
	}

	for (i = 0; i < ud->rchan_cnt; i++) {
		struct udma_rchan *rchan = &ud->rchans[i];

		rchan->id = i;
		rchan->reg_rt = ud->mmrs[AM62L_MMR_CHANRT] + UDMA_CH_1000(i);
		rchan->reg_chan = rchan->reg_rt + 0x4;
	}

	for (i = 0; i < ud->rflow_cnt; i++) {
		struct udma_rflow *rflow = &ud->rflows[i];

		rflow->id = i;
		rflow->reg_rt = ud->mmrs[AM62L_MMR_RFLOWRT] + UDMA_CH_1000(2 * i);
		rflow->reg_rflow = rflow->reg_rt;
	}

	for (i = 0; i < ud->ch_count; i++) {
		struct udma_chan *uc = &ud->channels[i];

		uc->ud = ud;
		uc->id = i;
		uc->config.remote_thread_id = -1;
		uc->bchan = NULL;
		uc->tchan = NULL;
		uc->rchan = NULL;
		uc->config.mapped_channel_id = -1;
		uc->config.default_flow_id = -1;
		uc->config.dir = DMA_MEM_TO_MEM;
		sprintf(uc->name, "UDMA chan%d\n", i);
		if (!i)
			uc->in_use = true;
	}

	uc = &ud->channels[0];
	ret = 0;
	switch (ud->match_data->type) {
	case DMA_TYPE_BCDMA_V2:
		ret = bcdma_v2_alloc_chan_resources(uc);
		break;
	default:
		break; /* Do nothing in any other case */
	};

	if (ret) {
		dev_err(dev, " Channel 0 allocation failure %d\n", ret);
		return ret;
	}

	dmad = &ud->dmad;

	dmad->dev = dev;
	dmad->ops = &am62l_udma_ops;

	ret = dma_device_register(dmad);

	return ret;
}

static void am62l_udma_remove(struct device *dev)
{
	struct udma_dev *ud = dev_get_priv(dev);
	struct udma_chan *uc = &ud->channels[0];

	switch (ud->match_data->type) {
	case DMA_TYPE_BCDMA_V2:
		bcdma_free_bchan_resources(uc);
		break;
	default:
		break;
	};
}

static struct udma_match_data am62l_bcdma_data = {
	.type = DMA_TYPE_BCDMA_V2,
	.psil_base = 0x2000, /* for tchan and rchan, not applicable to bchan */
	.enable_memcpy_support = true, /* Supported via bchan */
	.flags = UDMA_FLAG_PDMA_ACC32 | UDMA_FLAG_PDMA_BURST | UDMA_FLAG_TDTYPE,
	.statictr_z_mask = GENMASK(23, 0),
	.bchan_cnt = 16,
	.chan_cnt = 128,
	.tchan_cnt = 128,
	.rchan_cnt = 128,
};

static struct udma_match_data am62l_pktdma_data = {
	.type = DMA_TYPE_PKTDMA_V2,
	.psil_base = 0x1000,
	.enable_memcpy_support = false,
	.flags = UDMA_FLAG_PDMA_ACC32 | UDMA_FLAG_PDMA_BURST | UDMA_FLAG_TDTYPE,
	.statictr_z_mask = GENMASK(23, 0),
	.tchan_cnt = 97,
	.rchan_cnt = 97,
	.chan_cnt = 97,
	.tflow_cnt = 112,
	.rflow_cnt = 112,
};

static struct of_device_id k3_udma_am62l_dt_ids[] = {
	{
		.compatible = "ti,am62l-dmss-bcdma",
		.data = &am62l_bcdma_data,
	}, {
		.compatible = "ti,am62l-dmss-pktdma",
		.data = &am62l_pktdma_data,
	}, {
		/* Sentinel */
	},
};

static struct driver k3_udma_am62l_driver = {
	.probe  = k3_udma_am62l_probe,
	.remove = am62l_udma_remove,
	.name   = "k3-udma-am62l",
	.of_compatible = k3_udma_am62l_dt_ids,
};

core_platform_driver(k3_udma_am62l_driver);

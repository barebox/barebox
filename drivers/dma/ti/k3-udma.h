/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  Copyright (C) 2025 Texas Instruments Incorporated - https://www.ti.com
 *
 */

#ifndef K3_UDMA_H
#define K3_UDMA_H

#include <linux/types.h>
#include <soc/ti/k3-navss-ringacc.h>
#include <soc/ti/cppi5.h>
#include <soc/ti/ti-udma.h>
#include <soc/ti/ti_sci_protocol.h>
#include <soc/ti/cppi5.h>
#include <dma-devices.h>

#include "k3-udma-hwdef.h"
#include "k3-udma.h"
#include "k3-psil-priv.h"

enum k3_dma_type {
	DMA_TYPE_UDMA = 0,
	DMA_TYPE_BCDMA,
	DMA_TYPE_PKTDMA,
	DMA_TYPE_BCDMA_V2,
	DMA_TYPE_PKTDMA_V2,
};

enum udma_mmr {
	MMR_GCFG = 0,
	MMR_BCHANRT,
	MMR_RCHANRT,
	MMR_TCHANRT,
	MMR_RCHAN,
	MMR_TCHAN,
	MMR_RFLOW,
	MMR_LAST,
};

enum udma_rm_range {
	RM_RANGE_BCHAN = 0,
	RM_RANGE_TCHAN,
	RM_RANGE_RCHAN,
	RM_RANGE_RFLOW,
	RM_RANGE_TFLOW,
	RM_RANGE_LAST,
};

struct udma_tisci_rm {
	const struct ti_sci_handle *tisci;
	const struct ti_sci_rm_udmap_ops *tisci_udmap_ops;
	u32  tisci_dev_id;

	/* tisci information for PSI-L thread pairing/unpairing */
	const struct ti_sci_rm_psil_ops *tisci_psil_ops;
	u32  tisci_navss_dev_id;

	struct ti_sci_resource *rm_ranges[RM_RANGE_LAST];
};

// Structure definitions
struct udma_tchan {
	void __iomem *reg_chan;
	void __iomem *reg_rt;

	int id;
	struct k3_ring *t_ring; /* Transmit ring */
	struct k3_ring *tc_ring; /* Transmit Completion ring */
	int tflow_id; /* applicable only for PKTDMA */
};

#define udma_bchan udma_tchan

struct udma_rflow {
	void __iomem *reg_rflow;
	void __iomem *reg_rt;
	int id;
	struct k3_ring *fd_ring; /* Free Descriptor ring */
	struct k3_ring *r_ring; /* Receive ring */
};

struct udma_rchan {
	void __iomem *reg_chan;
	void __iomem *reg_rt;

	int id;
};

struct udma_oes_offsets {
	/* K3 UDMA Output Event Offset */
	u32 udma_rchan;

	/* BCDMA Output Event Offsets */
	u32 bcdma_bchan_data;
	u32 bcdma_bchan_ring;
	u32 bcdma_tchan_data;
	u32 bcdma_tchan_ring;
	u32 bcdma_rchan_data;
	u32 bcdma_rchan_ring;

	/* PKTDMA Output Event Offsets */
	u32 pktdma_tchan_flow;
	u32 pktdma_rchan_flow;
};

#define UDMA_FLAG_PDMA_ACC32		BIT(0)
#define UDMA_FLAG_PDMA_BURST		BIT(1)
#define UDMA_FLAG_TDTYPE		BIT(2)

struct udma_match_data {
	u32 type;
	u32 psil_base;
	bool enable_memcpy_support;
	u32 flags;
	u32 statictr_z_mask;
	struct udma_oes_offsets oes;

	u8 tpl_levels;
	u32 bchan_cnt;
	u32 tchan_cnt;
	u32 rchan_cnt;
	u32 tflow_cnt;
	u32 rflow_cnt;
	u32 chan_cnt;

	u32 level_start_idx[];
};

struct udma_dev {
	struct dma_device dmad;
	struct device *dev;
	void __iomem *mmrs[MMR_LAST];

	struct udma_tisci_rm tisci_rm;
	struct k3_ringacc *ringacc;

	u32 features;

	int bchan_cnt;
	int tchan_cnt;
	int echan_cnt;
	int rchan_cnt;
	int rflow_cnt;
	int tflow_cnt;
	int chan_cnt;
	unsigned long *bchan_map;
	unsigned long *tchan_map;
	unsigned long *rchan_map;
	unsigned long *rflow_map;
	unsigned long *rflow_map_reserved;
	unsigned long *tflow_map;

	struct udma_bchan *bchans;
	struct udma_tchan *tchans;
	struct udma_rchan *rchans;
	struct udma_rflow *rflows;

	const struct udma_match_data *match_data;
	void *bc_desc;

	struct udma_chan *channels;
	u32 psil_base;

	u32 ch_count;
};

struct udma_chan_config {
	u32 psd_size; /* size of Protocol Specific Data */
	u32 metadata_size; /* (needs_epib ? 16:0) + psd_size */
	u32 hdesc_size; /* Size of a packet descriptor in packet mode */
	int remote_thread_id;
	u32 atype;
	u32 src_thread;
	u32 dst_thread;
	enum psil_endpoint_type ep_type;
	enum udma_tp_level channel_tpl; /* Channel Throughput Level */

	/* PKTDMA mapped channel */
	int mapped_channel_id;
	/* PKTDMA default tflow or rflow for mapped channel */
	int default_flow_id;

	enum dma_transfer_direction dir;

	unsigned int pkt_mode:1; /* TR or packet */
	unsigned int needs_epib:1; /* EPIB is needed for the communication or not */
	unsigned int enable_acc32:1;
	unsigned int enable_burst:1;
	unsigned int notdpkt:1; /* Suppress sending TDC packet */

	u8 asel;
};

struct udma_chan {
	struct udma_dev *ud;
	char name[20];

	struct udma_bchan *bchan;
	struct udma_tchan *tchan;
	struct udma_rchan *rchan;
	struct udma_rflow *rflow;

	struct ti_udma_drv_chan_cfg_data cfg_data;

	u32 bcnt; /* number of bytes completed since the start of the channel */

	struct udma_chan_config config;

	u32 id;

	struct cppi5_host_desc_t *desc_tx;
	bool in_use;
	void	*desc_rx;
	u32	num_rx_bufs;
	u32	desc_rx_cur;

};

#define UDMA_CH_1000(ch)		(ch * 0x1000)
#define UDMA_CH_100(ch)			(ch * 0x100)
#define UDMA_CH_40(ch)			(ch * 0x40)

#define UDMA_RX_DESC_NUM 128

#define K3_UDMA_MAX_RFLOWS 1024
#define K3_UDMA_MAX_TR 2

/* Generic register access functions */
static inline u32 udma_read(void __iomem *base, int reg)
{
	u32 v;

	v = readl(base + reg);

	return v;
}

static inline void udma_write(void __iomem *base, int reg, u32 val)
{
	writel(val, base + reg);
}

static inline void udma_update_bits(void __iomem *base, int reg,
				    u32 mask, u32 val)
{
	u32 tmp, orig;

	orig = udma_read(base, reg);
	tmp = orig & ~mask;
	tmp |= (val & mask);

	if (tmp != orig)
		udma_write(base, reg, tmp);
}

/* TCHANRT */
static inline u32 udma_tchanrt_read(struct udma_tchan *tchan, int reg)
{
	if (!tchan)
		return 0;
	return udma_read(tchan->reg_rt, reg);
}

static inline void udma_tchanrt_write(struct udma_tchan *tchan,
				      int reg, u32 val)
{
	if (!tchan)
		return;
	udma_write(tchan->reg_rt, reg, val);
}

static inline void udma_tchanrt_update_bits(struct udma_tchan *tchan, int reg,
					    u32 mask, u32 val)
{
	if (!tchan)
		return;
	udma_update_bits(tchan->reg_rt, reg, mask, val);
}

/* RCHANRT */
static inline u32 udma_rchanrt_read(struct udma_rchan *rchan, int reg)
{
	if (!rchan)
		return 0;
	return udma_read(rchan->reg_rt, reg);
}

static inline void udma_rchanrt_write(struct udma_rchan *rchan,
				      int reg, u32 val)
{
	if (!rchan)
		return;
	udma_write(rchan->reg_rt, reg, val);
}

static inline void udma_rflowrt_write(struct udma_rflow *rflow,
				      int reg, u32 val)
{
	if (!rflow)
		return;
	udma_write(rflow->reg_rflow, reg, val);
}

static inline void udma_rchanrt_update_bits(struct udma_rchan *rchan, int reg,
					    u32 mask, u32 val)
{
	if (!rchan)
		return;
	udma_update_bits(rchan->reg_rt, reg, mask, val);
}

static inline void udma_rflowrt_update_bits(struct udma_rflow *rflow, int reg,
					    u32 mask, u32 val)
{
	if (!rflow)
		return;
	udma_update_bits(rflow->reg_rflow, reg, mask, val);
}

// Function headers
int udma_alloc_rx_resources(struct udma_chan *uc);
int udma_alloc_tx_resources(struct udma_chan *uc);
void udma_free_rx_resources(struct udma_chan *uc);
void udma_free_tx_resources(struct udma_chan *uc);
int udma_transfer(struct device *dev, int direction, dma_addr_t dst,
		  dma_addr_t src, size_t len);
int udma_enable(struct dma *dma);
int udma_disable(struct dma *dma);
int udma_send(struct dma *dma, dma_addr_t src, size_t len, void *metadata);
int udma_receive(struct dma *dma, dma_addr_t *dst, void *metadata);
int udma_get_cfg(struct dma *dma, u32 id, void **data);
int udma_prepare_rcv_buf(struct dma *dma, dma_addr_t dst, size_t size);
int udma_of_xlate(struct dma *dma, struct of_phandle_args *args);

int udma_alloc_tx_resources(struct udma_chan *uc);
int udma_alloc_rx_resources(struct udma_chan *uc);
void bcdma_free_bchan_resources(struct udma_chan *uc);
int bcdma_alloc_bchan_resources(struct udma_chan *uc);

void udma_stop_mem2dev(struct udma_chan *uc, bool sync);
int udma_start(struct udma_chan *uc);
void udma_reset_counters(struct udma_chan *uc);
int udma_stop_hard(struct udma_chan *uc);
int udma_stop(struct udma_chan *uc);
bool udma_is_chan_running(struct udma_chan *uc);
void udma_stop_dev2mem(struct udma_chan *uc, bool sync);
void udma_reset_uchan(struct udma_chan *uc);
void udma_reset_rings(struct udma_chan *uc);

char *udma_get_dir_text(enum dma_transfer_direction dir);

// Common macros
#define K3_UDMA_MAX_RFLOWS 1024
#define K3_UDMA_MAX_TR 2

#endif // K3_UDMA_H

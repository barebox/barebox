// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com
 *  Author: Peter Ujfalusi <peter.ujfalusi@ti.com>
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/err.h>
#include <of.h>

#include "k3-psil-priv.h"

#define PSIL_ETHERNET(x, ch, flow_base, flow_cnt)		\
	{							\
		.thread_id = x,					\
		.ep_config = {					\
			.ep_type = PSIL_EP_NATIVE,		\
			.pkt_mode = 1,				\
			.needs_epib = 1,			\
			.psd_size = 16,				\
			.mapped_channel_id = ch,		\
			.flow_start = flow_base,		\
			.flow_num = flow_cnt,			\
			.default_flow_id = flow_base,		\
		},						\
	}

/* PSI-L source thread IDs, used for RX (DMA_DEV_TO_MEM) */
static struct psil_ep am62_src_ep_map[] = {
	/* CPSW3G */
	PSIL_ETHERNET(0x4600, 19, 19, 16),
};

/* PSI-L destination thread IDs, used for TX (DMA_MEM_TO_DEV) */
static struct psil_ep am62_dst_ep_map[] = {
	/* CPSW3G */
	PSIL_ETHERNET(0xc600, 19, 19, 8),
	PSIL_ETHERNET(0xc601, 20, 27, 8),
	PSIL_ETHERNET(0xc602, 21, 35, 8),
	PSIL_ETHERNET(0xc603, 22, 43, 8),
	PSIL_ETHERNET(0xc604, 23, 51, 8),
	PSIL_ETHERNET(0xc605, 24, 59, 8),
	PSIL_ETHERNET(0xc606, 25, 67, 8),
	PSIL_ETHERNET(0xc607, 26, 75, 8),
};

struct psil_ep_map am62_ep_map = {
	.name = "am62",
	.src = am62_src_ep_map,
	.src_count = ARRAY_SIZE(am62_src_ep_map),
	.dst = am62_dst_ep_map,
	.dst_count = ARRAY_SIZE(am62_dst_ep_map),
};

static const struct psil_ep_map *soc_ep_map;

struct psil_endpoint_config *psil_get_ep_config(u32 thread_id)
{
	int i;

	if (!soc_ep_map) {
		if (of_machine_is_compatible("ti,am625"))
			soc_ep_map = &am62_ep_map;
		else if (of_machine_is_compatible("ti,am62l3"))
			soc_ep_map = &am62l_ep_map;
	}

	if (!soc_ep_map) {
		pr_err("Cannot find a soc_ep_map for the current machine\n");
		return ERR_PTR(-ENODEV);
	}

	if (thread_id & K3_PSIL_DST_THREAD_ID_OFFSET && soc_ep_map->dst) {
		/* check in destination thread map */
		for (i = 0; i < soc_ep_map->dst_count; i++) {
			if (soc_ep_map->dst[i].thread_id == thread_id)
				return &soc_ep_map->dst[i].ep_config;
		}
	}

	thread_id &= ~K3_PSIL_DST_THREAD_ID_OFFSET;
	if (soc_ep_map->src) {
		for (i = 0; i < soc_ep_map->src_count; i++) {
			if (soc_ep_map->src[i].thread_id == thread_id)
				return &soc_ep_map->src[i].ep_config;
		}
	}

	return ERR_PTR(-ENOENT);
}

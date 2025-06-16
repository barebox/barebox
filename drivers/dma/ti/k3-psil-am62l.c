// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2025 Texas Instruments Incorporated - https://www.ti.com
 */

#include <linux/kernel.h>

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
static struct psil_ep am62l_src_ep_map[] = {
	/* CPSW3G */
	PSIL_ETHERNET(0x4600, 96, 96, 16),
};

/* PSI-L destination thread IDs, used for TX (DMA_MEM_TO_DEV) */
static struct psil_ep am62l_dst_ep_map[] = {
	/* CPSW3G */
	PSIL_ETHERNET(0xc600, 64, 64, 2),
	PSIL_ETHERNET(0xc601, 66, 66, 2),
	PSIL_ETHERNET(0xc602, 68, 68, 2),
	PSIL_ETHERNET(0xc603, 70, 70, 2),
	PSIL_ETHERNET(0xc604, 72, 72, 2),
	PSIL_ETHERNET(0xc605, 74, 74, 2),
	PSIL_ETHERNET(0xc606, 76, 76, 2),
	PSIL_ETHERNET(0xc607, 78, 78, 2),
};

struct psil_ep_map am62l_ep_map = {
	.name = "am62l",
	.src = am62l_src_ep_map,
	.src_count = ARRAY_SIZE(am62l_src_ep_map),
	.dst = am62l_dst_ep_map,
	.dst_count = ARRAY_SIZE(am62l_dst_ep_map),
};
